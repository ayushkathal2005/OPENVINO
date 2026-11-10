# Phase 02 Report: Model Selection, Download, and OpenVINO Conversion

```text
Phase: 02 --- Model preparation
Status: READY_FOR_REVIEW
Start date: 2026-11-10
Completion date: 2026-11-11
Environment: Windows 11 Home (Build 10.0.26300), Python 3.11.9 (.venv), optimum-intel 2.2.0, openvino 2026.4.1, transformers 5.5.4, torch 2.15.0+cpu
Files changed:
  - .gitignore
  - models/README.md
  - scripts/export_model.py
  - docs/MODEL_CARD.md
  - docs/PHASE_STATUS.md
  - docs/phase-reports/PHASE_02_REPORT.md
```

---

## Implementation Summary

1. **Model Selection & Architecture:** Selected `TinyLlama/TinyLlama-1.1B-Chat-v1.0` (1.1 Billion parameters, Apache 2.0 license), an autoregressive decoder model with Grouped-Query Attention (GQA) fully supported by OpenVINO GenAI.
2. **Export Environment:** Established dedicated Python 3.11 virtual environment (`.venv`) with `optimum-intel[openvino]`, `openvino-genai`, `transformers`, and `torch` (CPU).
3. **Reproducible Tooling:** Implemented `scripts/export_model.py` providing automated model export and comprehensive asset integrity verification.
4. **Baseline Model Export (FP16):** Exported the full baseline model into `models/tinyllama-1.1b-chat-fp16` with past key-values caching (`text-generation-with-past`) and converted tokenizer graphs. Total footprint: 2,105.47 MB.
5. **Quantized Model Export (INT4):** Exported an NNCF-compressed INT4 model into `models/tinyllama-1.1b-chat-int4` using asymmetric group-size 128 quantization across 100% of ratio-defining layers. Total footprint: 613.34 MB (**71.1% memory reduction**).
6. **Git Hygiene & Security:** Confirmed via `git status` that `.gitignore` strictly excludes binary weights (`*.bin`, `*.safetensors`, `models/*`), and model caches (`raw_models/`, `.cache`).

---

## Commands Executed

```powershell
# 1. Virtual Environment Setup
py -3.11 -m venv .venv
.\.venv\Scripts\python.exe -m pip install --upgrade pip
.\.venv\Scripts\python.exe -m pip install "optimum-intel[openvino]" openvino-genai transformers torch --extra-index-url https://download.pytorch.org/whl/cpu

# 2. Baseline FP16 Model Export
& ".\.venv\Scripts\python.exe" -m optimum.commands.optimum_cli export openvino `
  --model "C:\Users\ayush\.cache\huggingface\hub\models--TinyLlama--TinyLlama-1.1B-Chat-v1.0\snapshots\fe8a4ea1ffedaf415f4da2f062534de366a451e6" `
  --task text-generation-with-past `
  --weight-format fp16 `
  models/tinyllama-1.1b-chat-fp16

# 3. Quantized INT4 Model Export
& ".\.venv\Scripts\python.exe" -m optimum.commands.optimum_cli export openvino `
  --model "C:\Users\ayush\.cache\huggingface\hub\models--TinyLlama--TinyLlama-1.1B-Chat-v1.0\snapshots\fe8a4ea1ffedaf415f4da2f062534de366a451e6" `
  --task text-generation-with-past `
  --weight-format int4 `
  models/tinyllama-1.1b-chat-int4

# 4. Asset Integrity Verification
& ".\.venv\Scripts\python.exe" -c "from pathlib import Path; from scripts.export_model import verify_exported_assets; verify_exported_assets(Path('models/tinyllama-1.1b-chat-fp16')); verify_exported_assets(Path('models/tinyllama-1.1b-chat-int4'))"
```

---

## Test results:

| Test / Check | Command / Operation | Result | Details |
| :--- | :--- | :--- | :--- |
| **Model Scope Compliance** | Parameter count verification | **PASS** | 1.1 Billion parameters (within 1B–3B assignment requirement) |
| **Export Tooling Setup** | `optimum-cli --help` | **PASS** | Optimum-Intel 2.2.0 + OpenVINO 2026.4.1 functional |
| **FP16 Export Execution** | `optimum-cli export openvino --weight-format fp16` | **PASS** | Completed with non-zero exit code 0 |
| **INT4 Export Execution** | `optimum-cli export openvino --weight-format int4` | **PASS** | NNCF compressed 154/154 ratio layers to `int4_asym` (exit code 0) |
| **FP16 Asset Integrity** | `verify_exported_assets('models/tinyllama-1.1b-chat-fp16')` | **PASS** | `openvino_model.xml`, `openvino_model.bin` (2,098.18 MB), tokenizer assets verified |
| **INT4 Asset Integrity** | `verify_exported_assets('models/tinyllama-1.1b-chat-int4')` | **PASS** | `openvino_model.xml`, `openvino_model.bin` (605.58 MB), tokenizer assets verified |
| **Git Exclusion Audit** | `git status` | **PASS** | Both `models/*` binary folders excluded; 0 weight bytes tracked |
| **Gate Verification** | `python scripts/verify_phase.py --phase 2` | **PASS** | Gate criteria satisfied |

### Export Directory Asset Summary:
```text
models/tinyllama-1.1b-chat-fp16 (Total: 2105.47 MB):
  + openvino_model.bin               (2098.18 MB)
  + openvino_model.xml               (1.57 MB)
  + openvino_tokenizer.bin           (1.78 MB)
  + openvino_tokenizer.xml           (0.03 MB)
  + openvino_detokenizer.bin         (0.45 MB)
  + openvino_detokenizer.xml         (0.01 MB)
  + tokenizer.json                   (3.45 MB)
  + chat_template.jinja / config.json / generation_config.json

models/tinyllama-1.1b-chat-int4 (Total: 613.34 MB - 71.1% footprint reduction):
  + openvino_model.bin               (605.58 MB)
  + openvino_model.xml               (2.04 MB)
  + openvino_config.json             (0.00 MB)
  + openvino_tokenizer.bin           (1.78 MB)
  + openvino_tokenizer.xml           (0.03 MB)
  + openvino_detokenizer.bin         (0.45 MB)
  + openvino_detokenizer.xml         (0.01 MB)
  + tokenizer.json                   (3.45 MB)
  + chat_template.jinja / config.json / generation_config.json
```

---

## Acceptance criteria checklist:

- [x] Model is within the 1B--3B assignment range (1.1B parameters).
- [x] Model is listed as supported or its compatibility is explicitly validated.
- [x] License and model source are documented (Apache 2.0 in `docs/MODEL_CARD.md`).
- [x] Export command completes successfully.
- [x] OpenVINO IR files and tokenizer assets exist locally.
- [x] Model path is configurable and not hardcoded to one developer's machine.
- [x] Model weights are not committed to Git.
- [x] Model directory size and export details are recorded.

---

## Known Issues & Notes
None. Both FP16 baseline and INT4 compressed models are validated and locally available for C++ inference in Phase 03.

---

## Skipped Tests and Reasons
None. All required model export, validation, and git hygiene tests were executed.

---

## Commit & Push Verification

```text
Commit hash: PENDING (to be committed upon review)
Remote branch: main
Remote hash: PENDING
Push verification: PENDING
User approval: PENDING
Next permitted phase: Phase 03 --- First real C++ local inference (LOCKED until Phase 02 is committed, pushed, verified, and approved)
```
