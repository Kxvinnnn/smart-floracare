/**
 * ============================================================================
 * SMART FLORACARE - HTTP & SECURITY UTILITIES (PARSER Y VALIDADORES PUROS)
 * ============================================================================
 * Archivo: include/HttpSecurityParser.h
 * Descripción: Funciones puras, desacopladas y 100% acotadas por longitud
 *              para validación sintáctica de peticiones HTTP (RFC 7230/9112),
 *              verificación de campos de seguridad (HMAC/Nonce) y política LRU.
 *
 * Garantías de Seguridad de Memoria (H-160):
 *   - No utiliza funciones de C dependientes de terminador NUL (strstr, strchr,
 *     strtol, strncmp). Todas las búsquedas están estrictamente acotadas por
 *     el puntero de límite superior (buf + len).
 *   - Gramática RFC 7230 §3.1.1 estricta: request-line = method SP target SP version CRLF
 *     Rechaza espacios, tabuladores o caracteres espurios tras HTTP-version.
 *
 * Utilizado de forma idéntica en:
 *   - src/esp32_mejorado_wifi/regador_esp32.cpp (Firmware de producción ESP32)
 *   - tests/test_native_controller.cpp (Harness de pruebas nativas C++ en CI)
 * ============================================================================
 */

#ifndef HTTP_SECURITY_PARSER_H
#define HTTP_SECURITY_PARSER_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace FloraCare {

/**
 * Búsqueda acotada de la secuencia "\r\n" dentro del rango [start, end).
 * Jamás lee más allá de end.
 */
inline const char* findCrlfBounded(const char* start, const char* end) {
    if (!start || !end || start >= end) return nullptr;
    for (const char* p = start; p + 1 < end; ++p) {
        if (p[0] == '\r' && p[1] == '\n') {
            return p;
        }
    }
    return nullptr;
}

/**
 * Búsqueda acotada de un carácter específico dentro del rango [start, end).
 */
inline const char* findCharBounded(const char* start, const char* end, char target) {
    if (!start || !end || start >= end) return nullptr;
    for (const char* p = start; p < end; ++p) {
        if (*p == target) {
            return p;
        }
    }
    return nullptr;
}

/**
 * Comparación insensible a mayúsculas/minúsculas acotada por [start, end).
 */
inline bool caseInsensitiveStartsWithBounded(const char* start, const char* end, const char* prefix) {
    if (!start || !end || !prefix) return false;
    const char* p = start;
    while (*prefix) {
        if (p >= end) return false;
        char c1 = *p++;
        char c2 = *prefix++;
        if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
        if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');
        if (c1 != c2) return false;
    }
    return true;
}

/**
 * Comparación exacta de longitud fija N dentro de [start, end).
 */
inline bool matchExactBounded(const char* start, const char* end, const char* exact, size_t n) {
    if (!start || !end || !exact) return false;
    if (static_cast<size_t>(end - start) < n) return false;
    return (memcmp(start, exact, n) == 0);
}

/**
 * Conversor decimal acotado para gramática 1*DIGIT (RFC 7230 §3.3.2 / RFC 9110 §8.6).
 * Rechaza estrictamente '+', '-' o caracteres no numéricos.
 */
inline bool parseBoundedDecimal(const char* start, const char* end, int& outVal) {
    outVal = -1;
    if (!start || !end || start >= end) return false;
    long val = 0;
    for (const char* p = start; p < end; ++p) {
        const char c = *p;
        if (c < '0' || c > '9') return false;
        val = val * 10 + (c - '0');
        if (val > 100000) return false; // Prevenir desbordamiento
    }
    outVal = static_cast<int>(val);
    return true;
}

/**
 * Validación sintáctica estricta de cadenas hexadecimales (H-157).
 * Retorna true únicamente si cada carácter pertenece al alfabeto [0-9a-fA-F].
 */
inline bool isHexStrict(const char* s, size_t len) {
    if (!s) return false;
    for (size_t i = 0; i < len; ++i) {
        const char c = s[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            return false;
        }
    }
    return true;
}

enum HttpParseResult {
    PARSE_OK,
    PARSE_METHOD_NOT_ALLOWED,
    PARSE_BAD_URI,
    PARSE_BAD_REQUEST,
    PARSE_PAYLOAD_TOO_LARGE,
    PARSE_NOT_IMPLEMENTED
};

/**
 * Parser estricto y acotado por memoria de cabeceras HTTP (RFC 7230 / RFC 9112) (H-160).
 * - 100% acotado por buf + len (sin dependencias de NUL terminator ni desbordamiento de búfer).
 * - Exige verbos GET o POST.
 * - Exige gramática estricta de request-line sin espacios finales antes de CRLF.
 * - Rechaza explícitamente Transfer-Encoding (chunked no soportado en API embebida -> 501).
 * - Rechaza múltiples cabeceras Content-Length (RFC 7230 §3.3.2 -> 400).
 * - Exige gramática Content-Length = 1*DIGIT (rechaza estrictamente '+' o '-' -> 400).
 * - Limita la longitud máxima admisible a 256 bytes (-> 413).
 */
inline HttpParseResult parseHttpHeaderStrict(const char* buf, size_t len,
                                             char* outUri, size_t outUriMax,
                                             bool& outIsGet, bool& outIsPost,
                                             int& outContentLen) {
    outIsGet = false;
    outIsPost = false;
    outContentLen = -1;
    if (!buf || len == 0 || !outUri || outUriMax == 0) return PARSE_BAD_REQUEST;
    outUri[0] = '\0';

    const char* const bufEnd = buf + len;

    // 1. Delimitar la request-line por CRLF dentro del rango acotado
    const char* const lineEnd = findCrlfBounded(buf, bufEnd);
    if (!lineEnd) return PARSE_BAD_REQUEST;

    const char* p = buf;
    if (matchExactBounded(p, lineEnd, "GET ", 4)) {
        outIsGet = true;
        p += 4;
    } else if (matchExactBounded(p, lineEnd, "POST ", 5)) {
        outIsPost = true;
        p += 5;
    } else {
        return PARSE_METHOD_NOT_ALLOWED;
    }

    // 2. Localizar espacio SP que separa request-target de HTTP-version
    const char* const space = findCharBounded(p, lineEnd, ' ');
    if (!space) return PARSE_BAD_REQUEST;

    const size_t uriLen = static_cast<size_t>(space - p);
    if (uriLen == 0 || uriLen >= outUriMax) return PARSE_BAD_URI;
    memcpy(outUri, p, uriLen);
    outUri[uriLen] = '\0';

    // 3. HTTP-version (RFC 7230 §3.1.1):
    // request-line = method SP request-target SP HTTP-version CRLF
    // Prohibido cualquier espacio final, tabulador o sufijo tras HTTP-version:
    // lineEnd debe encontrarse EXACTAMENTE 8 bytes después de space
    p = space + 1;
    const size_t verLen = static_cast<size_t>(lineEnd - p);
    if (verLen != 8) {
        return PARSE_BAD_REQUEST; // Rechaza espacios finales ("HTTP/1.1 ") o sufijos ("HTTP/1.1foo")
    }
    if (memcmp(p, "HTTP/1.1", 8) != 0 && memcmp(p, "HTTP/1.0", 8) != 0) {
        return PARSE_BAD_REQUEST;
    }

    // 4. Bucle de cabeceras estrictamente acotado por bufEnd
    const char* current = lineEnd + 2;
    while (current < bufEnd) {
        const char* const nextEnd = findCrlfBounded(current, bufEnd);
        if (!nextEnd) break;
        if (nextEnd == current) break; // Fin de cabeceras (\r\n\r\n)

        // Rechazo explícito de Transfer-Encoding
        if (caseInsensitiveStartsWithBounded(current, nextEnd, "Transfer-Encoding:")) {
            return PARSE_NOT_IMPLEMENTED;
        }

        // Validación estricta de Content-Length
        if (caseInsensitiveStartsWithBounded(current, nextEnd, "Content-Length:")) {
            if (outContentLen != -1) {
                return PARSE_BAD_REQUEST; // Prohibidas múltiples cabeceras Content-Length (RFC 7230 §3.3.2)
            }
            const char* val = current + 15;
            // OWS inicial (RFC 7230 §3.2.4: OWS = *( SP / HTAB ))
            while (val < nextEnd && (*val == ' ' || *val == '\t')) val++;

            // OWS final
            const char* valEnd = nextEnd;
            while (valEnd > val && (*(valEnd - 1) == ' ' || *(valEnd - 1) == '\t')) valEnd--;

            int parsedLen = -1;
            if (!parseBoundedDecimal(val, valEnd, parsedLen)) {
                return PARSE_BAD_REQUEST; // Rechaza signos '+' o '-', sufijos alfanuméricos o campos vacíos
            }
            if (parsedLen < 0 || parsedLen > 256) {
                return PARSE_PAYLOAD_TOO_LARGE; // Guardia de sobrecarga de búfer
            }
            outContentLen = parsedLen;
        }
        current = nextEnd + 2;
    }

    return PARSE_OK;
}

/**
 * Verificación de consistencia entre bytes residuales del cuerpo recibidos con
 * la cabecera y el Content-Length declarado (H-160).
 */
inline bool isResidualBodyValid(size_t residual, int contentLen) {
    if (contentLen < 0) return false;
    return (residual <= static_cast<size_t>(contentLen));
}

/**
 * Selección determinista de desalojo LRU robusta ante millis() rollover (H-158).
 * Utiliza resta sin signo (nowMs - lastAccessMs) en módulo 2^32.
 */
inline int selectLruCandidate(const uint32_t* lastAccessMs, const bool* underPenalty, size_t poolSize, uint32_t nowMs) {
    if (!lastAccessMs || !underPenalty || poolSize == 0) return -1;
    int bestLru = -1;
    uint32_t maxAgeMs = 0;
    for (size_t i = 0; i < poolSize; ++i) {
        if (!underPenalty[i]) {
            const uint32_t ageMs = static_cast<uint32_t>(nowMs - lastAccessMs[i]);
            if (bestLru < 0 || ageMs > maxAgeMs) {
                maxAgeMs = ageMs;
                bestLru = static_cast<int>(i);
            }
        }
    }
    return bestLru;
}

} // namespace FloraCare

#endif // HTTP_SECURITY_PARSER_H
