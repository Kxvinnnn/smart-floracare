/**
 * ============================================================================
 * SMART FLORACARE - FIRMWARE ARQUITECTÓNICO ESP32 v1.2.1 (V4.18 CONSOLIDADA)
 * ============================================================================
 * Plataforma: ESP32 DevKit V1 (30 pines, 3.3V lógica)
 * Framework: Arduino ESP32 core v3.3.12 (ESP-IDF v5.5.5)
 * 
 * Normas de Seguridad y Diseño:
 * - FSM desacoplada con corte bruto prioritario de boya de nivel (REQ-07, ACT-05).
 * - Drivers pasivos seguros durante reset y boot (H-150).
 * - Parser HTTP no bloqueante RFC 7230/9112 con transmisión chunked PROGMEM (H-151).
 * - Autenticación HMAC-SHA256 (RFC 2104) con verificación de vector normativo al arranque.
 * - Tabla de rate limiting con política de desalojo LRU y defensa por saturación (H-152).
 * - Socket lwIP con O_NONBLOCK y MSG_DONTWAIT estricto (H-153).
 * ============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <NetworkServer.h>
#include <NetworkClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#include <esp_random.h>
#include <mbedtls/md.h>
#include <lwip/sockets.h>
#include <fcntl.h>

#include "IrrigationController.h"
#include "SensorValidator.h"
#include "HttpSecurityParser.h"

using FloraCare::HttpParseResult;
using FloraCare::parseHttpHeaderStrict;
using FloraCare::isResidualBodyValid;
using FloraCare::isHexStrict;
using FloraCare::selectLruCandidate;
using FloraCare::PARSE_OK;
using FloraCare::PARSE_METHOD_NOT_ALLOWED;
using FloraCare::PARSE_BAD_URI;
using FloraCare::PARSE_BAD_REQUEST;
using FloraCare::PARSE_PAYLOAD_TOO_LARGE;
using FloraCare::PARSE_NOT_IMPLEMENTED;

// Inclusión de credenciales locales o respaldo seguro
#if __has_include("Credentials.h")
  #include "Credentials.h"
  #define HAS_LOCAL_CREDENTIALS 1
#else
  #define HAS_LOCAL_CREDENTIALS 0
#endif

#if HAS_LOCAL_CREDENTIALS && defined(SECRET_WIFI_SSID) && defined(SECRET_WIFI_PASS)
  static const char* WIFI_SSID       = SECRET_WIFI_SSID;
  static const char* WIFI_PASS       = SECRET_WIFI_PASS;
  static constexpr bool WIFI_ENABLED = true;
#else
  static const char* WIFI_SSID       = "";
  static const char* WIFI_PASS       = "";
  static constexpr bool WIFI_ENABLED = false;
#endif

#if HAS_LOCAL_CREDENTIALS && defined(SECRET_API_TOKEN)
  static const char* API_AUTH_TOKEN = SECRET_API_TOKEN;
#else
  static const char* API_AUTH_TOKEN = "";
#endif

// ============================================================================
// CONSTANTES HARDWARE Y ASIGNACIÓN COHERENTE DE PINES (H-148)
// ============================================================================
static constexpr uint8_t PIN_SENSOR_HUMEDAD = 34; // ADC1_CH6 (Entrada analógica pura)
static constexpr uint8_t PIN_DRIVER_BOMBA   = 18; // Salida digital a Pre-Driver NPN+PNP
static constexpr uint8_t PIN_FLOTADOR_AGUA  = 19; // Entrada digital (INPUT_PULLUP)
static constexpr uint8_t PIN_BUZZER         = 23; // Salida digital al zumbador
static constexpr uint8_t PIN_BOTON_MANUAL   = 4;  // Entrada digital (INPUT_PULLUP)

// Dimensiones de la pantalla OLED I2C
static constexpr uint8_t SCREEN_WIDTH  = 128;
static constexpr uint8_t SCREEN_HEIGHT = 64;
static constexpr int8_t  OLED_RESET    = -1;
static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
static bool oledAvailable = false;

// ============================================================================
// ESTRUCTURAS DE DOMINIO Y CONFIGURACIÓN CANÓNICA (H-147)
// ============================================================================
static constexpr FloraCare::CalibrationProfile ESP32_CALIBRATION = {
    3100, // airRaw (Seco total en aire libre)
    1350, // waterRaw (Sumergido en agua)
    100,  // minPhysLimit (Bajo 100 implica cortocircuito)
    4000, // maxPhysLimit (Sobre 4000 implica desconexión)
    200   // minDelta (Excursión mínima admisible)
};

static constexpr FloraCare::ControllerConfig IRRIGATION_CFG = {
#if defined(FLORACARE_BENCH_TEST_WATCHDOG_25S)
    30000, // Modo ensayo aislado ACT-06.A2 (30 s para verificar disparo WDT a 25 s)
#else
    20000, // maxAutoDurationMs: 20 segundos máximo en riego automático
#endif
    3000,  // manualBurstDurationMs: 3 segundos por ráfaga
    25000, // maxSafetyWatchdogMs: 25 segundos (Límite inmutable de seguridad)
    5000,  // cooldownPeriodMs: 5 segundos de enfriamiento obligatorio
    25,    // autoStartMoisturePct: Iniciar si humedad <= 25%
    70     // autoStopMoisturePct: Detener si humedad >= 70%
};

static FloraCare::IrrigationController controller(IRRIGATION_CFG);

// Variables de estado del sistema
static uint32_t lastSampleTimeMs     = 0;
static uint32_t lastDisplayTimeMs    = 0;
static uint32_t lastBeepTimeMs       = 0;
static uint8_t  currentMoisturePct   = 50;
static FloraCare::SensorHealth sensorStatus = FloraCare::SensorHealth::OK;
static bool     waterLevelOk         = true;
static bool     lastButtonState      = HIGH;
static uint32_t lastButtonDebounceMs = 0;

// Interbloqueo preventivo: inhibe ciclos automáticos tras disparo de watchdog de seguridad (H-156)
static bool     s_autoInhibitAfterTimeout = false;

// Estado del pin físico comandado (H-150)
static bool s_orderedPumpLevel = false;

// ============================================================================
// DEFINICIONES DE SEGURIDAD CRIPTOGRÁFICA Y RATE LIMITING (H-152)
// ============================================================================
static constexpr size_t   CHALLENGE_POOL_SIZE       = 4;
static constexpr size_t   AUTH_FAIL_CACHE_SIZE      = 8;
static constexpr uint32_t NONCE_VALIDITY_WINDOW_MS  = 15000;
static constexpr uint32_t PER_IP_NONCE_COOLDOWN_MS  = 500;
static constexpr uint32_t AUTH_FAIL_COOLDOWN_MS     = 3000;
static constexpr size_t   MAX_ACTIVE_PER_IP         = 2;

struct ChallengeSlot {
    uint8_t   nonce[16];
    IPAddress clientIp;
    uint32_t  issuedAtMs;
    uint8_t   failedAttempts;
    bool      valid;
    bool      consumed;
};

struct IpRateLimitRecord {
    IPAddress clientIp;
    uint32_t  lastAccessMs;
    uint32_t  lastNonceIssueMs;
    uint32_t  lastAuthFailMs;
    bool      hasIssuedNonce;
    bool      hasAuthFailed;
    bool      active;
};

static ChallengeSlot     s_challengePool[CHALLENGE_POOL_SIZE];
static IpRateLimitRecord s_ipRateLimits[AUTH_FAIL_CACHE_SIZE];

enum IssueResult {
    ISSUE_OK,
    ISSUE_POOL_EXHAUSTED,
    ISSUE_IP_QUOTA_EXCEEDED,
    ISSUE_IP_COOLDOWN,
    ISSUE_INSECURE_TOKEN,
    ISSUE_RATE_TABLE_FULL
};

// ============================================================================
// COMUNICACIÓN HTTP NO BLOQUEANTE Y CONTROLADORES DE RUTA (H-151, H-153)
// ============================================================================
static NetworkServer s_httpServer(80);
static NetworkClient s_client;

enum HttpConnectionState {
    HTTP_STATE_IDLE,
    HTTP_STATE_READ_HEADERS,
    HTTP_STATE_READ_BODY,
    HTTP_STATE_SENDING_HEADERS,
    HTTP_STATE_SENDING_BODY,
    HTTP_STATE_CLOSE
};

static HttpConnectionState s_connState = HTTP_STATE_IDLE;
static uint32_t s_connStartMs = 0;

static char   s_rxHdrBuf[512];
static size_t s_hdrLen = 0;
static char   s_rxBodyBuf[257]; // 256 bytes de payload + 1 byte de terminador '\0'
static size_t s_bodyLen = 0;
static int    s_contentLen = -1;
static char   s_uri[32];
static bool   s_isGet = false;
static bool   s_isPost = false;

// Búferes de transmisión estructurada
static char        s_txHdrBuf[256];
static size_t      s_txHdrLen = 0;
static size_t      s_txHdrSent = 0;
static const char* s_txBodyPtr = nullptr;
static size_t      s_txBodyLen = 0;
static size_t      s_txBodySent = 0;
static bool        s_isBodyProgmem = false;
static char        s_txChunkBuf[256];

// ============================================================================
// INTERFAZ DE USUARIO WEB ALMACENADA EN FLASH (PROGMEM)
// ============================================================================
static const char HTML_PAGE[] PROGMEM = 
"<!DOCTYPE html><html lang='es'><head><meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1.0'>"
"<title>FloraCare Pro</title><style>"
"body{font-family:system-ui,sans-serif;background:#121212;color:#e0e0e0;margin:0;padding:20px;text-align:center}"
".card{background:#1e1e1e;max-width:400px;margin:20px auto;padding:20px;border-radius:12px;box-shadow:0 4px 10px rgba(0,0,0,0.5)}"
"h1{color:#4caf50;margin-top:0}.val{font-size:3rem;font-weight:bold;margin:10px 0}"
"button{background:#4caf50;color:#fff;border:none;padding:12px 24px;font-size:1rem;border-radius:6px;cursor:pointer;width:100%}"
"button:disabled{background:#555;cursor:not-allowed}"
".status{margin-top:15px;font-size:0.9rem;color:#aaa}"
"</style></head><body>"
"<div class='card'>"
"<h1>FloraCare Pro</h1>"
"<div class='val' id='m'>--%</div>"
"<div class='status' id='w'>Nivel: --</div>"
"<div class='status' id='p'>Bomba: --</div><br>"
"<button id='b' onclick='water()'>Regar 3s</button>"
"<div class='status' id='msg'></div>"
"</div>"
"<script>"
"async function update(){"
" try{"
"  let r=await fetch('/api/status');let d=await r.json();"
"  document.getElementById('m').innerText=d.moisture+'%';"
"  document.getElementById('w').innerText='Nivel: '+(d.waterLevel?'OK':'VACÍO');"
"  document.getElementById('p').innerText='Bomba: '+(d.pumpActive?'REGANDO':'INACTIVA');"
" }catch(e){}"
"}"
"async function water(){"
" let btn=document.getElementById('b');btn.disabled=true;"
" let msg=document.getElementById('msg');msg.innerText='Obteniendo desafío...';"
" try{"
"  let r=await fetch('/api/nonce');if(!r.ok) throw new Error('Error en nonce');"
"  let d=await r.json();let n=d.nonce;"
"  let key=prompt('Ingrese clave secreta HMAC (Hex 32 caracteres):');"
"  if(!key){btn.disabled=false;msg.innerText='';return;}"
"  let h=await computeHmac(key,n);"
"  let wr=await fetch('/api/water',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({nonce:n,hmac:h})});"
"  let wd=await wr.json();"
"  msg.innerText=wd.status==='ok'?'¡Riego activado!':('Error: '+wd.message);"
" }catch(e){msg.innerText='Error: '+e.message;}"
" setTimeout(()=>{btn.disabled=false;update();},3500);"
"}"
"async function computeHmac(kHex,msgHex){"
" function toB(h){return new Uint8Array(h.match(/.{1,2}/g).map(byte=>parseInt(byte,16)));}"
" let key=await crypto.subtle.importKey('raw',toB(kHex),{name:'HMAC',hash:'SHA-256'},false,['sign']);"
" let sig=await crypto.subtle.sign('HMAC',key,toB(msgHex));"
" return Array.from(new Uint8Array(sig)).map(b=>b.toString(16).padStart(2,'0')).join('');"
"}"
"setInterval(update,2000);update();"
"</script></body></html>";

// ============================================================================
// CRIPTOGRAFÍA SEGURA Y AUTENTICACIÓN MUTUA (RFC 2104 / RFC 4231)
// ============================================================================
static bool isValidHexToken32(const char* token) {
    if (!token || strlen(token) != 32) return false;
    for (size_t i = 0; i < 32; ++i) {
        char c = token[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            return false;
        }
    }
    if (strcmp(token, "00000000000000000000000000000000") == 0) return false;
    return true;
}

static bool safeConstantTimeEquals(const uint8_t* a, const uint8_t* b, size_t len) {
    if (!a || !b) return false;
    volatile uint8_t diff = 0;
    for (size_t i = 0; i < len; ++i) {
        diff |= (a[i] ^ b[i]);
    }
    return (diff == 0);
}

static bool computeHmacSha256(const uint8_t* key, size_t keyLen,
                             const uint8_t* payload, size_t payloadLen,
                             uint8_t outDigest[32]) {
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!info) {
        mbedtls_md_free(&ctx);
        return false;
    }
    if (mbedtls_md_hmac_starts(&ctx, key, keyLen) != 0) {
        mbedtls_md_free(&ctx);
        return false;
    }
    if (mbedtls_md_hmac_update(&ctx, payload, payloadLen) != 0) {
        mbedtls_md_free(&ctx);
        return false;
    }
    if (mbedtls_md_hmac_finish(&ctx, outDigest) != 0) {
        mbedtls_md_free(&ctx);
        return false;
    }
    mbedtls_md_free(&ctx);
    return true;
}

/**
 * Arnés de auto-verificación en arranque mediante Vector Normativo RFC 4231 (TC2).
 * Clave: "Jefe" (4 bytes). Mensaje: "what do ya want for nothing?" (28 bytes).
 * Digest esperado: 5bdcc146bf60754e6a042426089575e7fb1e6d50216a94ff7221a09e04023252
 */
static bool runCryptoSelfTest() {
    static const uint8_t tcKey[] = "Jefe";
    static const uint8_t tcMsg[] = "what do ya want for nothing?";
    static const uint8_t expected[32] = {
        0x5b, 0xdc, 0xc1, 0x46, 0xbf, 0x60, 0x75, 0x4e,
        0x6a, 0x04, 0x24, 0x26, 0x08, 0x95, 0x75, 0xe7,
        0xfb, 0x1e, 0x6d, 0x50, 0x21, 0x6a, 0x94, 0xff,
        0x72, 0x21, 0xa0, 0x9e, 0x04, 0x02, 0x32, 0x52
    };
    uint8_t digest[32];
    if (!computeHmacSha256(tcKey, 4, tcMsg, 28, digest)) return false;
    return safeConstantTimeEquals(digest, expected, 32);
}

// Invalida inmediatamente todos los desafíos vigentes ante conmutación de bomba
static void invalidateAllChallenges() {
    for (size_t i = 0; i < CHALLENGE_POOL_SIZE; ++i) {
        s_challengePool[i].valid = false;
        s_challengePool[i].consumed = true;
        s_challengePool[i].failedAttempts = 0;
    }
}

// Salida exclusiva a la etapa de potencia física con guarda incondicional de firmware (H-155)
static void applyPumpActuatorCommand(bool turnOn) {
    // Guarda de firmware incondicional previa a excitación (H-155):
    // Si la lectura directa de la boya indica falta de agua (HIGH), el microcontrolador niega el encendido.
    // NOTA: Esta guarda es una mitigación lógica en firmware; la validación eléctrica independiente
    // del corte físico en potencia permanece sujeta a los ensayos en banco ACT-05.
    if (turnOn && (digitalRead(PIN_FLOTADOR_AGUA) == HIGH)) {
        turnOn = false;
    }

    if (turnOn != s_orderedPumpLevel) {
        s_orderedPumpLevel = turnOn;
        digitalWrite(PIN_DRIVER_BOMBA, turnOn ? HIGH : LOW);
        invalidateAllChallenges();
    }
}

// ============================================================================
// GESTIÓN DE RETOS Y RATE LIMITING DETERMINISTA (H-152)
// ============================================================================
static IpRateLimitRecord* getOrCreateRateLimit(const IPAddress& ip, uint32_t nowMs) {
    for (size_t i = 0; i < AUTH_FAIL_CACHE_SIZE; ++i) {
        if (s_ipRateLimits[i].active && s_ipRateLimits[i].clientIp == ip) {
            s_ipRateLimits[i].lastAccessMs = nowMs;
            return &s_ipRateLimits[i];
        }
    }
    for (size_t i = 0; i < AUTH_FAIL_CACHE_SIZE; ++i) {
        if (!s_ipRateLimits[i].active) {
            s_ipRateLimits[i].clientIp = ip;
            s_ipRateLimits[i].lastAccessMs = nowMs;
            s_ipRateLimits[i].hasIssuedNonce = false;
            s_ipRateLimits[i].hasAuthFailed = false;
            s_ipRateLimits[i].lastNonceIssueMs = 0;
            s_ipRateLimits[i].lastAuthFailMs = 0;
            s_ipRateLimits[i].active = true;
            return &s_ipRateLimits[i];
        }
    }
    // Reciclaje de ranuras totalmente expiradas
    for (size_t i = 0; i < AUTH_FAIL_CACHE_SIZE; ++i) {
        const bool issueExpired = !s_ipRateLimits[i].hasIssuedNonce ||
            (static_cast<uint32_t>(nowMs - s_ipRateLimits[i].lastNonceIssueMs) >= NONCE_VALIDITY_WINDOW_MS);
        const bool failExpired = !s_ipRateLimits[i].hasAuthFailed ||
            (static_cast<uint32_t>(nowMs - s_ipRateLimits[i].lastAuthFailMs) >= AUTH_FAIL_COOLDOWN_MS);

        if (issueExpired && failExpired) {
            s_ipRateLimits[i].clientIp = ip;
            s_ipRateLimits[i].lastAccessMs = nowMs;
            s_ipRateLimits[i].hasIssuedNonce = false;
            s_ipRateLimits[i].hasAuthFailed = false;
            s_ipRateLimits[i].lastNonceIssueMs = 0;
            s_ipRateLimits[i].lastAuthFailMs = 0;
            s_ipRateLimits[i].active = true;
            return &s_ipRateLimits[i];
        }
    }
    // Desalojo LRU genuino y robusto ante desbordamiento (rollover) de millis() (H-158)
    uint32_t lastAccessList[AUTH_FAIL_CACHE_SIZE];
    bool underPenaltyList[AUTH_FAIL_CACHE_SIZE];
    for (size_t i = 0; i < AUTH_FAIL_CACHE_SIZE; ++i) {
        lastAccessList[i] = s_ipRateLimits[i].lastAccessMs;
        underPenaltyList[i] = s_ipRateLimits[i].hasAuthFailed &&
            (static_cast<uint32_t>(nowMs - s_ipRateLimits[i].lastAuthFailMs) < AUTH_FAIL_COOLDOWN_MS);
    }
    const int bestLru = selectLruCandidate(lastAccessList, underPenaltyList, AUTH_FAIL_CACHE_SIZE, nowMs);

    if (bestLru >= 0) {
        s_ipRateLimits[bestLru].clientIp = ip;
        s_ipRateLimits[bestLru].lastAccessMs = nowMs;
        s_ipRateLimits[bestLru].hasIssuedNonce = false;
        s_ipRateLimits[bestLru].hasAuthFailed = false;
        s_ipRateLimits[bestLru].lastNonceIssueMs = 0;
        s_ipRateLimits[bestLru].lastAuthFailMs = 0;
        s_ipRateLimits[bestLru].active = true;
        return &s_ipRateLimits[bestLru];
    }

    return nullptr; // Saturación total de la tabla bajo ataque DoS masivo
}

static bool isIpAuthCoolingDown(const IPAddress& ip, uint32_t nowMs) {
    for (size_t i = 0; i < AUTH_FAIL_CACHE_SIZE; ++i) {
        if (s_ipRateLimits[i].active && s_ipRateLimits[i].clientIp == ip) {
            s_ipRateLimits[i].lastAccessMs = nowMs;
            if (s_ipRateLimits[i].hasAuthFailed &&
                static_cast<uint32_t>(nowMs - s_ipRateLimits[i].lastAuthFailMs) < AUTH_FAIL_COOLDOWN_MS) {
                return true;
            }
        }
    }
    return false;
}

static void recordAuthFailure(const IPAddress& ip, uint32_t nowMs) {
    IpRateLimitRecord* rec = getOrCreateRateLimit(ip, nowMs);
    if (rec) {
        rec->hasAuthFailed = true;
        rec->lastAuthFailMs = nowMs;
    }
}

static IssueResult issueNewChallenge(const IPAddress& clientIp, uint8_t outNonce[16], uint32_t nowMs) {
    if (!isValidHexToken32(API_AUTH_TOKEN)) return ISSUE_INSECURE_TOKEN;

    IpRateLimitRecord* rateRec = getOrCreateRateLimit(clientIp, nowMs);
    if (!rateRec) return ISSUE_RATE_TABLE_FULL;

    if (rateRec->hasIssuedNonce &&
        static_cast<uint32_t>(nowMs - rateRec->lastNonceIssueMs) < PER_IP_NONCE_COOLDOWN_MS) {
        return ISSUE_IP_COOLDOWN;
    }

    size_t activeCountForIp = 0;
    for (size_t i = 0; i < CHALLENGE_POOL_SIZE; ++i) {
        if (s_challengePool[i].valid && !s_challengePool[i].consumed &&
            s_challengePool[i].clientIp == clientIp) {
            if (static_cast<uint32_t>(nowMs - s_challengePool[i].issuedAtMs) < NONCE_VALIDITY_WINDOW_MS) {
                activeCountForIp++;
            }
        }
    }
    if (activeCountForIp >= MAX_ACTIVE_PER_IP) return ISSUE_IP_QUOTA_EXCEEDED;

    int candidateIdx = -1;
    for (size_t i = 0; i < CHALLENGE_POOL_SIZE; ++i) {
        if (!s_challengePool[i].valid || s_challengePool[i].consumed ||
            static_cast<uint32_t>(nowMs - s_challengePool[i].issuedAtMs) >= NONCE_VALIDITY_WINDOW_MS) {
            candidateIdx = static_cast<int>(i);
            break;
        }
    }
    if (candidateIdx < 0) return ISSUE_POOL_EXHAUSTED;

    esp_fill_random(s_challengePool[candidateIdx].nonce, 16);
    s_challengePool[candidateIdx].clientIp = clientIp;
    s_challengePool[candidateIdx].issuedAtMs = nowMs;
    s_challengePool[candidateIdx].valid = true;
    s_challengePool[candidateIdx].consumed = false;

    memcpy(outNonce, s_challengePool[candidateIdx].nonce, 16);
    rateRec->hasIssuedNonce = true;
    rateRec->lastNonceIssueMs = nowMs;
    return ISSUE_OK;
}

// ============================================================================
// PARSER HTTP ESTRICTO Y MOTOR DE RESPUESTA (RFC 7230/9112)
// ============================================================================
static const char* getHttpStatusPhrase(int code) {
    switch (code) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 409: return "Conflict";
        case 411: return "Length Required";
        case 413: return "Payload Too Large";
        case 429: return "Too Many Requests";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 503: return "Service Unavailable";
        default:  return "Unknown";
    }
}

static void queueResponse(int code, const char* contentType, const char* body,
                          bool isProgmem = false, const char* extraHeaders = nullptr) {
    s_isBodyProgmem = isProgmem;
    s_txBodyPtr = body;
    s_txBodyLen = isProgmem ? strlen_P(body) : strlen(body);
    s_txBodySent = 0;

    if (extraHeaders) {
        s_txHdrLen = snprintf(s_txHdrBuf, sizeof(s_txHdrBuf),
                              "HTTP/1.1 %d %s\r\n"
                              "Content-Type: %s\r\n"
                              "Content-Length: %u\r\n"
                              "%s\r\n"
                              "Connection: close\r\n\r\n",
                              code, getHttpStatusPhrase(code), contentType,
                              static_cast<unsigned int>(s_txBodyLen), extraHeaders);
    } else {
        s_txHdrLen = snprintf(s_txHdrBuf, sizeof(s_txHdrBuf),
                              "HTTP/1.1 %d %s\r\n"
                              "Content-Type: %s\r\n"
                              "Content-Length: %u\r\n"
                              "Connection: close\r\n\r\n",
                              code, getHttpStatusPhrase(code), contentType,
                              static_cast<unsigned int>(s_txBodyLen));
    }
    s_txHdrSent = 0;
    s_connState = HTTP_STATE_SENDING_HEADERS;
}

static void resetHttpState() {
    s_connState = HTTP_STATE_IDLE;
    s_hdrLen = 0;
    s_bodyLen = 0;
    s_contentLen = -1;
    s_uri[0] = '\0';
    s_isGet = false;
    s_isPost = false;
    s_txHdrLen = 0;
    s_txHdrSent = 0;
    s_txBodyLen = 0;
    s_txBodySent = 0;
    s_txBodyPtr = nullptr;
    s_isBodyProgmem = false;
}

// ============================================================================
// ENDPOINTS DE LA API REST
// ============================================================================
static void serveApiStatus() {
    char json[128];
    snprintf(json, sizeof(json),
             "{\"moisture\":%u,\"waterLevel\":%s,\"pumpActive\":%s,\"sensorOk\":%s}",
             currentMoisturePct,
             waterLevelOk ? "true" : "false",
             controller.isPumpActive() ? "true" : "false",
             (sensorStatus == FloraCare::SensorHealth::OK) ? "true" : "false");
    queueResponse(200, "application/json", json);
}

static void serveApiNonce() {
    if (!isValidHexToken32(API_AUTH_TOKEN)) {
        queueResponse(403, "application/json",
            "{\"status\":\"error\",\"message\":\"Remote watering disabled: unconfigured auth token\"}");
        return;
    }

    const IPAddress clientIp = s_client.remoteIP();
    const uint32_t nowMs = millis();

    if (isIpAuthCoolingDown(clientIp, nowMs)) {
        queueResponse(429, "application/json",
            "{\"status\":\"error\",\"message\":\"Rate limited: cooldown active\"}",
            false, "Retry-After: 3");
        return;
    }

    uint8_t nonce[16];
    const IssueResult res = issueNewChallenge(clientIp, nonce, nowMs);

    switch (res) {
        case ISSUE_OK: {
            char nonceHex[33] = {0};
            for (size_t i = 0; i < 16; ++i) sprintf(nonceHex + (i * 2), "%02x", nonce[i]);
            char resp[96];
            snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"nonce\":\"%s\"}", nonceHex);
            queueResponse(200, "application/json", resp);
            break;
        }
        case ISSUE_IP_COOLDOWN:
            queueResponse(429, "application/json",
                "{\"status\":\"error\",\"message\":\"Nonce cooldown active (500ms)\"}",
                false, "Retry-After: 1");
            break;
        case ISSUE_IP_QUOTA_EXCEEDED:
            queueResponse(429, "application/json",
                "{\"status\":\"error\",\"message\":\"Max active nonces reached for IP\"}",
                false, "Retry-After: 5");
            break;
        case ISSUE_POOL_EXHAUSTED:
        case ISSUE_RATE_TABLE_FULL:
            queueResponse(429, "application/json",
                "{\"status\":\"error\",\"message\":\"Challenge pool or rate table saturated\"}",
                false, "Retry-After: 3");
            break;
        case ISSUE_INSECURE_TOKEN:
            queueResponse(403, "application/json",
                "{\"status\":\"error\",\"message\":\"Invalid server token\"}");
            break;
    }
}

static void serveApiWater(const char* body, size_t bodyLen,
                          bool currentWaterOk, FloraCare::SensorHealth currentSensorStatus,
                          uint8_t currentMoisture) {
    const IPAddress clientIp = s_client.remoteIP();
    const uint32_t nowMs = millis();

    if (isIpAuthCoolingDown(clientIp, nowMs)) {
        queueResponse(429, "application/json",
            "{\"status\":\"error\",\"message\":\"Auth failure cooldown active\"}",
            false, "Retry-After: 3");
        return;
    }

    // Parser JSON estricto
    const char* nonceKey = "\"nonce\":\"";
    const char* hmacKey  = "\"hmac\":\"";
    const char* pNonce = strstr(body, nonceKey);
    const char* pHmac  = strstr(body, hmacKey);

    if (!pNonce || !pHmac) {
        recordAuthFailure(clientIp, nowMs);
        queueResponse(400, "application/json", "{\"status\":\"error\",\"message\":\"Malformed JSON payload\"}");
        return;
    }

    pNonce += strlen(nonceKey);
    pHmac  += strlen(hmacKey);
    const char* nonceEnd = strchr(pNonce, '\"');
    const char* hmacEnd  = strchr(pHmac, '\"');

    if (!nonceEnd || !hmacEnd || (nonceEnd - pNonce != 32) || (hmacEnd - pHmac != 64)) {
        recordAuthFailure(clientIp, nowMs);
        queueResponse(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid field lengths\"}");
        return;
    }

    // Validación sintáctica estricta de caracteres hexadecimales antes de conversión (H-157)
    if (!isHexStrict(pNonce, 32) || !isHexStrict(pHmac, 64)) {
        recordAuthFailure(clientIp, nowMs);
        queueResponse(400, "application/json", "{\"status\":\"error\",\"message\":\"Non-hexadecimal characters in security fields\"}");
        return;
    }

    uint8_t reqNonce[16];
    for (size_t i = 0; i < 16; ++i) {
        char byteStr[3] = { pNonce[i*2], pNonce[i*2+1], '\0' };
        reqNonce[i] = static_cast<uint8_t>(strtoul(byteStr, nullptr, 16));
    }

    int slotIdx = -1;
    for (size_t i = 0; i < CHALLENGE_POOL_SIZE; ++i) {
        if (s_challengePool[i].valid && !s_challengePool[i].consumed &&
            s_challengePool[i].clientIp == clientIp) {
            if (safeConstantTimeEquals(s_challengePool[i].nonce, reqNonce, 16)) {
                if (static_cast<uint32_t>(nowMs - s_challengePool[i].issuedAtMs) < NONCE_VALIDITY_WINDOW_MS) {
                    slotIdx = static_cast<int>(i);
                }
                break;
            }
        }
    }

    if (slotIdx < 0) {
        recordAuthFailure(clientIp, nowMs);
        queueResponse(403, "application/json", "{\"status\":\"error\",\"message\":\"Invalid or expired challenge\"}");
        return;
    }

    // Conversión de clave secreta
    uint8_t binaryKey[16];
    for (size_t i = 0; i < 16; ++i) {
        char byteStr[3] = { API_AUTH_TOKEN[i*2], API_AUTH_TOKEN[i*2+1], '\0' };
        binaryKey[i] = static_cast<uint8_t>(strtoul(byteStr, nullptr, 16));
    }

    uint8_t expectedHmac[32];
    if (!computeHmacSha256(binaryKey, 16, reqNonce, 16, expectedHmac)) {
        queueResponse(500, "application/json", "{\"status\":\"error\",\"message\":\"Crypto engine fault\"}");
        return;
    }

    uint8_t receivedHmac[32];
    for (size_t i = 0; i < 32; ++i) {
        char byteStr[3] = { pHmac[i*2], pHmac[i*2+1], '\0' };
        receivedHmac[i] = static_cast<uint8_t>(strtoul(byteStr, nullptr, 16));
    }

    if (!safeConstantTimeEquals(expectedHmac, receivedHmac, 32)) {
        s_challengePool[slotIdx].failedAttempts++;
        if (s_challengePool[slotIdx].failedAttempts >= 2) {
            s_challengePool[slotIdx].consumed = true;
            s_challengePool[slotIdx].valid = false;
        }
        recordAuthFailure(clientIp, nowMs);
        queueResponse(403, "application/json", "{\"status\":\"error\",\"message\":\"HMAC signature mismatch\"}");
        return;
    }

    // HMAC verificado con éxito: consumo atómico e irreversible del desafío (H-157)
    s_challengePool[slotIdx].consumed = true;
    s_challengePool[slotIdx].valid = false;

    // Solicitud a la FSM centralizada
    const bool accepted = controller.requestWatering(
        FloraCare::IrrigationSource::REMOTE_WEB,
        nowMs,
        currentWaterOk,
        currentSensorStatus,
        currentMoisture
    );

    if (accepted) {
        queueResponse(200, "application/json", "{\"status\":\"ok\",\"message\":\"Remote watering activated (3s)\"}");
    } else {
        queueResponse(409, "application/json", "{\"status\":\"error\",\"message\":\"Watering rejected by safety FSM\"}");
    }
}

// ============================================================================
// SERVIDOR HTTP COOPERATIVO CON DEADLINE ESTRICTO DE 150 MS (H-151, H-153)
// ============================================================================
static void pollHttpServer(bool currentWaterOk, FloraCare::SensorHealth currentSensorStatus, uint8_t currentMoisture) {
    const uint32_t nowMs = millis();

    // Verificación de timeout global de la transacción
    if (s_connState != HTTP_STATE_IDLE) {
        if (static_cast<uint32_t>(nowMs - s_connStartMs) >= 150) {
            s_client.stop();
            resetHttpState();
            return;
        }
    }

    switch (s_connState) {
        case HTTP_STATE_IDLE: {
            // NetworkServer::begin() configura O_NONBLOCK en el socket de escucha (NetworkServer.cpp:127).
            // hasClient() y accept() operan en modo no bloqueante sin retener la CPU.
            if (s_httpServer.hasClient()) {
                s_client = s_httpServer.accept();
                if (s_client) {
                    const int fd = s_client.fd();
                    if (fd >= 0) {
                        const int flags = fcntl(fd, F_GETFL, 0);
                        if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
                            s_client.stop();
                            return;
                        }
                    }
                    s_connStartMs = nowMs;
                    s_hdrLen = 0;
                    s_bodyLen = 0;
                    s_contentLen = -1;
                    s_connState = HTTP_STATE_READ_HEADERS;
                }
            }
            break;
        }

        case HTTP_STATE_READ_HEADERS: {
            if (!s_client.connected()) {
                s_client.stop();
                resetHttpState();
                return;
            }

            int avail = s_client.available();
            if (avail > 0) {
                size_t toRead = static_cast<size_t>(avail);
                if (toRead > 256) toRead = 256; // Rodaja acotada para no demorar el lazo de control (H-160)
                if (s_hdrLen + toRead >= sizeof(s_rxHdrBuf)) {
                    queueResponse(413, "application/json", "{\"status\":\"error\",\"message\":\"Headers Too Large\"}");
                    return;
                }

                int n = s_client.read(reinterpret_cast<uint8_t*>(s_rxHdrBuf + s_hdrLen), toRead);
                if (n > 0) {
                    s_hdrLen += n;
                    s_rxHdrBuf[s_hdrLen] = '\0';
                }

                const char* hdrEnd = strstr(s_rxHdrBuf, "\r\n\r\n");
                if (hdrEnd) {
                    const HttpParseResult res = parseHttpHeaderStrict(
                        s_rxHdrBuf, s_hdrLen, s_uri, sizeof(s_uri), s_isGet, s_isPost, s_contentLen
                    );

                    switch (res) {
                        case PARSE_OK:
                            break;
                        case PARSE_METHOD_NOT_ALLOWED:
                            queueResponse(405, "application/json", "{\"status\":\"error\",\"message\":\"Method Not Allowed\"}");
                            return;
                        case PARSE_BAD_URI:
                        case PARSE_BAD_REQUEST:
                            queueResponse(400, "application/json", "{\"status\":\"error\",\"message\":\"Bad Request\"}");
                            return;
                        case PARSE_PAYLOAD_TOO_LARGE:
                            queueResponse(413, "application/json", "{\"status\":\"error\",\"message\":\"Payload Too Large\"}");
                            return;
                        case PARSE_NOT_IMPLEMENTED:
                            queueResponse(501, "application/json", "{\"status\":\"error\",\"message\":\"Not Implemented (Transfer-Encoding not supported)\"}");
                            return;
                    }

                    if (s_isGet) {
                        if (strcmp(s_uri, "/") == 0) {
                            queueResponse(200, "text/html", HTML_PAGE, true);
                        } else if (strcmp(s_uri, "/api/status") == 0) {
                            serveApiStatus();
                        } else if (strcmp(s_uri, "/api/nonce") == 0) {
                            serveApiNonce();
                        } else {
                            queueResponse(404, "application/json", "{\"status\":\"error\",\"message\":\"Not Found\"}");
                        }
                    } else if (s_isPost) {
                        if (strcmp(s_uri, "/api/water") != 0) {
                            queueResponse(404, "application/json", "{\"status\":\"error\",\"message\":\"Endpoint Not Found\"}");
                            return;
                        }
                        if (s_contentLen <= 0) {
                            queueResponse(411, "application/json", "{\"status\":\"error\",\"message\":\"Length Required\"}");
                            return;
                        }

                        // Preservar bytes residuales del cuerpo recibidos junto con la cabecera
                        const char* bodyStart = hdrEnd + 4;
                        size_t residual = (s_rxHdrBuf + s_hdrLen) - bodyStart;
                        s_bodyLen = 0;
                        if (residual > 0) {
                            if (!isResidualBodyValid(residual, s_contentLen)) {
                                queueResponse(400, "application/json", "{\"status\":\"error\",\"message\":\"Residual body exceeds Content-Length\"}");
                                return;
                            }
                            if (residual > sizeof(s_rxBodyBuf) - 1) residual = sizeof(s_rxBodyBuf) - 1;
                            memcpy(s_rxBodyBuf, bodyStart, residual);
                            s_bodyLen = residual;
                            s_rxBodyBuf[s_bodyLen] = '\0';
                        }

                        if (s_bodyLen == static_cast<size_t>(s_contentLen)) {
                            serveApiWater(s_rxBodyBuf, s_bodyLen, currentWaterOk, currentSensorStatus, currentMoisture);
                            return;
                        }
                        s_connState = HTTP_STATE_READ_BODY;
                    }
                }
            }
            break;
        }

        case HTTP_STATE_READ_BODY: {
            if (!s_client.connected()) {
                s_client.stop();
                resetHttpState();
                return;
            }

            int avail = s_client.available();
            if (avail > 0) {
                size_t needed = static_cast<size_t>(s_contentLen) - s_bodyLen;
                size_t toRead = (static_cast<size_t>(avail) < needed) ? static_cast<size_t>(avail) : needed;
                if (toRead > 256) toRead = 256; // Rodaja acotada para no demorar el lazo de control (H-160)

                if (s_bodyLen + toRead >= sizeof(s_rxBodyBuf)) {
                    queueResponse(413, "application/json", "{\"status\":\"error\",\"message\":\"Body Too Large\"}");
                    return;
                }

                int n = s_client.read(reinterpret_cast<uint8_t*>(s_rxBodyBuf + s_bodyLen), toRead);
                if (n > 0) {
                    s_bodyLen += n;
                    s_rxBodyBuf[s_bodyLen] = '\0';
                }

                if (s_bodyLen == static_cast<size_t>(s_contentLen)) {
                    serveApiWater(s_rxBodyBuf, s_bodyLen, currentWaterOk, currentSensorStatus, currentMoisture);
                }
            }
            break;
        }

        case HTTP_STATE_SENDING_HEADERS: {
            if (s_txHdrSent < s_txHdrLen) {
                size_t chunk = s_txHdrLen - s_txHdrSent;
                if (chunk > 256) chunk = 256;
                ssize_t sent = lwip_send(s_client.fd(), s_txHdrBuf + s_txHdrSent, chunk, MSG_DONTWAIT);
                if (sent > 0) {
                    s_txHdrSent += sent;
                } else if (sent < 0 && (errno != EWOULDBLOCK && errno != EAGAIN)) {
                    s_client.stop();
                    resetHttpState();
                    return;
                }
            }

            if (s_txHdrSent >= s_txHdrLen) {
                if (s_txBodyLen == 0) {
                    s_connState = HTTP_STATE_CLOSE;
                } else {
                    s_connState = HTTP_STATE_SENDING_BODY;
                }
            }
            break;
        }

        case HTTP_STATE_SENDING_BODY: {
            if (s_txBodySent < s_txBodyLen) {
                size_t chunk = s_txBodyLen - s_txBodySent;
                if (chunk > sizeof(s_txChunkBuf)) chunk = sizeof(s_txChunkBuf);

                const char* sendPtr = nullptr;
                if (s_isBodyProgmem) {
                    memcpy_P(s_txChunkBuf, s_txBodyPtr + s_txBodySent, chunk);
                    sendPtr = s_txChunkBuf;
                } else {
                    sendPtr = s_txBodyPtr + s_txBodySent;
                }

                ssize_t sent = lwip_send(s_client.fd(), sendPtr, chunk, MSG_DONTWAIT);
                if (sent > 0) {
                    s_txBodySent += sent;
                } else if (sent < 0 && (errno != EWOULDBLOCK && errno != EAGAIN)) {
                    s_client.stop();
                    resetHttpState();
                    return;
                }
            }

            if (s_txBodySent >= s_txBodyLen) {
                s_connState = HTTP_STATE_CLOSE;
            }
            break;
        }

        case HTTP_STATE_CLOSE: {
            s_client.stop();
            resetHttpState();
            break;
        }
    }
}

// ============================================================================
// FUNCIONES AUXILIARES DE SENSOR, DISPLAY Y AUDITORÍA
// ============================================================================
static uint16_t sampleAnalogFast() {
    // Lectura directa no bloqueante sin retardos síncronos (H-159):
    // Conversión inmediata de disparo único con filtro exponencial paso bajo IIR en punto fijo:
    // y[n] = (3 * y[n-1] + x[n]) / 4 (constante de tiempo exacta tau = -Ts / ln(0.75) aprox. 348 ms
    // con Ts = 100 ms; frecuencia de corte aprox. 0.46 Hz).
    // NOTA: La latencia real de conversión del SAR ADC (~10 us nominal) y la dinámica temporal del filtro
    // son un modelo de diseño de firmware sujeto a caracterización oscilográfica en banco (ACT-05).
    static uint16_t s_filteredAdc = 0;
    static bool s_initialized = false;
    const uint16_t raw = static_cast<uint16_t>(analogRead(PIN_SENSOR_HUMEDAD));
    if (!s_initialized) {
        s_filteredAdc = raw;
        s_initialized = true;
    } else {
        s_filteredAdc = static_cast<uint16_t>(((static_cast<uint32_t>(s_filteredAdc) * 3) + raw) / 4);
    }
    return s_filteredAdc;
}

static void updateDisplay(const char* statusMsg) {
    if (!oledAvailable) return;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print(F("FLORACARE PRO (ESP32)"));
    display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

    display.setTextSize(3);
    display.setCursor(6, 16);
    if (sensorStatus == FloraCare::SensorHealth::OK) {
        display.print(currentMoisturePct);
        display.print(F("%"));
    } else {
        display.print(F("ERR"));
    }

    display.setTextSize(1);
    display.setCursor(80, 20);
    display.print(waterLevelOk ? F("[AGUA OK]") : F("[VACIO!]"));

    display.drawRect(0, 44, 128, 6, SSD1306_WHITE);
    if (sensorStatus == FloraCare::SensorHealth::OK) {
        uint8_t bar = static_cast<uint8_t>(map(currentMoisturePct, 0, 100, 0, 128));
        display.fillRect(0, 44, bar, 6, SSD1306_WHITE);
    }

    display.setCursor(0, 54);
    display.print(F("ESTADO: "));
    display.print(statusMsg);
    display.display();
}

// ============================================================================
// SETUP Y CONFIGURACIÓN INICIAL DEL SISTEMA
// ============================================================================
void setup() {
    // 1. INICIALIZACIÓN INMEDIATA DEL ACTUADOR A ESTADO SEGURO (H-150)
    pinMode(PIN_DRIVER_BOMBA, OUTPUT);
    digitalWrite(PIN_DRIVER_BOMBA, LOW);
    s_orderedPumpLevel = false;

    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);

    // 2. CONFIGURACIÓN DE ENTRADAS DIGITALES (H-148)
    pinMode(PIN_FLOTADOR_AGUA, INPUT_PULLUP);
    pinMode(PIN_BOTON_MANUAL, INPUT_PULLUP);

    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);

    Serial.begin(115200);
    Serial.println(F("\n[INIT] Inicializando Smart FloraCare v1.2.1 (V4.18)..."));

    // 3. WATCHDOG DE TAREA (TWDT) A 4000 MS
    esp_task_wdt_config_t twdt_cfg = {
        .timeout_ms = 4000,
        .idle_core_mask = (1 << 0),
        .trigger_panic = true
    };
    esp_task_wdt_init(&twdt_cfg);
    esp_task_wdt_add(NULL);

    // 4. AUTO-VERIFICACIÓN CRIPTOGRÁFICA EN BOOT (RFC 4231)
    if (!runCryptoSelfTest()) {
        Serial.println(F("[ERROR CRÍTICO] Falló el test criptográfico HMAC-SHA256. Sistema detenido."));
        while (true) {
            digitalWrite(PIN_DRIVER_BOMBA, LOW);
            delay(100);
        }
    }
    Serial.println(F("[OK] Vector normativo HMAC-SHA256 verificado exitosamente."));

    // 5. INICIALIZACIÓN DE DISPLAY OLED
    Wire.begin(21, 22);
    Wire.setClock(400000);
    if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        oledAvailable = true;
        display.clearDisplay();
        display.display();
    }

    // 6. INICIALIZACIÓN DE TABLAS DE SEGURIDAD
    for (size_t i = 0; i < CHALLENGE_POOL_SIZE; ++i) {
        s_challengePool[i].valid = false;
        s_challengePool[i].consumed = true;
        s_challengePool[i].failedAttempts = 0;
    }
    for (size_t i = 0; i < AUTH_FAIL_CACHE_SIZE; ++i) {
        s_ipRateLimits[i].active = false;
        s_ipRateLimits[i].lastAccessMs = 0;
    }

    // 7. CONEXIÓN WI-FI Y SERVIDOR HTTP
    if (WIFI_ENABLED && strlen(WIFI_SSID) > 0) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        Serial.print(F("[WIFI] Conectando"));
        uint32_t wifiStart = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - wifiStart < 8000)) {
            esp_task_wdt_reset();
            delay(250);
            Serial.print('.');
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            Serial.print(F("[WIFI] Conectado. IP: "));
            Serial.println(WiFi.localIP());
            s_httpServer.begin(80);
            Serial.println(F("[HTTP] Servidor cooperativo iniciado en puerto 80."));
        } else {
            Serial.println(F("[WIFI] No conectado. Modo autónomo local activo."));
        }
    } else {
        WiFi.mode(WIFI_OFF);
        Serial.println(F("[WIFI] Deshabilitado: sin credenciales configuradas (modo autónomo)"));
    }

    // 8. PRIMERA LECTURA SENSORIAL
    const uint16_t initialRaw = sampleAnalogFast();
    FloraCare::SensorValidator::calculateMoisturePercent(
        initialRaw, ESP32_CALIBRATION, currentMoisturePct, sensorStatus
    );
    waterLevelOk = (digitalRead(PIN_FLOTADOR_AGUA) == LOW);

    updateDisplay("INICIO OK");
    tone(PIN_BUZZER, 1000, 100);
    esp_task_wdt_reset();
}

// ============================================================================
// BUCLE PRINCIPAL DE EJECUCIÓN (LOOP DE 3 FASES PRIORIZADAS)
// ============================================================================
void loop() {
    esp_task_wdt_reset();
    const uint32_t nowMs = millis();

    // ------------------------------------------------------------------------
    // FASE 1: SEGURIDAD FÍSICA INMEDIATA Y CORTE BRUTO DE NIVEL (H-149)
    // ------------------------------------------------------------------------
    const bool rawWaterOk = (digitalRead(PIN_FLOTADOR_AGUA) == LOW);
    if (!rawWaterOk) {
        // Orden de corte de seguridad en firmware sobre el actuador
        applyPumpActuatorCommand(false);
        controller.stop(FloraCare::StopReason::WATER_DEPLETED, nowMs);
    }

    // Filtro antirrebote e integrador temporal de boya (REQ-07, ACT-05.3a)
    static FloraCare::FloatSwitchDebouncer floatDebouncer(digitalRead(PIN_FLOTADOR_AGUA) == LOW);
    waterLevelOk = floatDebouncer.update(rawWaterOk, nowMs);

    // Nivel consolidado de agua ultra-seguro (H-155):
    // Si la lectura física bruta cae, se propaga inmediatamente sin esperar el debounce
    const bool safeWaterLevel = rawWaterOk && waterLevelOk;

    // Lectura periódica y acotada del sensor analógico de suelo (H-159)
    if (nowMs - lastSampleTimeMs >= 100) {
        lastSampleTimeMs = nowMs;
        const uint16_t rawAdc = sampleAnalogFast();
        FloraCare::SensorValidator::calculateMoisturePercent(
            rawAdc, ESP32_CALIBRATION, currentMoisturePct, sensorStatus
        );

        // Si el sensor analógico falla, se comanda el apagado universal
        if (sensorStatus != FloraCare::SensorHealth::OK && controller.isPumpActive()) {
            applyPumpActuatorCommand(false);
            controller.stop(FloraCare::StopReason::SENSOR_FAULT, nowMs);
        }
    }

    // ------------------------------------------------------------------------
    // FASE 2: GESTIÓN DE ENTRADAS LOCALES Y CONTROLADOR DE DOMINIO FSM (H-156)
    // ------------------------------------------------------------------------
    // Intento explícito de recuperación condicionada ante retorno a la normalidad
    if (controller.getState() == FloraCare::ControllerState::FAULT) {
        controller.attemptRecovery(nowMs, safeWaterLevel, sensorStatus);
    }

    // Activación autónoma periódica según umbral de humedad de suelo (H-156)
    // Si la humedad se normaliza por encima del umbral, se libera la inhibición preventiva
    if (currentMoisturePct > IRRIGATION_CFG.autoStartMoisturePct) {
        s_autoInhibitAfterTimeout = false;
    }

    if (!controller.isPumpActive() && safeWaterLevel && sensorStatus == FloraCare::SensorHealth::OK) {
        if (!s_autoInhibitAfterTimeout && currentMoisturePct <= IRRIGATION_CFG.autoStartMoisturePct) {
            controller.requestWatering(
                FloraCare::IrrigationSource::AUTO_CYCLE,
                nowMs,
                safeWaterLevel,
                sensorStatus,
                currentMoisturePct
            );
        }
    }

    // Gestión del botón físico manual
    const bool currentBtnReading = digitalRead(PIN_BOTON_MANUAL);
    if (currentBtnReading != lastButtonState) {
        lastButtonDebounceMs = nowMs;
        lastButtonState = currentBtnReading;
    }

    if ((nowMs - lastButtonDebounceMs) > 50) {
        static bool s_buttonStableState = HIGH;
        if (currentBtnReading != s_buttonStableState) {
            s_buttonStableState = currentBtnReading;
            if (s_buttonStableState == LOW) { // Pulsación activa a tierra
                if (controller.getState() == FloraCare::ControllerState::FAULT) {
                    // Procedimiento explícito de rearme local ante fallos bloqueantes (ej. MAX_TIMEOUT_EXCEEDED) (H-156)
                    const bool rearmed = controller.resetFault(nowMs, safeWaterLevel, sensorStatus);
                    if (rearmed) {
                        tone(PIN_BUZZER, 1200, 100);
                    }
                } else {
                    // Toda intervención manual intencionada libera la inhibición de rearme
                    s_autoInhibitAfterTimeout = false;
                    controller.requestWatering(
                        FloraCare::IrrigationSource::MANUAL_BUTTON,
                        nowMs,
                        safeWaterLevel,
                        sensorStatus,
                        currentMoisturePct
                    );
                }
            }
        }
    }

    // Actualización de la FSM y mando exclusivo al actuador
    const bool pumpShouldRun = controller.update(nowMs, safeWaterLevel, sensorStatus, currentMoisturePct);
    applyPumpActuatorCommand(pumpShouldRun);

    // Si se disparó el watchdog de tiempo máximo, registrar inhibición para impedir re-arranque desatendido
    if (controller.getLastStopReason() == FloraCare::StopReason::SAFETY_TIMEOUT ||
        controller.getLastStopReason() == FloraCare::StopReason::AUTO_MAX_DURATION_REACHED) {
        s_autoInhibitAfterTimeout = true;
    }

    // Alarma acústica no bloqueante ante tanque vacío
    if (!safeWaterLevel) {
        if (nowMs - lastBeepTimeMs >= 3000) {
            lastBeepTimeMs = nowMs;
            tone(PIN_BUZZER, 850, 100);
        }
    }

    // ------------------------------------------------------------------------
    // FASE 3: PRESENTACIÓN OLED Y SERVIDOR HTTP COOPERATIVO (H-151, H-153)
    // ------------------------------------------------------------------------
    if (nowMs - lastDisplayTimeMs >= 250) {
        lastDisplayTimeMs = nowMs;
        const char* msg = "REPOSO";
        if (controller.isPumpActive()) {
            msg = (controller.getActiveSource() == FloraCare::IrrigationSource::AUTO_CYCLE) ? "RIEGO AUTO" : "RIEGO MANUAL";
        } else if (!safeWaterLevel) {
            msg = "SIN AGUA";
        } else if (sensorStatus != FloraCare::SensorHealth::OK) {
            msg = "ERR SENSOR";
        }
        updateDisplay(msg);
    }

    // Sondeo de la pila de red (acotada internamente a <= 150 ms de deadline)
    if (WiFi.status() == WL_CONNECTED) {
        pollHttpServer(safeWaterLevel, sensorStatus, currentMoisturePct);
    }

    // Chequeo de seguridad de cierre de ciclo
    if (digitalRead(PIN_FLOTADOR_AGUA) == HIGH && s_orderedPumpLevel) {
        applyPumpActuatorCommand(false);
    }
}
