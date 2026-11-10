# Local LLM Inference & Optimization Workbench

[![Phase Status](https://img.shields.io/badge/Phase%2000-READY__FOR__REVIEW-yellow.svg)](docs/PHASE_STATUS.md)
[![License: Apache 2.0](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)
[![Inference: Local C++](https://img.shields.io/badge/Inference-OpenVINO%20GenAI%20C%2B%2B-green.svg)](docs/ARCHITECTURE.md)

**Assignment ID:** SWFW_5S_13  
**Developer:** Ayush Kathal  
**Specification:** [OpenVINO_Local_LLM_Implementation_Blueprint.md](OpenVINO_Local_LLM_Implementation_Blueprint.md)

---

## 1. Project Overview

The **Local LLM Inference & Optimization Workbench** is an engineering project demonstrating deterministic on-device inference of lightweight Large Language Models (1B–3B parameters) on consumer hardware without connecting to cloud APIs.

The core engine is implemented in **C++** using the official **OpenVINO GenAI C++ API** (`ov::genai::LLMPipeline`). The project investigates:
- Multi-turn conversational sessions with local tokenization and context management.
- Transformer Key-Value (KV) cache memory scaling and latency impact.
- Model precision trade-offs (FP16 vs. INT4/INT8 quantization).
- Rigorous performance profiling measuring Time-To-First-Token (TTFT), Time-Per-Output-Token (TPOT), throughput, and memory consumption.

---

## 2. Architecture Overview

```text
                     USER / PROMPT INPUT
                             |
                             v
                  C++ Application Engine
                  - Input validation
                  - Generation configuration
                  - Session history & reset
                             |
                             v
                  OpenVINO GenAI C++ API
                  - ov::genai::LLMPipeline
                  - Local Tokenizer & Detokenizer
                  - Chat template processor
                             |
                             v
                      OpenVINO Runtime
                             |
                   +---------+---------+
                   |                   |
                   v                   v
              Intel CPU           Intel iGPU
           (i7-12700H)          (Iris Xe)
                   \                   /
                    +--------+--------+
                             |
                             v
               Local OpenVINO IR Model (1B–3B)
               - openvino_model.xml / .bin
               - openvino_tokenizer.xml / .bin
               - generation_config.json
```

---

## 3. Hardware Baseline

Audited and verified on local development system:
- **CPU:** 12th Gen Intel(R) Core(TM) i7-12700H (14 cores, 20 logical threads)
- **RAM:** 16 GB physical RAM
- **GPU 1:** Intel(R) Iris(R) Xe Graphics (Integrated)
- **GPU 2:** NVIDIA GeForce RTX 3050 Laptop GPU (Discrete, 4 GB VRAM)
- **OS:** Windows 11 Home Single Language (64-bit)

Detailed hardware and device queries are documented in [docs/ENVIRONMENT.md](docs/ENVIRONMENT.md).

---

## 4. Phase-Gated Engineering Contract

This repository enforces a strict phase-gate protocol. No phase is marked complete without meeting every acceptance criterion, executing verification tests, committing, pushing, and obtaining formal review.

| Phase | Description | Status | Report |
| :--- | :--- | :--- | :--- |
| **00** | Workspace audit, Git repository isolation, and contract setup | **READY_FOR_REVIEW** | [PHASE_00_REPORT.md](docs/phase-reports/PHASE_00_REPORT.md) |
| **01** | C++ toolchain & OpenVINO GenAI environment configuration | NOT_STARTED | --- |
| **02** | Model selection, download, and OpenVINO conversion | NOT_STARTED | --- |
| **03** | Local C++ inference engine (`ov::genai::LLMPipeline`) | NOT_STARTED | --- |
| **04** | Chat sessions & generation controls | NOT_STARTED | --- |
| **05** | KV cache characterization & context experiments | NOT_STARTED | --- |
| **06** | Quantization & model-quality comparison | NOT_STARTED | --- |
| **07** | Benchmarking & resource profiling | NOT_STARTED | --- |
| **08** | Optional local interface & usability polish | NOT_STARTED | --- |
| **09** | Final hardening, documentation & submission packaging | NOT_STARTED | --- |

See [docs/PHASE_STATUS.md](docs/PHASE_STATUS.md) for live gate status.

---

## 5. Security & Privacy Boundary

- **Local Execution:** 100% of LLM inference is processed locally on the device.
- **Zero Cloud API Calls:** No remote LLM services, external inference proxies, or unauthorized telemetry are utilized.
- **Asset Protection:** Model weights, caches, build artifacts, and secrets are excluded from version control via `.gitignore`.
