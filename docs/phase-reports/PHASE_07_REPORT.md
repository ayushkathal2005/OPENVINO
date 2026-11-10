# Phase 07 Report: Benchmarking and Resource Profiling

```text
Phase: 07 --- Benchmarking
Status: READY_FOR_REVIEW
Start date: 2026-11-11
Completion date: 2026-11-11
Environment: Windows 11 Home (Build 10.0.26300), Intel Core i7-12700H, Intel Iris Xe Graphics (GPU.0), MSVC v143 (19.44.35222 x64), CMake 4.4.4, OpenVINO GenAI 2026.4.1.0 C++ API
Files changed:
  - CMakeLists.txt
  - src/benchmark_suite.cpp
  - docs/BENCHMARK_METHODOLOGY.md
  - benchmarks/results/benchmark_results.csv
  - docs/PHASE_STATUS.md
  - docs/phase-reports/PHASE_07_REPORT.md
```

---

## Implementation Summary

1. **Benchmarking Harness (`src/benchmark_suite.cpp`):**
   - Engineered an automated C++ multi-repetition benchmark suite measuring cold model load time, warm-up runs, and multi-run repeated generation across three distinct hardware/model configurations:
     - `CPU_INT4`: Intel Core i7-12700H with NNCF asymmetric INT4 model.
     - `CPU_FP16`: Intel Core i7-12700H with full FP16 baseline model.
     - `GPU.0_INT4`: Intel Iris Xe Graphics (96 EUs) with NNCF asymmetric INT4 model.
   - Profiled all four fixed tasks from `benchmarks/prompts.json` with **5 repetitions per configuration** (60 total individual benchmark runs).
   - Captured microsecond-precision hardware metrics via `ov::genai::PerfMetrics` and process working set RAM via Win32 `GetProcessMemoryInfo`.
   - Exported complete raw per-iteration dataset directly to `benchmarks/results/benchmark_results.csv`.

2. **Benchmarking Methodology Documentation (`docs/BENCHMARK_METHODOLOGY.md`):**
   - Authored formal documentation specifying hardware testbed, metrics definitions, measurement methodology, warm-up protocol, CSV schema, and exact reproduction steps.

---

## Real Statistical Benchmark Results (Mean $\pm$ StdDev)

*Dataset: 60 measured runs across 4 prompts $\times$ 5 repetitions $\times$ 3 configurations (Greedy, 64 new tokens)*

```text
========================================================================================================================
                               Statistical Performance Summary Table (Mean +/- StdDev)                                  
========================================================================================================================
Configuration Prompt ID   N       TTFT (ms)             TPOT (ms/tok)         Throughput (t/s)      RAM Peak (MB)     
------------------------------------------------------------------------------------------------------------------------
CPU_INT4      fact_01     5       78.1 +/- 40.5         22.7 +/- 1.2          44.2 +/- 2.3          1225.6            
CPU_INT4      summ_01     5       62.9 +/- 42.6         23.6 +/- 3.0          43.0 +/- 5.3          1229.5            
CPU_INT4      instruct_01 5       81.1 +/- 38.6         21.1 +/- 0.4          47.4 +/- 1.0          1237.9            
CPU_INT4      reason_01   5       89.4 +/- 54.1         21.5 +/- 0.5          46.5 +/- 1.0          1240.8            
------------------------------------------------------------------------------------------------------------------------
CPU_FP16      fact_01     5       278.2 +/- 148.9       73.5 +/- 1.9          13.6 +/- 0.4          4109.6            
CPU_FP16      summ_01     5       159.7 +/- 68.3        57.3 +/- 2.1          17.5 +/- 0.6          4114.1            
CPU_FP16      instruct_01 5       210.5 +/- 105.0       59.6 +/- 4.8          16.9 +/- 1.3          4122.8            
CPU_FP16      reason_01   5       187.6 +/- 76.5        56.2 +/- 0.9          17.8 +/- 0.3          4126.3            
------------------------------------------------------------------------------------------------------------------------
GPU.0_INT4    fact_01     5       87.0 +/- 28.3         25.0 +/- 2.9          40.4 +/- 4.5          1108.0            
GPU.0_INT4    summ_01     5       57.7 +/- 21.7         22.4 +/- 0.6          44.6 +/- 1.2          1057.8            
GPU.0_INT4    instruct_01 5       43.2 +/- 43.2         22.1 +/- 0.5          45.2 +/- 0.9          1058.0            
GPU.0_INT4    reason_01   5       41.8 +/- 38.8         22.8 +/- 0.4          43.9 +/- 0.7          1057.8            
========================================================================================================================
```

---

## High-Level Configuration Comparison Summary

| Metric | CPU INT4 | CPU FP16 | GPU.0 INT4 (Iris Xe) | INT4 vs FP16 Advantage |
| :--- | :---: | :---: | :---: | :---: |
| **Cold Model Load Time** | 2,187.86 ms | 1,434.85 ms | 6,318.77 ms | FP16 loads faster (direct tensor map) |
| **Overall Mean TTFT** | 77.9 ms | 209.0 ms | 57.4 ms | **GPU.0 is 3.6x faster** than CPU FP16 |
| **Overall Mean TPOT** | 22.2 ms/token | 61.6 ms/token | 23.1 ms/token | **CPU INT4 is 2.8x faster** than CPU FP16 |
| **Overall Mean Throughput** | 45.3 tokens/s | 16.4 tokens/s | 43.5 tokens/s | **INT4 delivers +176% throughput** |
| **Peak Resident RAM** | 1,233.4 MB | 4,118.2 MB | 1,070.4 MB | **INT4 saves 2,885 MB RAM (-70%)** |

---

## Test results:

| Test / Check | Command / Operation | Result | Details |
| :--- | :--- | :--- | :--- |
| **Benchmark Suite Build** | `cmake --build build --target benchmark_suite` | **PASS** | Clean build of `benchmark_suite.exe` |
| **Multi-Run Repetitions** | `benchmark_suite.exe 5` | **PASS** | Completed 60 individual measured runs (5 per prompt) |
| **CPU INT4 Profiling** | 5 reps $\times$ 4 prompts on CPU | **PASS** | Mean throughput: 45.3 t/s, Mean TTFT: 77.9 ms |
| **CPU FP16 Profiling** | 5 reps $\times$ 4 prompts on CPU | **PASS** | Mean throughput: 16.4 t/s, Mean TTFT: 209.0 ms |
| **GPU.0 INT4 Profiling** | 5 reps $\times$ 4 prompts on GPU.0 | **PASS** | Mean throughput: 43.5 t/s, Peak RAM: 1,070.4 MB |
| **CSV Export Verification** | File inspection `benchmark_results.csv` | **PASS** | 60 valid data rows written with complete headers |
| **Statistical Validity** | Mean & Sample StdDev computation | **PASS** | Standard deviation calculated for all primary metrics |
| **Gate Script Validation** | `python scripts/verify_phase.py --phase 7` | **PASS** | Gate criteria satisfied |

---

## Acceptance criteria checklist:

- [x] Benchmark prompts and settings are version-controlled (`benchmarks/prompts.json`).
- [x] At least five measured repetitions are completed per primary configuration ($N=5$ across 3 configurations, 60 runs total).
- [x] Warm-up and cold-start methodology is documented (`docs/BENCHMARK_METHODOLOGY.md`).
- [x] Raw results are saved as CSV (`benchmarks/results/benchmark_results.csv`).
- [x] Mean and standard deviation are reported for all primary latency and throughput metrics.
- [x] Metrics are sourced from the runtime (`ov::genai::PerfMetrics`) or OS monitor (`GetProcessMemoryInfo`).
- [x] Comparisons change one major variable at a time (Precision: INT4 vs FP16 on CPU; Device: CPU vs GPU.0 on INT4).
- [x] Results can be regenerated using documented commands (`.\build\Release\benchmark_suite.exe 5`).
- [x] Limitations and device boundaries are transparent (Intel Iris Xe supported, NVIDIA dGPU not targeted by OpenVINO GenAI, NPU unavailable).
