# Key-Value (KV) Cache: Architecture, Scaling, and OpenVINO GenAI Runtime Behavior

**Project:** Local LLM Inference & Optimization Workbench  
**Model Target:** TinyLlama-1.1B-Chat-v1.0 (FP16 Baseline & INT4 Compressed)  
**OpenVINO GenAI Version:** 2026.4.1.0 C++ API  
**Hardware Profile:** Intel Core i7-12700H (14 cores, 20 threads), 16 GB DDR5 RAM, Windows 11 Home  

---

## 1. Executive Summary & Conceptual Foundations

In transformer-based auto-regressive language generation, tokens are predicted sequentially one token at a time. At generation step $t$, the self-attention mechanism computes attention scores across all previously generated tokens $1 \dots t-1$. 

Without a **Key-Value (KV) cache**, generating token $t$ would require recomputing key and value vectors for all previous tokens from scratch, resulting in quadratic time complexity $\mathcal{O}(T^2)$ with respect to sequence length $T$.

The **KV cache** stores the computed key ($K$) and value ($V$) projection states of all past tokens across every attention layer. This reduces token generation from quadratic complexity $\mathcal{O}(T^2)$ to linear complexity $\mathcal{O}(T)$, enabling fast generation during the decode phase.

```
       Prefill Phase (Prompt Evaluation):
       [Token 1, Token 2, ... Token N] ---> Parallel Forward Pass ---> Store K, V in Cache (High TTFT)
                                                                            |
       Decode Phase (Auto-Regressive Generation):                           |
       Token t-1 + [Cached K, V] -----------> Single Forward Pass -----> Generate Token t (Low TPOT)
                                                      |
                                                      +-------------> Append new K_t, V_t to Cache
```

---

## 2. Core Distinctions

To avoid common industry misconceptions, the following architectural layers are strictly distinguished:

| Architectural Concept | Layer | Description | Lifetime / Storage |
| :--- | :--- | :--- | :--- |
| **Application Chat History** | Application Space (`src/chat_session.cpp`) | The human-readable structured sequence of messages (`system`, `user`, `assistant`). Formatted via the model's Jinja template (`chat_template.jinja`) into prompt strings. | Heap memory (`std::vector<ChatTurn>`, `ov::genai::ChatHistory`). Cleared on `/reset`. |
| **Runtime KV Cache** | OpenVINO Engine Space (`ov::genai::LLMPipeline`) | The internal multidimensional tensor state holding projected key and value activations across all transformer layers. | Managed inside OpenVINO runtime memory pools. Updated during prefill and each decode iteration. |
| **Weight Quantization** | Model Representation (`NNCF INT4`) | Compressing static model weights (e.g. from FP16 16-bit floats to INT4 4-bit integers) using asymmetric block quantization. | Disk & RAM (`openvino_model.bin`). Reduces static model memory from 2.1 GB to 613 MB. |
| **KV Cache Quantization** | Runtime Attention Buffers | Compressing the dynamic key/value attention tensors (e.g., from FP16 to INT8 or FP8). | Dynamic runtime buffer compression where supported by execution device. |

---

## 3. Mathematical KV Cache Sizing & GQA Dynamics

### 3.1 Generalized Sizing Formula

The memory footprint of an uncompressed KV cache in a standard Transformer model is calculated as:

$$\text{KV Cache Bytes} = 2 \times L \times T \times H_{KV} \times D_{head} \times B \times P$$

Where:
- $2$: Multiplier representing both Key ($K$) and Value ($V$) tensors.
- $L$: Number of hidden transformer layers.
- $T$: Total sequence length (input prompt tokens + generated tokens).
- $H_{KV}$: Number of Key-Value attention heads.
- $D_{head}$: Dimension of each attention head ($D_{head} = \frac{\text{hidden\_size}}{H_Q}$).
- $B$: Batch size (for single-user local inference, $B = 1$).
- $P$: Precision in bytes per element ($P = 2$ for FP16).

---

### 3.2 TinyLlama-1.1B Architectural Specification

From `config.json` of `TinyLlama/TinyLlama-1.1B-Chat-v1.0`:
- **Hidden Size:** $2048$
- **Number of Layers ($L$):** $22$
- **Query Attention Heads ($H_Q$):** $32$
- **KV Attention Heads ($H_{KV}$):** $4$ (**Grouped-Query Attention**, GQA)
- **Head Dimension ($D_{head}$):** $\frac{2048}{32} = 64$
- **Element Precision ($P$):** $2\text{ bytes}$ (FP16)

#### Memory per Token Calculation:
$$\text{Bytes per Token} = 2 \times 22 \times 4 \times 64 \times 1 \times 2 = 22,528\text{ bytes} \approx 22.00\text{ KB/token}$$

#### Comparison: Grouped-Query Attention (GQA) vs Multi-Head Attention (MHA)
If TinyLlama used standard Multi-Head Attention ($H_{KV} = H_Q = 32$):
$$\text{MHA Bytes per Token} = 2 \times 22 \times 32 \times 64 \times 1 \times 2 = 180,224\text{ bytes} \approx 176.00\text{ KB/token}$$

> **Architectural Insight:** By utilizing Grouped-Query Attention with 4 KV heads instead of 32, TinyLlama achieves an **87.5% reduction in theoretical KV cache footprint**, drastically lowering memory bandwidth demands during autoregressive generation on CPU.

---

### 3.3 Theoretical Sizing Across Context Tiers

| Context Length ($T$) | Theoretical KV Cache (GQA) | Theoretical KV Cache (MHA) | GQA Savings |
| :--- | :--- | :--- | :--- |
| **128 tokens** | $2.75\text{ MB}$ | $22.00\text{ MB}$ | 87.5% |
| **512 tokens** | $11.00\text{ MB}$ | $88.00\text{ MB}$ | 87.5% |
| **1,024 tokens** | $22.00\text{ MB}$ | $176.00\text{ MB}$ | 87.5% |
| **2,048 tokens** (Max Context) | $44.00\text{ MB}$ | $352.00\text{ MB}$ | 87.5% |

---

## 4. OpenVINO GenAI Runtime Cache Architecture

Inspection of OpenVINO GenAI 2026.4.1 C++ headers (`llm_pipeline.hpp`, `cache_eviction.hpp`, `scheduler_config.hpp`) reveals how the runtime manages attention caches:

1. **Automatic Internal KV Allocation:** In `ov::genai::LLMPipeline`, KV cache management is fully automated. When `LLMPipeline::generate()` is invoked, the pipeline allocates stateful tensors within the underlying OpenVINO compiled model (`ov::CompiledModel`) or continuous batching scheduler.
2. **Prefill vs. Decode Execution:**
   - **Prefill:** Computes attention across all $N$ prompt tokens in parallel forward passes. Populates initial KV buffers. Reflected in **TTFT (Time to First Token)**.
   - **Decode:** Evaluates one token at a time, performing matrix multiplications against the accumulated KV cache. Reflected in **TPOT (Time per Output Token)**.
3. **Advanced Cache Mechanisms in OpenVINO GenAI 2026.4:**
   - `ov::genai::CacheEvictionConfig`: Supports token pruning/eviction algorithms (`AggregationMode::SUM`, `AggregationMode::NORM_SUM`, `AggregationMode::ADAPTIVE_RKV`).
   - `ov::genai::KVCrushConfig`: Anchor-point based KV cache compression (`KVCrushAnchorPointMode::RANDOM`, `ZEROS`, `ONES`, `MEAN`, `ALTERNATING`).
   - `ov::genai::SchedulerConfig`: Blocked paged attention configuration (`num_kv_blocks`, `block_size`, `cache_size`) for continuous batching pipelines.
4. **API Evolution Note:** The legacy methods `LLMPipeline::start_chat()` and `LLMPipeline::finish_chat()` are officially marked `OPENVINO_DEPRECATED`. Stateful multi-turn conversation is now driven by passing `ov::genai::ChatHistory` directly to `generate()`.

---

## 5. Controlled Empirical Scaling Experiment

A dedicated C++ benchmark harness (`src/benchmark_kv.cpp`) was compiled in Release mode and executed on Intel Core i7-12700H. The benchmark programmatically synthesizes prompts of exactly 128, 512, 1024, and 2048 tokens using the model tokenizer, executes greedy deterministic generation ($T=32$ new tokens), and records microsecond-level hardware metrics via `ov::genai::PerfMetrics` and Windows OS working set RAM via `GetProcessMemoryInfo`.

### 5.1 INT4 Quantized Model Results (`models/tinyllama-1.1b-chat-int4`)

- **Model Static Footprint on Disk:** 613.34 MB
- **Process Baseline RAM Before Inference:** 154.75 MB

| Target Context | Actual Input Tokens | New Tokens | TTFT (ms) | TPOT (ms/token) | Throughput (tokens/s) | Total Time (ms) | Peak Process RAM (MB) |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **128** | 128 | 32 | 529.59 | 25.64 | 39.00 | 1,325.20 | 1,238.87 |
| **512** | 512 | 32 | 1,825.35 | 33.78 | 29.60 | 2,873.31 | 1,284.43 |
| **1024** | 1024 | 32 | 3,411.34 | 35.95 | 27.82 | 4,094.93 | 1,313.23 |
| **2048** | 2048 | 32 | 7,086.28 | 46.09 | 21.70 | 7,962.86 | 1,403.81 |

---

### 5.2 FP16 Baseline Model Results (`models/tinyllama-1.1b-chat-fp16`)

- **Model Static Footprint on Disk:** 2,105.47 MB
- **Process Baseline RAM Before Inference:** 74.82 MB

| Target Context | Actual Input Tokens | New Tokens | TTFT (ms) | TPOT (ms/token) | Throughput (tokens/s) | Total Time (ms) | Peak Process RAM (MB) |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **128** | 128 | 32 | 2,911.92 | 63.27 | 15.81 | 4,873.91 | 4,117.89 |
| **512** | 512 | 32 | 2,757.48 | 59.27 | 16.87 | 4,595.23 | 4,163.77 |
| **1024** | 1024 | 32 | 3,396.00 | 55.89 | 17.89 | 5,129.26 | 4,195.68 |
| **2048** | 2048 | 32 | 6,772.34 | 60.46 | 16.54 | 8,647.30 | 4,293.98 |

---

## 6. Analytical Findings & Scaling Discussion

```
              TTFT Scaling (ms) vs Input Tokens
    8000 +                                                * (2048 tok: ~7.0s)
    7000 |
    6000 |
    5000 |
    4000 |                                * (1024 tok: ~3.4s)
    3000 |
    2000 |                * (512 tok: ~1.8s)
    1000 |  * (128 tok: ~0.5s)
       0 +----+-----------+---------------+---------------+
         0   128         512             1024            2048
```

1. **TTFT Prefill Scaling:**
   - TTFT displays near-perfect linear scaling with prompt length on INT4: $529\text{ ms}$ (128 tok) $\rightarrow$ $1,825\text{ ms}$ (512 tok) $\rightarrow$ $3,411\text{ ms}$ (1024 tok) $\rightarrow$ $7,086\text{ ms}$ (2048 tok).
   - This directly reflects the computational cost of evaluating the initial sequence to build the initial KV states before generation can begin.
2. **TPOT Autoregressive Scaling:**
   - In INT4, TPOT grows from $25.64\text{ ms/token}$ at 128 tokens up to $46.09\text{ ms/token}$ at 2048 tokens (+79.7% latency increase per generated token).
   - As context expands, the attention dot product must scan over progressively larger KV tensors, increasing memory bandwidth overhead per token step.
3. **RAM Footprint Dynamics:**
   - INT4 peak RAM scales from $1,238.87\text{ MB}$ to $1,403.81\text{ MB}$ (+164.94 MB observed delta across 2048 tokens).
   - FP16 peak RAM scales from $4,117.89\text{ MB}$ to $4,293.98\text{ MB}$ (+176.09 MB observed delta across 2048 tokens).
   - The observed physical RAM growth exceeds theoretical tensor sizes ($44\text{ MB}$) due to OpenVINO internal pre-allocated workspace buffers, TBB thread scratchpads, and execution graph memory alignment.

---

## 7. Unsupported Features & Hardware Boundaries

- **NPU Device Target:** The tested 12th Gen Intel Core i7-12700H does not feature an integrated Intel NPU (NPU is introduced in Intel Core Ultra / Meteor Lake). Attempting to target `NPU` fails validation.
- **KV Cache Quantization on CPU:** While weight quantization (INT4) is fully accelerated by the CPU plugin via AVX2 / AVX-512 VNNI, dynamic KV cache quantization (e.g. 4-bit KV cache) is not enabled by default for high-level CPU inference without explicit continuous batching scheduler properties.
