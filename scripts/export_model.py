#!/usr/bin/env python3
"""
Reproducible Model Export Script for OpenVINO GenAI
Exports supported Hugging Face models to OpenVINO Intermediate Representation (IR).
"""

import sys
import os
import time
import argparse
import subprocess
from pathlib import Path

REQUIRED_ASSETS = [
    "openvino_model.xml",
    "openvino_model.bin",
    "openvino_tokenizer.xml",
    "openvino_tokenizer.bin",
    "openvino_detokenizer.xml",
    "openvino_detokenizer.bin",
    "tokenizer.json",
    "config.json"
]

def verify_exported_assets(output_dir: Path):
    print(f"\n[INFO] Verifying exported assets in {output_dir}...")
    missing = []
    total_bytes = 0
    
    if not output_dir.exists():
        print(f"[FAIL] Output directory does not exist: {output_dir}")
        return False, 0, ["Directory missing"]

    found_files = list(output_dir.iterdir())
    for f in found_files:
        if f.is_file():
            size = f.stat().st_size
            total_bytes += size
            print(f"  + {f.name:32s} ({size / (1024*1024):.2f} MB)")

    # Check minimum required model weights and tokenizer configs
    if not (output_dir / "openvino_model.xml").exists():
        missing.append("openvino_model.xml")
    if not (output_dir / "openvino_model.bin").exists():
        missing.append("openvino_model.bin")
    if not (output_dir / "config.json").exists():
        missing.append("config.json")

    has_tokenizer = (
        (output_dir / "openvino_tokenizer.xml").exists() or
        (output_dir / "tokenizer.json").exists() or
        (output_dir / "tokenizer_config.json").exists()
    )
    if not has_tokenizer:
        missing.append("tokenizer assets")

    if missing:
        print(f"[FAIL] Missing required assets: {missing}")
        return False, total_bytes, missing

    print(f"[PASS] Asset validation passed. Total size: {total_bytes / (1024*1024):.2f} MB")
    return True, total_bytes, []

def export_model(model_id: str, output_dir: Path, precision: str):
    output_dir.mkdir(parents=True, exist_ok=True)
    print("==================================================")
    print(f"  Exporting Model to OpenVINO IR")
    print(f"  Model ID:  {model_id}")
    print(f"  Precision: {precision}")
    print(f"  Target:    {output_dir}")
    print("==================================================")

    # Resolve python in active virtual environment if present
    python_exe = sys.executable

    cmd = [
        python_exe, "-m", "optimum.commands.optimum_cli",
        "export", "openvino",
        "--model", model_id,
        "--task", "text-generation-with-past",
        "--trust-remote-code"
    ]

    precision_upper = precision.upper()
    if precision_upper == "FP16":
        cmd.extend(["--weight-format", "fp16"])
    elif precision_upper == "INT8":
        cmd.extend(["--weight-format", "int8"])
    elif precision_upper == "INT4":
        cmd.extend(["--weight-format", "int4"])
    elif precision_upper == "FP32":
        cmd.extend(["--weight-format", "fp32"])
    else:
        print(f"[ERROR] Unsupported precision format: {precision}")
        return False

    cmd.append(str(output_dir))

    print(f"[EXEC] {' '.join(cmd)}\n")
    start_time = time.time()
    result = subprocess.run(cmd)
    elapsed = time.time() - start_time

    if result.returncode != 0:
        print(f"\n[FAIL] Model export command exited with non-zero code: {result.returncode}")
        return False

    print(f"\n[SUCCESS] Model export completed in {elapsed:.2f} seconds.")
    valid, size_bytes, missing = verify_exported_assets(output_dir)
    return valid

def main():
    parser = argparse.ArgumentParser(description="Export Hugging Face model to OpenVINO IR.")
    parser.add_argument("--model-id", default="TinyLlama/TinyLlama-1.1B-Chat-v1.0", help="Hugging Face model ID")
    parser.add_argument("--output-dir", default="models/tinyllama-1.1b-chat-fp16", help="Destination directory")
    parser.add_argument("--precision", default="FP16", choices=["FP16", "INT4", "INT8", "FP32"], help="Model weight precision")
    args = parser.parse_args()

    success = export_model(args.model_id, Path(args.output_dir), args.precision)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()

