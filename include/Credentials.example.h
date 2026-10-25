/**
 * ============================================================================
 * SMART FLORACARE - PLANTILLA DE CREDENCIALES Y CONFIGURACIÓN SEGURA
 * ============================================================================
 * Copia este archivo como 'Credentials.h' en el directorio 'include/' y 
 * completa con los datos de tu red. 'Credentials.h' está en .gitignore para
 * evitar publicar contraseñas y claves privadas en el repositorio.
 * ============================================================================
 */

#ifndef FLORACARE_CREDENTIALS_H
#define FLORACARE_CREDENTIALS_H

// --- CREDENCIALES WI-FI ---
#define SECRET_WIFI_SSID "TU_RED_WIFI_AQUI"
#define SECRET_WIFI_PASS "TU_CONTRASENA_AQUI"

// --- TOKEN DE AUTORIZACIÓN API REST ---
// Genera una cadena aleatoria de al menos 16 caracteres para la autenticación
#define SECRET_API_TOKEN "CAMBIA_ESTE_TOKEN_SECRETO_12345"

#endif // FLORACARE_CREDENTIALS_H
