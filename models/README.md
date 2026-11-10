# Local Model Storage & Export Instructions

This directory holds local OpenVINO Intermediate Representation (IR) models and tokenizer assets.

## Git Tracking Policy
Model weights, `.bin`, `.safetensors`, `.onnx`, `.xml`, and related binary assets are excluded from Git version control via `.gitignore`.
Only this `README.md` is committed.

---

## Model Preparation Workflow

Exporting models is automated through [`scripts/export_model.py`](../scripts/export_model.py) using the Python 3.11 virtual environment (`.venv`).

### 1. Baseline Model (FP16)
To export the 1.1B parameter FP16 baseline model:
```powershell
& ".\.venv\Scripts\python.exe" scripts/export_model.py `
  --model-id "TinyLlama/TinyLlama-1.1B-Chat-v1.0" `
  --output-dir "models/tinyllama-1.1b-chat-fp16" `
  --precision FP16
```

### 2. Quantized Model (INT4)
To export the INT4 compressed model for comparative analysis:
```powershell
& ".\.venv\Scripts\python.exe" scripts/export_model.py `
  --model-id "TinyLlama/TinyLlama-1.1B-Chat-v1.0" `
  --output-dir "models/tinyllama-1.1b-chat-int4" `
  --precision INT4
```

---

## Required Model Assets Verification
Each exported model directory must contain:
- `openvino_model.xml` — Model topology graph
- `openvino_model.bin` — Optimized model weights
- `openvino_tokenizer.xml` / `openvino_tokenizer.bin` — Tokenizer execution graph
- `openvino_detokenizer.xml` / `openvino_detokenizer.bin` — Detokenizer execution graph
- `config.json` — Model configuration parameters
- `generation_config.json` — Default generation parameters
