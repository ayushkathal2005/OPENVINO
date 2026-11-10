# Local LLM Inference Benchmarking Methodology

**Project:** Local LLM Inference & Optimization Workbench  
**Assignment ID:** SWFW_5S_13  
**Model Family:** TinyLlama-1.1B-Chat-v1.0 (FP16 Baseline & NNCF INT4 Compressed)  
**Runtime:** OpenVINO GenAI 2026.4.1.0 C++ API  
**Host Hardware:** Intel Core i7-12700H (14 cores, 20 threads), 16 GB DDR5 RAM, Intel Iris Xe Graphics (GPU.0), Windows 11 Home  

---

## 1. Overview & Goals

The objective of this benchmarking suite is to provide repeatable, statistically validated performance data for local large language model inference using the OpenVINO GenAI C++ API. 

The benchmark captures:
1. **Cold Initialization vs. Warm Inference Latency.**
2. **Time to First Token (TTFT)** representing prompt prefill latency.
3. **Time Per Output Token (TPOT)** and **Generation Throughput (Tokens per Second)** representing autoregressive decode latency.
4. **Physical Working Set Memory (RAM)** footprint before load, after load, and at peak generation.
5. **Cross-Hardware/Cross-Precision Comparisons:** Comparing CPU INT4, CPU FP16, and GPU.0 INT4 under identical deterministic decoding conditions.

---

## 2. Experimental Testbed & Configuration

| Parameter | Specification |
| :--- | :--- |
| **Operating System** | Windows 11 Home 64-bit (Build 10.0.26300) |
| **Processor (CPU)** | Intel Core i7-12700H (6 P-cores, 8 E-cores, 20 threads, base 2.3 GHz, boost up to 4.7 GHz) |
| **Integrated GPU (GPU.0)**| Intel Iris Xe Graphics (96 Execution Units) |
| **System Memory** | 16 GB DDR5 4800 MHz (Dual-channel) |
| **C++ Toolchain** | MSVC v143 (19.44.35222 x64) via Visual Studio 2022 BuildTools |
| **Build Mode** | Release (`-O2` / `/O2`, `/MD`, C++17) |
| **OpenVINO GenAI** | Official Intel release 2026.4.1.0 (`openvino_genai.dll`, `openvino.dll`, `tbb12.dll`) |

---

## 3. Metrics Definitions & Measurement Instrumentation

All latency metrics are sourced directly from OpenVINO GenAI's native microsecond-precision performance subsystem (`ov::genai::PerfMetrics`), ensuring that measurement overhead does not skew observed inference numbers. Process memory is sampled via the Win32 API (`GetProcessMemoryInfo`).

| Metric | Symbol | Unit | Measurement Method | Description |
| :--- | :---: | :---: | :--- | :--- |
| **Cold Model Load Time** | $T_{load}$ | ms | High-resolution wall clock | Duration from instantiation of `ov::genai::LLMPipeline` until weights and topology are fully mapped and compiled in hardware memory. |
| **Time to First Token** | $\text{TTFT}$ | ms | `PerfMetrics::get_ttft()` | Latency from prompt ingestion to production of the first generated token (prompt prefill phase). |
| **Time Per Output Token**| $\text{TPOT}$ | ms/tok | `PerfMetrics::get_tpot()` | Mean duration required to generate each subsequent token during the autoregressive decode phase. |
| **Decode Throughput** | $\text{TPS}$ | tokens/s | `PerfMetrics::get_throughput()` | Number of generated tokens per second during the generation phase ($\text{TPS} \approx \frac{1000}{\text{TPOT}}$). |
| **Total Generation Time**| $T_{gen}$ | ms | High-resolution wall clock | Total elapsed time for prefill + decode across all output tokens. |
| **Peak Working Set RAM** | $M_{peak}$ | MB | `GetProcessMemoryInfo` (`WorkingSetSize`) | Maximum physical resident memory allocated by the operating system for the running process during execution. |

---

## 4. Benchmark Dataset & Prompt Categories

The benchmark uses a fixed set of four diverse prompts checked into the repository at [`benchmarks/prompts.json`](file:///c:/Users/ayush/Desktop/OPENVINO/benchmarks/prompts.json):

1. **`fact_01` (Factual Explanation):**  
   *"Explain the concept of Key-Value (KV) caching in large language models and why it optimizes autoregressive generation."* (43 input tokens)
2. **`summ_01` (Summarization):**  
   *"Summarize the primary benefits and potential accuracy trade-offs of INT4 weight quantization for edge AI inference."* (39 input tokens)
3. **`instruct_01` (Instruction Following):**  
   *"List exactly three advantages of deploying LLMs locally using OpenVINO GenAI C++ API instead of remote cloud APIs. Format as a numbered list."* (49 input tokens)
4. **`reason_01` (Simple Reasoning):**  
   *"If a language model processes 20 tokens per second, how many seconds will it take to generate a 120-token response? Show your calculation."* (48 input tokens)

---

## 5. Controlled Execution Protocol

1. **Deterministic Decoding:** Randomness is eliminated by enforcing greedy decoding (`temperature = 0.0`, `do_sample = false`, `top_p = 1.0`, `repetition_penalty = 1.0`).
2. **Output Token Budget:** Standardized to $T_{out} = 64$ tokens per generation.
3. **Warm-Up Phase:** Prior to measured runs, an initial unmeasured request (16 tokens) is executed to prime runtime thread pools, TBB allocators, and OpenCL JIT compilation buffers.
4. **Multi-Run Repetitions:** A minimum of **5 consecutive measured repetitions** ($N=5$) are captured per prompt per configuration.
5. **Statistical Aggregation:** For each metric vector $x_1, \dots, x_N$:
   $$\text{Mean } \mu = \frac{1}{N}\sum_{i=1}^N x_i, \quad \text{Sample StdDev } \sigma = \sqrt{\frac{1}{N-1}\sum_{i=1}^N (x_i - \mu)^2}$$

---

## 6. Raw Data Schema (`benchmarks/results/benchmark_results.csv`)

The automated runner exports raw per-run rows to CSV with the following schema:
```csv
config,device,model_path,prompt_id,category,repetition,input_tokens,output_tokens,ttft_ms,tpot_ms,throughput_tps,total_infer_ms,peak_ram_mb
CPU_INT4,CPU,models/tinyllama-1.1b-chat-int4,fact_01,factual_explanation,1,43,64,149.30,22.00,45.50,1557.00,1223.40
...
```

---

## 7. How to Reproduce

Execute the following commands in PowerShell from the repository root:

```powershell
# Set OpenVINO runtime dynamic library paths
$env:PATH = "C:\openvino_genai\openvino_genai_windows_2026.4.1.0_x86_64\runtime\bin\intel64\Release;C:\openvino_genai\openvino_genai_windows_2026.4.1.0_x86_64\runtime\3rdparty\tbb\bin;$env:PATH"

# Run 5 repetitions across CPU INT4, CPU FP16, and GPU.0 INT4
.\build\Release\benchmark_suite.exe 5
```
Results will print to console and save automatically to `benchmarks/results/benchmark_results.csv`.
