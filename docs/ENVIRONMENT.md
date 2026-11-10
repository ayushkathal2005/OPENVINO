# Environment & Hardware Audit

**Assignment ID:** SWFW_5S_13  
**Project:** Local LLM Inference & Optimization Workbench  
**Audit Date:** 2026-11-10  
**Status:** Verified through system CIM/WMI queries and CLI inspection

---

## 1. Host Machine Specifications

| Property | Value | Verification Method |
| :--- | :--- | :--- |
| **Operating System** | Microsoft Windows 11 Home Single Language | `Win32_OperatingSystem` |
| **OS Version / Build** | 10.0.26300 (64-bit) | `Win32_OperatingSystem.Version` |
| **CPU Model** | 12th Gen Intel(R) Core(TM) i7-12700H | `Win32_Processor.Name` |
| **Physical Cores** | 14 cores (6 Performance-cores + 8 Efficient-cores) | `Win32_Processor.NumberOfCores` |
| **Logical Processors** | 20 threads | `Win32_Processor.NumberOfLogicalProcessors` |
| **Base / Max Clock** | 2.30 GHz nominal base | `Win32_Processor.MaxClockSpeed` |
| **Total Physical RAM** | 16.0 GB (16,388,772 KB visible) | `Win32_OperatingSystem.TotalVisibleMemorySize` |
| **Primary Storage (C:)** | 195.9 GB free / 564.6 GB capacity (NTFS) | `Get-PSDrive C` |

---

## 2. Graphic Processing Units & Accelerators

| Device Name | Architecture / Type | Driver Version | Memory | OpenVINO Compatibility Status |
| :--- | :--- | :--- | :--- | :--- |
| **Intel(R) Iris(R) Xe Graphics** | Integrated GPU (Intel Gen12 Xe-LP) | 32.0.101.6790 | 2.0 GB shared adapter RAM | Supported target (`GPU.0` via OpenVINO GPU plugin) |
| **NVIDIA GeForce RTX 3050 Laptop GPU** | Discrete GPU (Ampere GA107) | 32.0.15.8142 | 4.0 GB dedicated GDDR6 | Third-party device; OpenVINO baseline will prioritize CPU / Intel GPU |
| **NPU (Neural Processing Unit)** | N/A | None | N/A | **UNAVAILABLE** (Intel 12th Gen Alder Lake lacks integrated NPU) |

> [!NOTE]
> Device target defaults to **CPU** as the mandatory deterministic baseline. Intel Iris Xe Graphics will be evaluated only after C++ CPU inference is verified.

---

## 3. Developer Toolchain & Runtime State

| Tool | Detected Version | Location / Source | Phase Action Required |
| :--- | :--- | :--- | :--- |
| **Git** | `2.45.1.windows.1` | `C:\Program Files\Git\cmd\git.exe` | Verified; active repository |
| **GitHub CLI (`gh`)** | Authenticated (`ayushkathal2005`) | `C:\Program Files\GitHub CLI\gh.exe` | Verified; repo `ayushkathal2005/OPENVINO` synced |
| **CMake** | `4.4.4` (x86_64) | `C:\Program Files\CMake\bin\cmake.exe` | Installed & verified in Phase 01 |
| **C++ Compiler (MSVC)** | MSVC `19.44.35222` (x64) | `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe` | Installed & verified in Phase 01 |
| **Windows SDK** | `10.0.26100.0` | `C:\Program Files (x86)\Windows Kits\10` | Installed & verified in Phase 01 |
| **OpenVINO Runtime** | `2026.4.1-22982-e213a147257-releases/2026/4` | `C:\openvino_genai\openvino_genai_windows_2026.4.1.0_x86_64\runtime` | Installed & verified in Phase 01 |
| **OpenVINO GenAI C++** | `2026.4.1.0` | `C:\openvino_genai\openvino_genai_windows_2026.4.1.0_x86_64` | Installed & verified in Phase 01 |
| **Intel oneTBB** | `tbb12.dll` | `C:\openvino_genai\openvino_genai_windows_2026.4.1.0_x86_64\runtime\3rdparty\tbb\bin` | Verified in Phase 01 |
| **System Python** | `3.14.0` | System PATH | Model preparation tools in Phase 02 will run in dedicated venv |

---

## 4. Git Repository & Remote Configuration

- **Repository Root:** `C:\Users\ayush\Desktop\OPENVINO`
- **Initial Branch:** `main`
- **Configured Remote (`origin`):** `https://github.com/ayushkathal2005/OPENVINO.git`
- **Isolation Status:** Fully isolated from host drive root (`C:\`).

---

## 5. Project Scope & Boundaries

### In Scope
1. Local C++ inference engine using official `ov::genai::LLMPipeline`.
2. Model export and preparation for a supported 1B–3B model (`TinyLlama-1.1B-Chat-v1.0`).
3. Precision comparison (FP16 baseline vs. INT4/INT8 quantized).
4. KV-cache memory and latency characterization under varying context lengths.
5. Deterministic benchmarking reporting TTFT, TPOT, throughput, and memory consumption.
6. Phase-gated Git commits with remote push verification.

### Strictly Out of Scope
1. Cloud LLM API inference (strictly prohibited; all inference runs on local OpenVINO C++).
2. Model fine-tuning or pre-training.
3. Heavy web frameworks, React dashboards, or distributed microservices.
4. Multi-agent / vector database / RAG frameworks before completing core requirements.
5. Committing large model weights, binary dumps, or build artifacts to Git.

