#!/usr/bin/env python3
"""
Firmware Security & Static Code Verification Suite for Smart FloraCare.

Verifies:
1. Strict absence of default/hardcoded passwords, SSIDs, and tokens in firmware source files.
2. Proper HTTP header authorization collection (server.collectHeaders) in ESP32 firmware.
3. Absence of unconstrained blocking loops (while(true){}, while(1){}) in runtime routines.
4. Safe fail-safe polarity checks for tank float switch.
"""

import os
import re
import unittest

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SRC_DIR = os.path.join(REPO_ROOT, "src")
ESP32_CPP = os.path.join(SRC_DIR, "esp32_mejorado_wifi", "regador_esp32.cpp")
NANO_CPP = os.path.join(SRC_DIR, "arduino_nano_mejorado", "regador_nano_pro.cpp")

FORBIDDEN_FALLBACK_PATTERNS = [
    r"plantas123",
    r"FLORACARE_DEFAULT_TOKEN",
    r"FLORACARE_SETUP_AP",
    r'#define\s+SECRET_WIFI_PASS\s+"[^"]+"',
    r'#define\s+SECRET_API_TOKEN\s+"[^"]+"',
]

class TestFirmwareSecurity(unittest.TestCase):

    def test_no_hardcoded_credentials_in_esp32(self):
        """Verify that esp32 source contains zero default passwords or tokens."""
        self.assertTrue(os.path.exists(ESP32_CPP), f"Missing {ESP32_CPP}")
        with open(ESP32_CPP, "r", encoding="utf-8") as f:
            content = f.read()

        for pattern in FORBIDDEN_FALLBACK_PATTERNS:
            matches = re.findall(pattern, content)
            self.assertEqual(
                len(matches), 0,
                f"Security violation: Found forbidden hardcoded pattern '{pattern}' in {ESP32_CPP}"
            )

    def test_credentials_template_exists_and_clean(self):
        """Verify Credentials.h.example exists and contains no production secrets."""
        template_path = os.path.join(SRC_DIR, "esp32_mejorado_wifi", "Credentials.h.example")
        self.assertTrue(os.path.exists(template_path), f"Missing {template_path}")
        with open(template_path, "r", encoding="utf-8") as f:
            content = f.read()
        self.assertIn("TU_RED_WIFI", content)
        self.assertIn("TU_CONTRASENA_WIFI", content)

    def test_http_authorization_headers_collected(self):
        """Verify ESP32 web server collects Authorization header before endpoint registration."""
        with open(ESP32_CPP, "r", encoding="utf-8") as f:
            content = f.read()
        self.assertIn("server.collectHeaders(headerKeys, 1);", content)
        self.assertIn('const char* headerKeys[] = {"Authorization"};', content)

    def test_safe_float_switch_failsafe_logic(self):
        """Verify that water level switch uses INPUT_PULLUP and checks LOW for OK."""
        with open(ESP32_CPP, "r", encoding="utf-8") as f:
            esp_content = f.read()
        self.assertIn("pinMode(PIN_FLOTADOR_AGUA, INPUT_PULLUP);", esp_content)
        self.assertIn("digitalRead(PIN_FLOTADOR_AGUA) == LOW", esp_content)

        with open(NANO_CPP, "r", encoding="utf-8") as f:
            nano_content = f.read()
        self.assertIn("pinMode(PIN_FLOTADOR_AGUA, INPUT_PULLUP);", nano_content)
        self.assertIn("digitalRead(PIN_FLOTADOR_AGUA) == LOW", nano_content)

    def test_no_infinite_blocking_loops_in_loop(self):
        """Verify runtime loops do not contain unconstrained while(true) loops."""
        for path in [ESP32_CPP, NANO_CPP]:
            with open(path, "r", encoding="utf-8") as f:
                content = f.read()
            # Extract loop() function body
            loop_match = re.search(r"void\s+loop\s*\(\s*\)\s*\{([^}]*)\}", content, re.DOTALL)
            if loop_match:
                loop_body = loop_match.group(1)
                self.assertNotIn("while (true)", loop_body)
                self.assertNotIn("while(1)", loop_body)

if __name__ == "__main__":
    unittest.main()
