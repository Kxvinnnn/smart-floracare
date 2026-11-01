/**
 * ============================================================================
 * SMART FLORACARE - HTTP & SECURITY UTILITIES (PARSER Y VALIDADORES PUROS)
 * ============================================================================
 * Archivo: include/HttpSecurityParser.h
 * Descripción: Funciones puras y desacopladas para validación sintáctica de
 *              cabeceras HTTP (RFC 7230/9112), verificación de campos de
 *              seguridad (HMAC/Nonce) y política de desalojo LRU.
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
#include <stdlib.h>
#include <string.h>

namespace FloraCare {

/**
 * Comparación de prefijos insensible a mayúsculas/minúsculas 100% estándar C++.
 */
inline bool caseInsensitiveStartsWith(const char* str, const char* prefix) {
    if (!str || !prefix) return false;
    while (*prefix) {
        char c1 = *str++;
        char c2 = *prefix++;
        if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
        if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');
        if (c1 != c2) return false;
    }
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
 * Parser estricto de cabeceras HTTP (RFC 7230 / RFC 9112) (H-160).
 * - Exige verbos GET o POST.
 * - Rechaza explícitamente Transfer-Encoding (chunked no soportado en API embebida -> 501).
 * - Rechaza múltiples cabeceras Content-Length (RFC 7230 §3.3.2 -> 400).
 * - Exige gramática Content-Length = 1*DIGIT (rechaza estrictamente '+' o '-' -> 400).
 * - Rechaza caracteres espurios entre los dígitos y el terminador CRLF (-> 400).
 * - Limita la longitud máxima admisible a 256 bytes (-> 413).
 */
inline HttpParseResult parseHttpHeaderStrict(const char* buf, size_t len,
                                             char* outUri, size_t outUriMax,
                                             bool& outIsGet, bool& outIsPost,
                                             int& outContentLen) {
    outIsGet = false;
    outIsPost = false;
    outContentLen = -1;
    if (!buf || !outUri || outUriMax == 0) return PARSE_BAD_REQUEST;
    outUri[0] = '\0';

    const char* lineEnd = strstr(buf, "\r\n");
    if (!lineEnd) return PARSE_BAD_REQUEST;

    const char* p = buf;
    if (strncmp(p, "GET ", 4) == 0) {
        outIsGet = true;
        p += 4;
    } else if (strncmp(p, "POST ", 5) == 0) {
        outIsPost = true;
        p += 5;
    } else {
        return PARSE_METHOD_NOT_ALLOWED;
    }

    const char* space = strchr(p, ' ');
    if (!space || space > lineEnd) return PARSE_BAD_REQUEST;

    size_t uriLen = space - p;
    if (uriLen == 0 || uriLen >= outUriMax) return PARSE_BAD_URI;
    memcpy(outUri, p, uriLen);
    outUri[uriLen] = '\0';

    p = space + 1;
    if (strncmp(p, "HTTP/1.1", 8) != 0 && strncmp(p, "HTTP/1.0", 8) != 0) {
        return PARSE_BAD_REQUEST;
    }

    const char* current = lineEnd + 2;
    while (current < buf + len) {
        const char* nextEnd = strstr(current, "\r\n");
        if (!nextEnd) break;
        if (nextEnd == current) break; // Fin de cabeceras (\r\n\r\n)

        // Rechazo explícito de Transfer-Encoding (chunked no soportado en API embebida)
        if (caseInsensitiveStartsWith(current, "Transfer-Encoding:")) {
            return PARSE_NOT_IMPLEMENTED;
        }

        if (caseInsensitiveStartsWith(current, "Content-Length:")) {
            if (outContentLen != -1) {
                return PARSE_BAD_REQUEST; // Múltiples cabeceras Content-Length prohibidas (RFC 7230 §3.3.2)
            }
            const char* val = current + 15;
            while (*val == ' ' || *val == '\t') val++;

            // Gramática RFC 7230 §3.3.2 / RFC 9110 §8.6: Content-Length = 1*DIGIT (prohibido signo '+' o '-')
            if (*val < '0' || *val > '9') {
                return PARSE_BAD_REQUEST;
            }

            char* endPtr = nullptr;
            long parsedLen = strtol(val, &endPtr, 10);
            if (endPtr == val) return PARSE_BAD_REQUEST;
            if (parsedLen < 0 || parsedLen > 256) return PARSE_PAYLOAD_TOO_LARGE;

            // Validación estricta: asegurar que no haya caracteres espurios antes de CRLF
            while (*endPtr == ' ' || *endPtr == '\t') endPtr++;
            if (endPtr != nextEnd) return PARSE_BAD_REQUEST;

            outContentLen = static_cast<int>(parsedLen);
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
