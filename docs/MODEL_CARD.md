# Model Card: TinyLlama 1.1B Chat (OpenVINO IR)

**Assignment ID:** SWFW_5S_13  
**Project:** Local LLM Inference & Optimization Workbench

---

## 1. Model Overview

| Attribute | Specification |
| :--- | :--- |
| **Model ID** | `TinyLlama/TinyLlama-1.1B-Chat-v1.0` |
| **Repository URL** | [https://huggingface.co/TinyLlama/TinyLlama-1.1B-Chat-v1.0](https://huggingface.co/TinyLlama/TinyLlama-1.1B-Chat-v1.0) |
| **Model Architecture** | `LlamaForCausalLM` (Autoregressive Decoder Transformer) |
| **Total Parameter Count** | 1.1 Billion parameters |
| **Hidden Dimension** | 2048 |
| **Intermediate Dimension** | 5632 |
| **Number of Layers** | 22 |
| **Number of Attention Heads** | 32 |
| **Number of Key-Value Heads** | 4 (Grouped-Query Attention - GQA) |
| **Vocabulary Size** | 32,000 |
| **Maximum Context Length** | 2048 tokens |
| **License** | Apache 2.0 (Permissive, commercial and research allowed) |

---

## 2. Tokenization & Chat Template

### Tokenizer
- Type: Byte-Pair Encoding (BPE) tokenizer converted to OpenVINO Tokenizers IR.
- Assets: `openvino_tokenizer.xml`, `openvino_tokenizer.bin`, `openvino_detokenizer.xml`, `openvino_detokenizer.bin`, `tokenizer.json`.

### Chat Template
The model employs a standard prompt structure:
```text
<|system|>
You are a helpful and friendly chatbot.</s>
<|user|>
Hello! Can you help me?</s>
<|assistant|>
Hello! I would be happy to help you.
```

---

## 3. OpenVINO Intermediate Representation (IR) Formats

| Format Variant | Precision | Weight Format | Purpose |
| :--- | :--- | :--- | :--- |
| **Baseline** | FP16 | `--weight-format fp16` | Baseline reference model for latency, throughput, and accuracy measurements |
| **Quantized** | INT4 | `--weight-format int4` | Memory-optimized variant for low RAM footprint and comparison |
| **Quantized** | INT8 | `--weight-format int8` | Balanced compression variant |

---

## 4. Verification & Reproducibility Command

Models are exported via `optimum-cli` / [`scripts/export_model.py`](../scripts/export_model.py):
```bash
python scripts/export_model.py --model-id TinyLlama/TinyLlama-1.1B-Chat-v1.0 --output-dir models/tinyllama-1.1b-chat-fp16 --precision FP16
python scripts/export_model.py --model-id TinyLlama/TinyLlama-1.1B-Chat-v1.0 --output-dir models/tinyllama-1.1b-chat-int4 --precision INT4
```
All binary weights are strictly stored in `models/` and excluded from Git commits via `.gitignore`.

