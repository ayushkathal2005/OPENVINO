#!/usr/bin/env python3
"""
Local UI Bridge Server for OpenVINO Local LLM Workbench
Binds STRICTLY to 127.0.0.1 (localhost only).
Executes inference exclusively through local C++ binary: build/Release/llm_inference.exe.
"""

import sys
import os
import json
import subprocess
import time
from pathlib import Path
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse

REPO_ROOT = Path(__file__).resolve().parent.parent
BUILD_EXE = REPO_ROOT / "build" / "Release" / "llm_inference.exe"
CSV_PATH = REPO_ROOT / "benchmarks" / "results" / "benchmark_results.csv"

# Configure OpenVINO GenAI runtime DLL paths in environment
OPENVINO_BIN = Path(r"C:\openvino_genai\openvino_genai_windows_2026.4.1.0_x86_64\runtime\bin\intel64\Release")
TBB_BIN = Path(r"C:\openvino_genai\openvino_genai_windows_2026.4.1.0_x86_64\runtime\3rdparty\tbb\bin")

current_path = os.environ.get("PATH", "")
if str(OPENVINO_BIN) not in current_path:
    os.environ["PATH"] = f"{OPENVINO_BIN};{TBB_BIN};{current_path}"

class LocalWorkbenchHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        # Override to keep terminal output clean
        sys.stderr.write(f"[LocalServer] {self.address_string()} - {format % args}\n")

    def send_json(self, status_code, data):
        payload = json.dumps(data).encode("utf-8")
        self.send_response(status_code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Access-Control-Allow-Origin", "http://127.0.0.1:8080")
        self.end_headers()
        self.wfile.write(payload)

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", "http://127.0.0.1:8080")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path

        if path in ["/", "/index.html"]:
            html_path = REPO_ROOT / "ui" / "index.html"
            if not html_path.exists():
                self.send_error(404, "UI template not found")
                return
            content = html_path.read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(content)))
            self.end_headers()
            self.wfile.write(content)
            return

        if path == "/api/status":
            info = {
                "status": "online",
                "backend": "OpenVINO GenAI 2026.4.1 C++ API",
                "binary": str(BUILD_EXE),
                "binary_exists": BUILD_EXE.exists(),
                "devices": ["CPU", "GPU.0 (Iris Xe)", "GPU.1 (RTX 3050)"],
                "default_device": "CPU",
                "models": [
                    {"id": "int4", "name": "TinyLlama 1.1B Chat (INT4 Compressed)", "path": "models/tinyllama-1.1b-chat-int4"},
                    {"id": "fp16", "name": "TinyLlama 1.1B Chat (FP16 Baseline)", "path": "models/tinyllama-1.1b-chat-fp16"}
                ],
                "cloud_api_calls": False,
                "host_binding": "127.0.0.1"
            }
            self.send_json(200, info)
            return

        if path == "/api/benchmarks":
            if not CSV_PATH.exists():
                self.send_json(404, {"error": "Benchmark CSV not found"})
                return

            rows = []
            with open(CSV_PATH, "r", encoding="utf-8") as f:
                header = f.readline().strip().split(",")
                for line in f:
                    parts = line.strip().split(",")
                    if len(parts) == len(header):
                        row_dict = dict(zip(header, parts))
                        rows.append(row_dict)

            self.send_json(200, {"count": len(rows), "records": rows})
            return

        self.send_error(404, "Endpoint not found")

    def do_POST(self):
        if self.path != "/api/generate":
            self.send_error(404, "Unknown POST endpoint")
            return

        content_length = int(self.headers.get("Content-Length", 0))
        if content_length == 0 or content_length > 1024 * 1024:
            self.send_json(400, {"error": "Invalid request body size"})
            return

        body = self.rfile.read(content_length)
        try:
            req = json.loads(body.decode("utf-8"))
        except json.JSONDecodeError:
            self.send_json(400, {"error": "Invalid JSON payload"})
            return

        prompt = req.get("prompt", "").strip()
        if not prompt:
            self.send_json(400, {"error": "Prompt cannot be empty"})
            return

        model_path = req.get("model_path", "models/tinyllama-1.1b-chat-int4")
        device = req.get("device", "CPU")
        max_tokens = int(req.get("max_tokens", 64))
        temperature = float(req.get("temperature", 0.7))
        top_p = float(req.get("top_p", 0.9))
        deterministic = bool(req.get("deterministic", False))

        if not BUILD_EXE.exists():
            self.send_json(500, {"error": f"C++ binary not found at {BUILD_EXE}"})
            return

        cmd = [
            str(BUILD_EXE),
            "--model-path", model_path,
            "--device", device,
            "--prompt", prompt,
            "--max-tokens", str(max_tokens),
            "--temperature", str(temperature),
            "--top-p", str(top_p)
        ]
        if deterministic:
            cmd.append("--deterministic")

        t0 = time.perf_counter()
        try:
            proc = subprocess.run(
                cmd,
                cwd=REPO_ROOT,
                capture_output=True,
                text=True,
                timeout=60,
                env=os.environ
            )
            t_total_ms = (time.perf_counter() - t0) * 1000.0

            if proc.returncode != 0:
                self.send_json(500, {
                    "error": "Local C++ inference engine failed",
                    "stderr": proc.stderr,
                    "stdout": proc.stdout
                })
                return

            # Extract response from standard output
            output = proc.stdout
            response_text = ""
            if "[Assistant]:" in output:
                response_text = output.split("[Assistant]:", 1)[1].strip()
            else:
                response_text = output.strip()

            self.send_json(200, {
                "status": "success",
                "response": response_text,
                "elapsed_ms": round(t_total_ms, 2),
                "model_path": model_path,
                "device": device,
                "max_tokens": max_tokens,
                "deterministic": deterministic,
                "backend": "Local OpenVINO C++ Binary (Zero Cloud)"
            })
        except subprocess.TimeoutExpired:
            self.send_json(504, {"error": "Inference timed out after 60s"})
        except Exception as e:
            self.send_json(500, {"error": f"Subprocess error: {str(e)}"})

def run_server(port=8080):
    server_address = ("127.0.0.1", port)
    httpd = HTTPServer(server_address, LocalWorkbenchHandler)
    print(f"[LocalWorkbench] Server started on http://127.0.0.1:{port} (Strict localhost binding)")
    return httpd

if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    httpd = run_server(port)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n[LocalWorkbench] Server stopped cleanly.")
