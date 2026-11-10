# Local Model Storage

This directory holds local OpenVINO Intermediate Representation (IR) models and tokenizer assets.

## Git Tracking Policy
Model weights, `.bin`, `.safetensors`, `.onnx`, `.xml`, and related binary assets are excluded from Git version control via `.gitignore`.
Only this `README.md` is committed.

## Planned Model Architecture
- **Primary Model Candidate:** `TinyLlama/TinyLlama-1.1B-Chat-v1.0`
- **Parameter Count:** 1.1 Billion
- **Target Precision:** FP16 baseline, INT4/INT8 quantized variants for comparative analysis
- **Export Workflow:** Managed via `optimum-cli export openvino` in Phase 2
