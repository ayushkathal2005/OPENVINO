#!/usr/bin/env python3
"""
Unit and Integration Tests for Local UI Server Bridge (Phase 08)
Validates strict localhost binding, input validation, and local C++ invocation.
"""

import unittest
import threading
import json
import time
import urllib.request
import urllib.error
from pathlib import Path
import sys

REPO_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO_ROOT / "ui"))
import server

class TestLocalUIServer(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.port = 8899
        cls.httpd = server.run_server(port=cls.port)
        cls.server_thread = threading.Thread(target=cls.httpd.serve_forever, daemon=True)
        cls.server_thread.start()
        time.sleep(0.5)

    @classmethod
    def tearDownClass(cls):
        cls.httpd.shutdown()
        cls.httpd.server_close()

    def test_localhost_binding(self):
        """Verify server binds strictly to 127.0.0.1"""
        self.assertEqual(self.httpd.server_address[0], "127.0.0.1")
        self.assertEqual(self.httpd.server_address[1], self.port)

    def test_api_status(self):
        """Verify status endpoint returns C++ binary metadata and zero cloud dependencies"""
        url = f"http://127.0.0.1:{self.port}/api/status"
        with urllib.request.urlopen(url) as resp:
            self.assertEqual(resp.status, 200)
            data = json.loads(resp.read().decode("utf-8"))
            self.assertEqual(data["status"], "online")
            self.assertEqual(data["cloud_api_calls"], False)
            self.assertEqual(data["host_binding"], "127.0.0.1")
            self.assertTrue(data["binary_exists"])

    def test_api_benchmarks(self):
        """Verify benchmarks endpoint serves records from CSV"""
        url = f"http://127.0.0.1:{self.port}/api/benchmarks"
        with urllib.request.urlopen(url) as resp:
            self.assertEqual(resp.status, 200)
            data = json.loads(resp.read().decode("utf-8"))
            self.assertGreaterEqual(data["count"], 60)
            self.assertEqual(len(data["records"]), data["count"])

    def test_api_generate_empty_prompt_rejected(self):
        """Verify empty prompt payload is rejected with HTTP 400"""
        url = f"http://127.0.0.1:{self.port}/api/generate"
        req = urllib.request.Request(
            url,
            data=json.dumps({"prompt": ""}).encode("utf-8"),
            headers={"Content-Type": "application/json"}
        )
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            urllib.request.urlopen(req)
        self.assertEqual(ctx.exception.code, 400)

    def test_api_generate_local_cpp_execution(self):
        """Verify end-to-end local inference via C++ binary"""
        url = f"http://127.0.0.1:{self.port}/api/generate"
        payload = {
            "prompt": "What is 1+1? Answer:",
            "model_path": "models/tinyllama-1.1b-chat-int4",
            "device": "CPU",
            "max_tokens": 10,
            "deterministic": True
        }
        req = urllib.request.Request(
            url,
            data=json.dumps(payload).encode("utf-8"),
            headers={"Content-Type": "application/json"}
        )
        with urllib.request.urlopen(req, timeout=30) as resp:
            self.assertEqual(resp.status, 200)
            data = json.loads(resp.read().decode("utf-8"))
            self.assertEqual(data["status"], "success")
            self.assertTrue(len(data["response"]) > 0)
            self.assertIn("Local OpenVINO C++ Binary", data["backend"])

if __name__ == "__main__":
    unittest.main()

