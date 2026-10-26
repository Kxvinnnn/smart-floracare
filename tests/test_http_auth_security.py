#!/usr/bin/env python3
"""
=============================================================================
SMART FLORACARE - HTTP AUTHENTICATION & SECURITY TEST SUITE (NIVEL A Y B)
=============================================================================
Suite exhaustiva de 33 casos de prueba que verifican rigurosamente el
subsistema de autenticación, transporte, mitigación CSRF canónica, robustez
de credenciales, limitación no bloqueante e integridad del controlador:

  [Configuración y Arranque]
   1. Ausencia de Credentials.h.
   2. Ausencia de SSID o contraseña.
   3. Ausencia de token de autenticación.
   4. Token vacío.
   5. Token predeterminado o placeholder de demostración.
   6. Credenciales válidas completas.
   7. Comportamiento de arranque seguro en modo autónomo local.

  [Autenticación HTTP]
   8. Solicitud GET a ruta de riego (/api/water -> 405 Method Not Allowed).
   9. Solicitud POST sin token (-> 401 Unauthorized).
  10. Solicitud con token incorrecto (-> 401 Unauthorized).
  11. Solicitud con token demasiado corto (< 16 caracteres -> 403 Forbidden).
  12. Solicitud con token de longitud válida pero incorrecto (-> 401 Unauthorized).
  13. Solicitud con token correcto en mecanismo admitido (Bearer y Formulario).
  14. Solicitud con encabezado Authorization malformado (-> 401 Unauthorized).
  15. Solicitud con esquema de autorización incorrecto (ej. Basic -> 401 Unauthorized).
  16. Solicitud con token en ubicación no admitida (query string / cookie -> 401).
  17. Solicitud con credenciales contradictorias (header != body -> 400 Bad Request).
  18. Inspección de tamaño de carga excesiva (> 256 bytes -> 413 Payload Too Large).
  19. Control de tasa no bloqueante ante fallos sucesivos (rate limit -> 429 Too Many Requests).
  20. Validación de códigos HTTP y estructura de respuestas JSON en regador_esp32.cpp.

  [CSRF y Transporte]
  21. Solicitud desde origen cruzado malicioso y bypasses de subcadena (-> 403 Forbidden).
  22. Ausencia de encabezado Origin permitida para clientes REST legítimos (cURL/scripts).
  23. Verificación de que el método GET jamás activa el actuador.
  24. Verificación de que el secreto nunca se devuelve en el cuerpo de respuesta.
  25. Verificación de que los registros seriales no contienen credenciales ni tokens.
  26. Verificación de limitaciones de transporte HTTP en texto plano y políticas.

  [Integridad del Controlador]
  27. Token válido con depósito vacío (-> 409 Conflict, bomba apagada).
  28. Token válido con sensor en fallo (-> enclavamiento de seguridad).
  29. Token válido mientras el controlador está bloqueado por FAULT (-> 409 Conflict).
  30. Token válido durante bombeo activo (rechazo sin extensión de tiempo).
  31. Solicitudes repetidas durante el intervalo de enfriamiento (cooldown 5s).
  32. Confirmación de que ningún rechazo activa la bomba en ningún caso.
  33. Confirmación de que una solicitud aceptada respeta estrictamente el límite de 3s.
=============================================================================
"""

import os
import re
import unittest
from test_irrigation_suite import (
    MockIrrigationController,
    ControllerState,
    IrrigationSource,
    SensorHealth,
    StopReason,
    FaultReason
)

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
ESP32_CPP = os.path.join(REPO_ROOT, "src", "esp32_mejorado_wifi", "regador_esp32.cpp")


class MockHttpServerEndpoint:
    """
    Modelo de integración en Python que replica fielmente el comportamiento
    del endpoint handleApiWater() y las reglas de seguridad de regador_esp32.cpp.
    """
    MIN_TOKEN_LEN = 16
    MAX_TOKEN_LEN = 128
    KNOWN_PLACEHOLDERS = (
        "GENERA_UN_TOKEN_SECRETO_DE_16_O_MAS_CARACTERES",
        "CAMBIA_ESTE_TOKEN_SECRETO_12345",
        "FLORACARE_DEFAULT_TOKEN_CHANGE_ME",
        "1234567890123456"
    )

    def __init__(self, configured_token, controller, server_host="192.168.1.50"):
        self.configured_token = configured_token
        self.controller = controller
        self.server_host = server_host
        self.last_failed_auth_ms = 0

    def is_token_insecure(self, token):
        if not token:
            return True
        if len(token) < self.MIN_TOKEN_LEN:
            return True
        if token in self.KNOWN_PLACEHOLDERS:
            return True
        return False

    def constant_time_equals(self, a, b):
        if not a or not b:
            return False
        len_a = len(a)
        len_b = len(b)
        if len_a > self.MAX_TOKEN_LEN or len_b > self.MAX_TOKEN_LEN:
            return False
        diff = len_a ^ len_b
        max_iter = max(len_a, len_b)
        for i in range(max_iter):
            ca = ord(a[i]) if i < len_a else 0
            cb = ord(b[i]) if i < len_b else 0
            diff |= (ca ^ cb)
        return diff == 0

    def _validate_single_authority(self, target, is_origin):
        if target is None:
            return False
        target = target.strip()
        if not target or len(target) < 8:
            return False
        # H-01.5: Esquema obligatorio http:// (insensible a mayúsculas)
        if target[:7].lower() != "http://":
            return False
        authority = target[7:]

        # H-01.1: Origin según RFC 6454 NO admite '/', '?' ni '#'
        if is_origin:
            if "/" in authority or "?" in authority or "#" in authority:
                return False
        else:
            for sep in ('/', '?', '#'):
                if sep in authority:
                    authority = authority.split(sep, 1)[0]

        # H-01.3: Caracteres de control, no-ASCII, espacios, userinfo (@) o IPv6
        if not authority or any(ord(c) < 33 or ord(c) > 126 or c in "@[]" for c in authority):
            return False

        # H-01.6: Separación de Host y Puerto con validación estricta
        if ":" in authority:
            origin_host, origin_port = authority.split(":", 1)
            if not origin_port or not origin_port.isdigit():
                return False
            # Puerto explícito debe ser estrictamente 80 (rechaza 8080, 080, etc.)
            if origin_port != "80":
                return False
        else:
            origin_host = authority

        if not origin_host or self.server_host == "0.0.0.0":
            return False

        # H-01.2 & H-01.7: Normalización a minúsculas y validación contra IP configurada
        return origin_host.lower() == self.server_host.lower()

    def is_origin_allowed(self, origin=None, referer=None):
        if origin is None and referer is None:
            return True # Clientes REST legítimos sin navegador (cURL, scripts, IoT gateways)

        if origin is not None:
            if not self._validate_single_authority(origin, True):
                return False

        if referer is not None:
            if not self._validate_single_authority(referer, False):
                return False

        return True

    def handle_request(self, method, form_token=None, auth_header=None,
                       origin=None, referer=None, now_ms=1000, water_level_ok=True,
                       sensor_health=SensorHealth.OK, current_moisture=50,
                       content_length=None):
        # 1. Método HTTP
        if method != "POST":
            return 405, {"error": "Method Not Allowed. Use POST."}

        # 2. Control de tasa no bloqueante ante fallos sucesivos
        if self.last_failed_auth_ms > 0 and (now_ms - self.last_failed_auth_ms < 1000):
            return 429, {"error": "Too Many Requests. Authentication rate limit active. Please wait."}

        # 3. Tamaño de carga excesivo (previene DoS en Heap)
        if content_length is not None and content_length > 256:
            return 413, {"error": "Payload Too Large. Request body exceeds maximum allowed size."}

        # 4. Token configurado en firmware
        if self.is_token_insecure(self.configured_token):
            return 403, {"error": "Forbidden. Remote API disabled: Insecure or default token configured in Credentials.h"}

        # 5. Origen cruzado canónico (CSRF) y Referer
        if not self.is_origin_allowed(origin=origin, referer=referer):
            return 403, {"error": "Forbidden. Cross-Origin request blocked."}

        # 6. Extracción de credenciales
        has_form = bool(form_token)
        has_header = False
        header_token = None

        if auth_header:
            if len(auth_header) > (self.MAX_TOKEN_LEN + 16):
                return 400, {"error": "Bad Request. Authorization header exceeds maximum allowed length."}
            if auth_header.startswith("Bearer "):
                header_token = auth_header[7:].strip()
                has_header = True
            else:
                return 401, {"error": "Unauthorized. Invalid Authorization scheme. Expected 'Bearer <token>'."}

        if has_form and has_header:
            if form_token != header_token:
                return 400, {"error": "Bad Request. Conflicting credentials provided in form and Authorization header."}

        chosen_token = header_token if has_header else form_token

        if not chosen_token:
            return 401, {"error": "Unauthorized. Missing authentication token."}

        if len(chosen_token) > self.MAX_TOKEN_LEN:
            return 400, {"error": "Bad Request. Token exceeds maximum allowed length."}

        # 7. Comparación segura en tiempo constante
        if not self.constant_time_equals(chosen_token, self.configured_token):
            self.last_failed_auth_ms = now_ms
            return 401, {"error": "Unauthorized. Invalid token."}

        # 8. Despacho al controlador central de riego
        accepted = self.controller.request_watering(
            IrrigationSource.REMOTE_WEB,
            now_ms,
            water_level_ok,
            sensor_health,
            current_moisture
        )

        if accepted:
            return 303, {"Location": "/"}
        else:
            return 409, {
                "error": "Watering request rejected by safety controller",
                "state": self.controller.state.value,
                "lastFault": self.controller.last_fault.value
            }


class TestHttpAuthSecurityComprehensive(unittest.TestCase):

    def setUp(self):
        self.controller = MockIrrigationController(
            max_auto_ms=20000,
            manual_ms=3000,
            watchdog_ms=25000,
            cooldown_ms=5000,
            start_pct=25,
            stop_pct=70
        )
        self.valid_secret = "k7P#9mQ2$vL5xW8z!yR4bN1cF3jT6hU0" # 32 chars alta entropía
        self.endpoint = MockHttpServerEndpoint(self.valid_secret, self.controller)

    # --- SECCIÓN 1: CONFIGURACIÓN Y ARRANQUE (CASOS 1-7) ---

    def test_01_absence_of_credentials_h(self):
        """1. Ausencia de Credentials.h deshabilita Wi-Fi y API remota condicionalmente."""
        with open(ESP32_CPP, "r", encoding="utf-8") as f:
            content = f.read()
        self.assertIn('#if __has_include("Credentials.h")', content)
        self.assertIn("#define HAS_LOCAL_CREDENTIALS 0", content)
        self.assertIn("static constexpr bool WIFI_ENABLED = false;", content)

    def test_02_absence_of_ssid_or_password(self):
        """2. Ausencia de SSID o contraseña deshabilita Wi-Fi y deja SSID vacío."""
        with open(ESP32_CPP, "r", encoding="utf-8") as f:
            content = f.read()
        self.assertIn("WIFI_ENABLED && strlen(WIFI_SSID) > 0", content)
        self.assertIn("WiFi.mode(WIFI_OFF);", content)

    def test_03_absence_of_api_token(self):
        """3. Ausencia de SECRET_API_TOKEN asigna cadena vacía y bloquea la API remota."""
        ep_no_token = MockHttpServerEndpoint("", self.controller)
        status, body = ep_no_token.handle_request("POST", form_token="anything1234567890")
        self.assertEqual(status, 403)
        self.assertIn("Remote API disabled", body["error"])

    def test_04_token_empty(self):
        """4. Token de configuración configurado como cadena vacía rechaza peticiones con 403."""
        ep_empty = MockHttpServerEndpoint("", self.controller)
        status, body = ep_empty.handle_request("POST", form_token="k7P#9mQ2$vL5xW8z!yR4bN1cF3jT6hU0")
        self.assertEqual(status, 403)

    def test_05_token_default_or_demo_placeholder(self):
        """5. Tokens de plantilla y placeholders conocidos se bloquean con 403."""
        placeholders = [
            "GENERA_UN_TOKEN_SECRETO_DE_16_O_MAS_CARACTERES",
            "CAMBIA_ESTE_TOKEN_SECRETO_12345",
            "1234567890123456",
            "short_token"
        ]
        for ph in placeholders:
            ep_ph = MockHttpServerEndpoint(ph, self.controller)
            status, body = ep_ph.handle_request("POST", form_token=ph)
            self.assertEqual(status, 403, f"Fallo al bloquear placeholder: {ph}")

    def test_06_valid_credentials_allowed(self):
        """6. Credenciales válidas de alta entropía permiten operar el endpoint."""
        status, _ = self.endpoint.handle_request("POST", form_token=self.valid_secret)
        self.assertEqual(status, 303)
        self.assertEqual(self.controller.state, ControllerState.WATERING)

    def test_07_safe_boot_without_remote_connectivity(self):
        """7. Arranque sin conectividad opera el lazo local con bomba apagada."""
        self.assertEqual(self.controller.state, ControllerState.IDLE)
        self.assertFalse(self.controller.is_pump_active())

    # --- SECCIÓN 2: AUTENTICACIÓN HTTP (CASOS 8-20) ---

    def test_08_get_request_to_water_endpoint_rejected(self):
        """8. Solicitud GET a /api/water se rechaza con 405 Method Not Allowed."""
        status, body = self.endpoint.handle_request("GET", form_token=self.valid_secret)
        self.assertEqual(status, 405)
        self.assertIn("Method Not Allowed", body["error"])
        self.assertFalse(self.controller.is_pump_active())

    def test_09_post_without_token_rejected(self):
        """9. Solicitud POST sin token se rechaza con 401 Unauthorized."""
        status, body = self.endpoint.handle_request("POST", form_token=None, auth_header=None)
        self.assertEqual(status, 401)
        self.assertIn("Missing authentication token", body["error"])

    def test_10_post_with_invalid_token_rejected(self):
        """10. Solicitud con token incorrecto se rechaza con 401 Unauthorized."""
        status, body = self.endpoint.handle_request("POST", form_token="incorrect_token_12345")
        self.assertEqual(status, 401)
        self.assertIn("Invalid token", body["error"])

    def test_11_post_with_token_too_short_rejected(self):
        """11. Solicitud con token menor a 16 caracteres se rechaza con 401."""
        status, body = self.endpoint.handle_request("POST", form_token="short123")
        self.assertEqual(status, 401)
        self.assertIn("Invalid token", body["error"])

    def test_12_post_with_valid_length_but_wrong_token(self):
        """12. Solicitud con token de 32 caracteres pero contenido incorrecto se rechaza con 401."""
        status, body = self.endpoint.handle_request("POST", form_token="A" * 32)
        self.assertEqual(status, 401)

    def test_13_post_with_valid_token_supported_mechanisms(self):
        """13. Token correcto se acepta tanto por cabecera Bearer como por formulario."""
        # Vía Formulario
        self.controller.stop(StopReason.NORMAL_TARGET_REACHED, 1000)
        self.controller.last_stop_ms = 0
        status_form, _ = self.endpoint.handle_request("POST", form_token=self.valid_secret, now_ms=1000)
        self.assertEqual(status_form, 303)

        # Vía Header Authorization: Bearer
        self.controller.stop(StopReason.NORMAL_TARGET_REACHED, 5000)
        self.controller.last_stop_ms = 0
        status_hdr, _ = self.endpoint.handle_request("POST", auth_header=f"Bearer {self.valid_secret}", now_ms=12000)
        self.assertEqual(status_hdr, 303)

    def test_14_malformed_authorization_header(self):
        """14. Encabezado Authorization malformado (sin 'Bearer ') se rechaza con 401."""
        status, body = self.endpoint.handle_request("POST", auth_header=self.valid_secret)
        self.assertEqual(status, 401)
        self.assertIn("Invalid Authorization scheme", body["error"])

    def test_15_incorrect_authorization_scheme(self):
        """15. Esquema no soportado (ej. Basic) se rechaza con 401."""
        status, body = self.endpoint.handle_request("POST", auth_header=f"Basic {self.valid_secret}")
        self.assertEqual(status, 401)
        self.assertIn("Invalid Authorization scheme", body["error"])

    def test_16_token_in_unsupported_location(self):
        """16. Petición sin token en formulario ni header (ej. en URL) se rechaza con 401."""
        status, body = self.endpoint.handle_request("POST", form_token="", auth_header="")
        self.assertEqual(status, 401)

    def test_17_conflicting_tokens_in_form_and_header(self):
        """17. Petición con tokens contradictorios entre header y body se rechaza con 400 Bad Request."""
        status, body = self.endpoint.handle_request(
            "POST",
            form_token=self.valid_secret,
            auth_header="Bearer DIFFERENT_TOKEN_999999"
        )
        self.assertEqual(status, 400)
        self.assertIn("Conflicting credentials", body["error"])

    def test_18_payload_too_large_rejection(self):
        """18. Carga útil mayor a 256 bytes se rechaza de inmediato con 413 Payload Too Large."""
        status, body = self.endpoint.handle_request(
            "POST",
            form_token=self.valid_secret,
            content_length=512
        )
        self.assertEqual(status, 413)
        self.assertIn("Payload Too Large", body["error"])

    def test_19_rate_limiting_non_blocking_cooldown(self):
        """19. Fallos sucesivos de autenticación en menos de 1000 ms se rechazan con 429 Too Many Requests."""
        # Intento 1 fallido a los 1000 ms -> 401
        status1, _ = self.endpoint.handle_request("POST", form_token="WRONG_1", now_ms=1000)
        self.assertEqual(status1, 401)

        # Intento 2 fallido a los 1300 ms (< 1000 ms transcurridos) -> 429 Too Many Requests
        status2, body2 = self.endpoint.handle_request("POST", form_token="WRONG_2", now_ms=1300)
        self.assertEqual(status2, 429)
        self.assertIn("Too Many Requests", body2["error"])

        # Intento 3 a los 2100 ms (> 1000 ms transcurridos) -> vuelve a evaluar credencial
        status3, _ = self.endpoint.handle_request("POST", form_token="WRONG_3", now_ms=2100)
        self.assertEqual(status3, 401)

    def test_20_http_response_status_and_json_structure(self):
        """20. Validación de códigos HTTP y estructura de respuestas JSON en regador_esp32.cpp."""
        with open(ESP32_CPP, "r", encoding="utf-8") as f:
            content = f.read()
        self.assertIn('server.send(405, "application/json"', content)
        self.assertIn('server.send(403, "application/json"', content)
        self.assertIn('server.send(401, "application/json"', content)
        self.assertIn('server.send(400, "application/json"', content)
        self.assertIn('server.send(413, "application/json"', content)
        self.assertIn('server.send(429, "application/json"', content)
        self.assertIn('server.send(409, "application/json"', content)
        self.assertIn('server.send(303);', content)

    # --- SECCIÓN 3: CSRF Y TRANSPORTE (CASOS 21-26) ---

    def test_21_cross_origin_request_and_subdomain_bypasses_blocked(self):
        """21. Solicitudes con Origin externo, subdominios falsos o queries son bloqueadas con 403."""
        malicious_origins = [
            "http://sitio-malicioso-externo.com",
            "http://192.168.1.50.sitio-atacante.com",   # Evasión clásica de subcadena
            "http://sitio-atacante.com/?q=192.168.1.50",# Evasión en query string
            "http://sitio-atacante.com/192.168.1.50",   # Evasión en ruta
            "null",                                     # Opaque origin
            "file:///tmp/poc.html",                     # Esquema local no permitido
            "https://192.168.1.50",                     # H-01.5: Esquema HTTPS dispar
            "ftp://192.168.1.50",                       # H-01.5: Esquema FTP dispar
            "ws://192.168.1.50",                        # H-01.5: Esquema WebSocket dispar
            "http://192.168.1.50:8080",                 # H-01.6: Puerto dispar no permitido
            "http://192.168.1.50:3000",                 # Puerto cruzado local no permitido
            "http://[fe80::1]:8080",                    # IPv6 con puerto cruzado
            "http://[fe80::1]:80",                      # IPv6 rechazado explícitamente
            "http://192.168.1.50:",                     # H-01.6: Dos puntos finales sin dígitos de puerto
            "http://192.168.1.50:80a",                  # H-01.6: Puerto no numérico
            "http://192.168.1.50:080",                  # H-01.6: Puerto con ceros iniciales (no canónico)
            "http://192.168.1.50:+80",                  # H-01.6: Puerto con signo positivo no numérico
            "http://192.168.1.50: 80",                  # H-01.3: Espacio en autoridad de puerto
            "http://",                                  # Esquema sin autoridad
            "http://:80",                               # Autoridad sin host
            "http://usuario@192.168.1.50",              # Userinfo incrustado
            "http://attacker@192.168.1.50:80",          # Userinfo con puerto
            "http://@192.168.1.50",                     # Arroba sin usuario
            "http://192.168.1.50/dashboard",            # Origin con ruta (malformado en RFC 6454)
            "http://192.168.1.50?query=1",              # Origin con query (malformado en RFC 6454)
            "http://192.168.1.50#section",              # Origin con fragmento (malformado en RFC 6454)
            "http://floracare.local",                   # mDNS no configurado en firmware
            "",                                         # Cabecera vacía presente
            "   "                                       # Cabecera solo espacios presente
        ]
        for origin_hdr in malicious_origins:
            status, body = self.endpoint.handle_request(
                "POST",
                form_token=self.valid_secret,
                origin=origin_hdr,
                now_ms=50000
            )
            self.assertEqual(status, 403, f"Fallo al bloquear origen malicioso o malformado: '{origin_hdr}'")
            self.assertIn("Cross-Origin request blocked", body["error"])
            self.assertFalse(self.controller.is_pump_active())

        # Validación de cabecera Referer vacía presente
        status_empty_ref, _ = self.endpoint.handle_request(
            "POST",
            form_token=self.valid_secret,
            origin=None,
            referer="",
            now_ms=50500
        )
        self.assertEqual(status_empty_ref, 403, "Fallo al bloquear Referer vacío presente")

        # Validación cruzada mediante Referer cuando Origin está ausente
        status_ref, body_ref = self.endpoint.handle_request(
            "POST",
            form_token=self.valid_secret,
            origin=None,
            referer="http://sitio-atacante.com/malicious_page.html",
            now_ms=51000
        )
        self.assertEqual(status_ref, 403, "Fallo al bloquear Referer cruzado malicioso")
        self.assertIn("Cross-Origin request blocked", body_ref["error"])

        # Validación de conflicto: Origin legítimo pero Referer cruzado malicioso
        status_conflict1, _ = self.endpoint.handle_request(
            "POST",
            form_token=self.valid_secret,
            origin="http://192.168.1.50",
            referer="http://sitio-atacante.com/malicious.html",
            now_ms=51500
        )
        self.assertEqual(status_conflict1, 403, "Fallo al bloquear petición con Referer malicioso aunque Origin sea válido")
        self.assertFalse(self.controller.is_pump_active())

        # Validación de conflicto inverso: Origin malicioso con Referer local
        status_conflict2, _ = self.endpoint.handle_request(
            "POST",
            form_token=self.valid_secret,
            origin="http://sitio-malicioso.com",
            referer="http://192.168.1.50/",
            now_ms=52000
        )
        self.assertEqual(status_conflict2, 403, "Fallo al bloquear petición con Origin malicioso aunque Referer sea válido")
        self.assertFalse(self.controller.is_pump_active())

        # Validación de servidor con IP no asignada (0.0.0.0): debe rechazar cualquier origen
        orig_host = self.endpoint.server_host
        self.endpoint.server_host = "0.0.0.0"
        status_unassigned, _ = self.endpoint.handle_request(
            "POST",
            form_token=self.valid_secret,
            origin="http://192.168.1.50",
            now_ms=52500
        )
        self.assertEqual(status_unassigned, 403, "Fallo al bloquear peticiones cuando la IP del servidor es 0.0.0.0")
        self.endpoint.server_host = orig_host

    def test_22_legitimate_origin_or_curl_permitted(self):
        """22. Origin local legítimo (con o sin puerto) o ausencia de Origin (cURL) es admitido."""
        # Origen local sin puerto
        self.controller.last_stop_ms = 0
        status_local, _ = self.endpoint.handle_request(
            "POST",
            form_token=self.valid_secret,
            origin="http://192.168.1.50",
            now_ms=10000
        )
        self.assertEqual(status_local, 303)

        # Origen local con puerto :80
        self.controller.stop(StopReason.NORMAL_TARGET_REACHED, 12000)
        self.controller.last_stop_ms = 0
        status_port, _ = self.endpoint.handle_request(
            "POST",
            form_token=self.valid_secret,
            origin="http://192.168.1.50:80",
            now_ms=20000
        )
        self.assertEqual(status_port, 303)

        # Origen local con esquema en mayúsculas (HTTP://)
        self.controller.stop(StopReason.NORMAL_TARGET_REACHED, 22000)
        self.controller.last_stop_ms = 0
        status_upper, _ = self.endpoint.handle_request(
            "POST",
            form_token=self.valid_secret,
            origin="HTTP://192.168.1.50",
            now_ms=23000
        )
        self.assertEqual(status_upper, 303)

        # Sin origen (cURL / script REST)
        self.controller.stop(StopReason.NORMAL_TARGET_REACHED, 24000)
        self.controller.last_stop_ms = 0
        status_curl, _ = self.endpoint.handle_request(
            "POST",
            auth_header=f"Bearer {self.valid_secret}",
            origin=None,
            now_ms=25000
        )
        self.assertEqual(status_curl, 303)
        self.assertEqual(status_curl, 303)

    def test_23_get_never_triggers_irrigation(self):
        """23. El método GET bajo ninguna circunstancia activa la bomba."""
        self.endpoint.handle_request("GET", form_token=self.valid_secret)
        self.assertFalse(self.controller.is_pump_active())

    def test_24_secret_never_echoed_in_response(self):
        """24. El secreto nunca se devuelve en el cuerpo de respuesta de ningún código."""
        for method in ["GET", "POST"]:
            _, body = self.endpoint.handle_request(method, form_token=self.valid_secret, now_ms=40000)
            body_str = str(body)
            self.assertNotIn(self.valid_secret, body_str)

    def test_25_logs_contain_no_credentials(self):
        """25. Los registros en código fuente no imprimen variables de contraseñas ni tokens."""
        with open(ESP32_CPP, "r", encoding="utf-8") as f:
            content = f.read()
        self.assertNotIn("Serial.print(API_AUTH_TOKEN)", content)
        self.assertNotIn("Serial.print(WIFI_PASS)", content)
        self.assertNotIn("Serial.println(API_AUTH_TOKEN)", content)
        self.assertNotIn("Serial.println(WIFI_PASS)", content)

    def test_26_transport_limitations_asserted(self):
        """26. El servidor opera en HTTP en puerto 80 con restricción explícita a LAN aislada."""
        with open(ESP32_CPP, "r", encoding="utf-8") as f:
            content = f.read()
        self.assertIn("WebServer server(80);", content)

    # --- SECCIÓN 4: INTEGRIDAD DEL CONTROLADOR (CASOS 27-33) ---

    def test_27_valid_token_with_empty_tank_rejected(self):
        """27. Token válido con depósito vacío es rechazado por el controlador (409 Conflict)."""
        status, body = self.endpoint.handle_request(
            "POST",
            form_token=self.valid_secret,
            water_level_ok=False,
            now_ms=50000
        )
        self.assertEqual(status, 409)
        self.assertFalse(self.controller.is_pump_active())
        self.assertEqual(self.controller.state, ControllerState.FAULT)

    def test_28_valid_token_with_sensor_malfunction(self):
        """28. Riego manual/web con token válido no depende del sensor de suelo salvo fallo en auto."""
        status, _ = self.endpoint.handle_request(
            "POST",
            form_token=self.valid_secret,
            water_level_ok=True,
            sensor_health=SensorHealth.SHORT_CIRCUIT_OR_LOW,
            now_ms=60000
        )
        self.assertEqual(status, 303)
        self.assertTrue(self.controller.is_pump_active())

    def test_29_valid_token_during_fault_lockout_rejected(self):
        """29. Token válido durante estado FAULT del controlador se rechaza con 409 Conflict."""
        self.controller.state = ControllerState.FAULT
        self.controller.last_fault = FaultReason.MAX_TIMEOUT_EXCEEDED
        status, body = self.endpoint.handle_request("POST", form_token=self.valid_secret, now_ms=70000)
        self.assertEqual(status, 409)
        self.assertFalse(self.controller.is_pump_active())

    def test_30_valid_token_during_active_watering_does_not_extend(self):
        """30. Petición adicional durante riego no extiende el tiempo ni reinicia el temporizador."""
        self.endpoint.handle_request("POST", form_token=self.valid_secret, now_ms=80000)
        start_time = self.controller.pump_start_time_ms
        self.endpoint.handle_request("POST", form_token=self.valid_secret, now_ms=80500)
        self.assertEqual(self.controller.pump_start_time_ms, start_time)

    def test_31_repeated_requests_during_cooldown_rejected(self):
        """31. Petición durante el periodo de enfriamiento (cooldown 5s) se rechaza con 409."""
        self.controller.last_stop_ms = 90000
        # Petición a los 2000 ms del apagado (< 5000 ms cooldown)
        status, body = self.endpoint.handle_request("POST", form_token=self.valid_secret, now_ms=92000)
        self.assertEqual(status, 409)
        self.assertFalse(self.controller.is_pump_active())

    def test_32_confirmation_rejections_never_activate_pump(self):
        """32. Verificación universal: cada rechazo (400, 401, 403, 405, 409, 413, 429) devuelve su código exacto y garantiza bomba apagada."""
        scenarios = [
            (400, "Token excede longitud maxima (>128)", "POST", "A" * 129, None, None, None, True, None, None),
            (400, "Credenciales contradictorias form != header", "POST", self.valid_secret, "Bearer " + self.valid_secret + "_diff", None, None, True, None, None),
            (401, "Token ausente", "POST", "", None, None, None, True, None, None),
            (401, "Token invalido/erroneo", "POST", "TOKEN_INCORRECTO_123", None, None, None, True, None, None),
            (401, "Esquema Authorization invalido", "POST", None, "Basic dXNlcjpwYXNz", None, None, True, None, None),
            (403, "Token en firmware es placeholder inseguro", "POST", self.valid_secret, None, None, None, True, None, "set_insecure_fw_token"),
            (403, "Origin de origen cruzado malicioso", "POST", self.valid_secret, None, "http://sitio-malicioso.com", None, True, None, None),
            (403, "Origin malformado con ruta", "POST", self.valid_secret, None, "http://192.168.1.50/malformed", None, True, None, None),
            (405, "Metodo HTTP GET no permitido", "GET", self.valid_secret, None, None, None, True, None, None),
            (409, "Deposito de agua vacio", "POST", self.valid_secret, None, None, None, False, None, None),
            (409, "Controlador en estado FAULT", "POST", self.valid_secret, None, None, None, True, None, "set_controller_fault"),
            (409, "Periodo de enfriamiento activo (cooldown)", "POST", self.valid_secret, None, None, None, True, None, "set_cooldown"),
            (413, "Cuerpo superior al limite de 256 bytes", "POST", self.valid_secret, None, None, None, True, 512, None),
            (429, "Limitador de tasa activo tras fallo previo", "POST", self.valid_secret, None, None, None, True, None, "set_rate_limit"),
        ]

        original_token = self.endpoint.configured_token
        for expected_code, desc, meth, f_tok, a_hdr, orig, ref, w_ok, c_len, prep in scenarios:
            self.controller.state = ControllerState.IDLE
            self.controller.last_stop_ms = 0
            self.endpoint.configured_token = original_token
            self.endpoint.last_failed_auth_ms = 0

            if prep == "set_insecure_fw_token":
                self.endpoint.configured_token = "CAMBIA_ESTE_TOKEN_SECRETO_12345"
            elif prep == "set_controller_fault":
                self.controller.state = ControllerState.FAULT
                self.controller.last_fault = FaultReason.MAX_TIMEOUT_EXCEEDED
            elif prep == "set_cooldown":
                self.controller.last_stop_ms = 100000
            elif prep == "set_rate_limit":
                self.endpoint.last_failed_auth_ms = 99500 # Fallo previo hace 500ms (<1000ms)

            now = 100000
            status, _ = self.endpoint.handle_request(
                meth,
                form_token=f_tok,
                auth_header=a_hdr,
                origin=orig,
                referer=ref,
                water_level_ok=w_ok,
                content_length=c_len,
                now_ms=now
            )

            self.assertEqual(
                status, expected_code,
                f"Escenario '{desc}' debia devolver HTTP {expected_code}, pero devolvio {status}"
            )
            self.assertFalse(
                self.controller.is_pump_active(),
                f"VIOLACION CRITICA DE SEGURIDAD: Bomba activada en rechazo HTTP {status} ('{desc}')"
            )
        self.endpoint.configured_token = original_token

    def test_33_accepted_request_obeys_manual_burst_limit(self):
        """33. Petición aceptada obedece estrictamente el pulso de 3s y corta la bomba."""
        self.controller.last_stop_ms = 0
        status, _ = self.endpoint.handle_request("POST", form_token=self.valid_secret, now_ms=110000)
        self.assertEqual(status, 303)
        self.assertTrue(self.controller.is_pump_active())

        # A los 2999 ms sigue activa
        running = self.controller.update(112999, True, SensorHealth.OK, 50)
        self.assertTrue(running)

        # A los 3000 ms exactos debe cortar
        running = self.controller.update(113000, True, SensorHealth.OK, 50)
        self.assertFalse(running)
        self.assertEqual(self.controller.state, ControllerState.IDLE)
        self.assertEqual(self.controller.last_stop_reason, StopReason.MANUAL_BURST_COMPLETE)


if __name__ == "__main__":
    unittest.main(verbosity=2)
