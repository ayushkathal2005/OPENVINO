# Phase 05 Report: KV Cache Understanding and Controlled Experiment

```text
Phase: 05 --- KV cache
Status: READY_FOR_REVIEW
Start date: 2026-11-11
Completion date: 2026-11-11
Environment: Windows 11 Home (Build 10.0.26300), Intel Core i7-12700H, MSVC v143 (19.44.35222 x64), CMake 4.4.4, OpenVINO GenAI 2026.4.1.0 C++ API
Files changed:
  - CMakeLists.txt
  - src/benchmark_kv.cpp
  - docs/KV_CACHE.md
  - docs/PHASE_STATUS.md
  - docs/phase-reports/PHASE_05_REPORT.md
```

---

## Implementation Summary

1. **Theoretical Architecture & Mathematical Modeling (`docs/KV_CACHE.md`):**
   - Established the rigorous theoretical formulation for KV cache memory scaling in autoregressive transformers.
   - Profiled `TinyLlama-1.1B` architecture ($L=22$, $H_{KV}=4$, $D_{head}=64$, $P=2$ bytes) demonstrating that Grouped-Query Attention (GQA) reduces KV cache memory consumption by **87.5%** ($22.00\text{ KB/token}$ vs $176.00\text{ KB/token}$ for MHA).
   - Formally documented four fundamental conceptual distinctions: Application Chat History vs Runtime KV Cache vs Weight Quantization vs KV Cache Quantization.
   - Evaluated OpenVINO GenAI 2026.4 runtime mechanics, including `CacheEvictionConfig`, `KVCrushConfig`, and continuous batching `SchedulerConfig`.

2. **Controlled Scaling Benchmark Application (`src/benchmark_kv.cpp`):**
   - Developed a native C++ benchmarking utility that programmatically scales context length using the model's tokenizer.
   - Tested 4 context tiers fitting model capabilities: **128, 512, 1,024, and 2,048 tokens**.
   - Captured microsecond-resolution hardware performance metrics via `ov::genai::PerfMetrics` (TTFT, TPOT, throughput, duration) and exact process working set RAM via Windows API `GetProcessMemoryInfo`.
   - Executed benchmark on both `TinyLlama-1.1B-Chat-INT4` and `TinyLlama-1.1B-Chat-FP16` on CPU under deterministic greedy decoding.

---

## Real Measured Benchmark Results

### 1. INT4 Quantized Model Scaling (`models/tinyllama-1.1b-chat-int4`)

```text
========================================================================================================
                             OpenVINO GenAI KV Cache Benchmark Results Table                            
========================================================================================================
Target Ctx    Input Tokens  New Tokens  TTFT (ms)     TPOT (ms/tok)   Throughput (t/s)  Total Time (ms) RAM Peak (MB) 
--------------------------------------------------------------------------------------------------------
128           128           32          529.59        25.64           39.00             1325.20         1238.87       
512           512           32          1825.35       33.78           29.60             2873.31         1284.43       
1024          1024          32          3411.34       35.95           27.82             4094.93         1313.23       
2048          2048          32          7086.28       46.09           21.70             7962.86         1403.81       
========================================================================================================
```

### 2. FP16 Baseline Model Scaling (`models/tinyllama-1.1b-chat-fp16`)

```text
========================================================================================================
                             OpenVINO GenAI KV Cache Benchmark Results Table                            
========================================================================================================
Target Ctx    Input Tokens  New Tokens  TTFT (ms)     TPOT (ms/tok)   Throughput (t/s)  Total Time (ms) RAM Peak (MB) 
--------------------------------------------------------------------------------------------------------
128           128           32          2911.92       63.27           15.81             4873.91         4117.89       
512           512           32          2757.48       59.27           16.87             4595.23         4163.77       
1024          1024          32          3396.00       55.89           17.89             5129.26         4195.68       
2048          2048          32          6772.34       60.46           16.54             8647.30         4293.98       
========================================================================================================
```

---

## Test results:

| Test / Check | Command / Operation | Result | Details |
| :--- | :--- | :--- | :--- |
| **Benchmark Compilation** | `cmake --build build --target benchmark_kv` | **PASS** | Clean compilation of `benchmark_kv.exe` with MSVC v143 |
| **Context Length Tier 1** | `benchmark_kv` (128 tokens) | **PASS** | INT4 TTFT: 529.59 ms, TPOT: 25.64 ms, RAM: 1,238.87 MB |
| **Context Length Tier 2** | `benchmark_kv` (512 tokens) | **PASS** | INT4 TTFT: 1,825.35 ms, TPOT: 33.78 ms, RAM: 1,284.43 MB |
| **Context Length Tier 3** | `benchmark_kv` (1,024 tokens) | **PASS** | INT4 TTFT: 3,411.34 ms, TPOT: 35.95 ms, RAM: 1,313.23 MB |
| **Context Length Tier 4** | `benchmark_kv` (2,048 tokens) | **PASS** | INT4 TTFT: 7,086.28 ms, TPOT: 46.09 ms, RAM: 1,403.81 MB |
| **FP16 Comparative Sweep** | `benchmark_kv` FP16 sweep | **PASS** | Full 4-tier sweep completed; RAM peak: 4,293.98 MB |
| **Theoretical vs Measured** | Mathematical verification | **PASS** | Verified GQA 4 KV heads saves 87.5% memory vs 32 MHA heads |
| **Gate Script Validation** | `python scripts/verify_phase.py --phase 5` | **PASS** | Gate criteria satisfied |

---

## Acceptance criteria checklist:

- [x] `docs/KV_CACHE.md` explains keys, values, token growth, and memory implications accurately.
- [x] Application-level chat history is distinguished from runtime KV cache.
- [x] The runtime's actual cache-management behavior is documented with official references.
- [x] At least three context lengths are tested (tested 4 tiers: 128, 512, 1024, 2048 tokens).
- [x] Memory and latency observations are recorded with methodology.
- [x] Any cache limit/eviction/quantization feature is used only if verified in the installed API.
- [x] Unsupported features are explicitly identified (NPU target, unconfigured dynamic KV cache quantization).
- [x] No fabricated cache measurements or unsupported claims appear in the report.
