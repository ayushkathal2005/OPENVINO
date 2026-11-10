# Phase 03 Report: First Real C++ Local Inference

```text
Phase: 03 --- Local inference
Status: READY_FOR_REVIEW
Start date: 2026-11-11
Completion date: 2026-11-11
Environment: Windows 11 Home (Build 10.0.26300), Intel Core i7-12700H, MSVC v143 (19.44.35222 x64), CMake 4.4.4, OpenVINO GenAI 2026.4.1.0 C++ API
Files changed:
  - CMakeLists.txt
  - src/llm_engine.hpp
  - src/llm_engine.cpp
  - src/main.cpp
  - tests/test_smoke_inference.cpp
  - docs/PHASE_STATUS.md
  - docs/phase-reports/PHASE_03_REPORT.md
```

---

## Implementation Summary

1. **C++ LLMEngine Architecture (`src/llm_engine.hpp` / `src/llm_engine.cpp`):**
   - Designed and implemented `LLMEngine` wrapping OpenVINO GenAI's native `ov::genai::LLMPipeline`.
   - Comprehensive input validation: verifies directory existence, presence of mandatory OpenVINO IR files (`openvino_model.xml`, `openvino_model.bin`), and device validity against detected available devices (`ov::Core::get_available_devices()`).
   - Generation configuration encapsulation: handles max new tokens, temperature, and top-p sampling safely using `ov::genai::GenerationConfig`.
   - Exception isolation: wraps pipeline calls in standard C++ `try-catch` blocks to convert runtime errors into clean diagnostic error messages without crashing.
   - Non-empty output validation and empty prompt protection.

2. **Command-Line Interface & Interactive Loop (`src/main.cpp`):**
   - Implemented standard argument parsing supporting:
     - `--model-path <dir>` (defaults to `models/tinyllama-1.1b-chat-int4`, supports FP16/INT4 or custom path)
     - `--device <dev>` (defaults to `CPU`, supports `GPU.0`, `GPU.1`)
     - `--prompt <text>` (optional direct single prompt mode)
     - `--max-tokens <int>` (defaults to 128)
     - `--help` (usage guide)
   - Interactive prompt session loop: allows continuous multi-turn prompting from standard input.
   - Clear interactive termination commands: recognizes `exit` and `quit` keywords and handles EOF (`std::cin.eof()`) cleanly.

3. **Automated Smoke Test Suite (`tests/test_smoke_inference.cpp`):**
   - Implemented an automated test harness validating 9 critical paths:
     1. Empty model path rejected.
     2. Non-existent model path rejected.
     3. Unsupported device rejected.
     4. Supported CPU device accepted.
     5. Real local model directory verified.
     6. Pipeline initialization on CPU.
     7. Safe empty prompt handling.
     8. Non-empty coherent generation from valid prompt.
     9. Sequential generation stability.

4. **CMake Build Integration (`CMakeLists.txt`):**
   - Modularized build with `ov_llm_core` static library linking against `openvino::runtime` and `openvino::genai`.
   - Built standalone executables `llm_inference` and `test_smoke_inference`.
   - Integrated CTest runner for automated test verification.

---

## Commands Executed & Real Outputs

### 1. Build and Test Suite Execution

```powershell
$env:PATH="C:\openvino_genai\openvino_genai_windows_2026.4.1.0_x86_64\runtime\bin\intel64\Release;C:\openvino_genai\openvino_genai_windows_2026.4.1.0_x86_64\runtime\3rdparty\tbb\bin;$env:PATH"
.\build\Release\test_smoke_inference.exe
```

**Real Output:**
```text
==================================================
  Running Phase 03 C++ Local Inference Smoke Tests
==================================================

[TEST 1] Validation: Empty model path rejected...
  -> PASS

[TEST 2] Validation: Non-existent model path rejected...
  -> PASS

[TEST 3] Validation: Unsupported device rejected...
  -> PASS

[TEST 4] Validation: CPU device accepted...
  -> PASS

[TEST 5] Validation: Real FP16 model directory accepted...
  -> PASS

[TEST 6] Execution: Model load on CPU...
[LLMEngine] Initializing ov::genai::LLMPipeline on device: CPU...
[LLMEngine] Model successfully loaded into memory.
  -> PASS

[TEST 7] Execution: Empty prompt returns safely without crash...
[LLMEngine Warning] Prompt is empty.
  -> PASS

[TEST 8] Execution: Valid prompt produces non-empty output...
  Prompt: What is the capital of France? Answer:
  Output: The capital of France is Paris.
  -> PASS

[TEST 9] Execution: Second sequential prompt produces non-empty output...
  Prompt: 2 + 2 = 
  Output: The answer to the question "2 + 2 =?" is "Yes, 2 + 2 = 4
  -> PASS

==================================================
  Test Summary: 9/9 Passed
==================================================
```

---

### 2. INT4 Model Inference CLI Test

```powershell
.\build\Release\llm_inference.exe --model-path models/tinyllama-1.1b-chat-int4 --device CPU --prompt "Explain what an operating system is in two sentences." --max-tokens 40
```

**Real Output:**
```text
==================================================
  Local LLM Inference Workbench (OpenVINO GenAI)
==================================================
Model Path: models/tinyllama-1.1b-chat-int4
Device:     CPU
Max Tokens: 40
--------------------------------------------------
[LLMEngine] Initializing ov::genai::LLMPipeline on device: CPU...
[LLMEngine] Model successfully loaded into memory.

[User]: Explain what an operating system is in two sentences.
[Assistant]: An operating system is a software that controls and manages the hardware and applications on a computer system. It is responsible for providing basic functionality, such as file management, user interface, and system resources allocation
```

---

### 3. Interactive Mode & Exit Command Test

```powershell
echo "exit" | .\build\Release\llm_inference.exe --model-path models/tinyllama-1.1b-chat-int4 --device CPU
```

**Real Output:**
```text
==================================================
  Local LLM Inference Workbench (OpenVINO GenAI)
==================================================
Model Path: models/tinyllama-1.1b-chat-int4
Device:     CPU
Max Tokens: 128
--------------------------------------------------
[LLMEngine] Initializing ov::genai::LLMPipeline on device: CPU...
[LLMEngine] Model successfully loaded into memory.

Entering interactive prompt loop. Type 'exit' or 'quit' to end.

[User]: Exiting application. Goodbye.
```

---

### 4. Error Handling: Invalid Model Path

```powershell
.\build\Release\llm_inference.exe --model-path non_existent_dir --device CPU
```

**Real Output:**
```text
[LLMEngine Error] Directory does not exist: non_existent_dir
[FATAL] Failed to initialize model. Exiting.
```

---

### 5. Error Handling: Unsupported Device

```powershell
.\build\Release\llm_inference.exe --model-path models/tinyllama-1.1b-chat-int4 --device TPU
```

**Real Output:**
```text
[LLMEngine Error] Requested device 'TPU' is not available. Available devices: [CPU, GPU.0, GPU.1]
[FATAL] Failed to initialize model. Exiting.
```

---

## Test results:

| Test / Check | Command / Operation | Result | Details |
| :--- | :--- | :--- | :--- |
| **C++ Release Build** | `cmake --build build --config Release` | **PASS** | Clean build of `ov_llm_core`, `llm_inference`, `test_smoke_inference` |
| **Validation: Empty Path** | `test_smoke_inference` Test 1 | **PASS** | Returns `false` with diagnostic message |
| **Validation: Missing Dir** | `test_smoke_inference` Test 2 | **PASS** | Returns `false` with diagnostic message |
| **Validation: Bad Device** | `test_smoke_inference` Test 3 | **PASS** | Returns `false` listing available devices |
| **Model Load (CPU)** | `test_smoke_inference` Test 6 | **PASS** | `ov::genai::LLMPipeline` loaded into host RAM |
| **Empty Prompt Guard** | `test_smoke_inference` Test 7 | **PASS** | Handled without crashing; returns warning |
| **Valid Inference** | `test_smoke_inference` Test 8 | **PASS** | Generates coherent response: "The capital of France is Paris." |
| **Sequential Inference** | `test_smoke_inference` Test 9 | **PASS** | Generates coherent response: "The answer ... 2 + 2 = 4" |
| **INT4 CLI Inference** | `llm_inference.exe --prompt "..."` | **PASS** | Fully functional local INT4 generation |
| **Interactive Exit** | `llm_inference.exe` (stdin `exit`) | **PASS** | Clean termination with status code 0 |
| **Local-Only Inference** | Verification & Code Inspection | **PASS** | Zero cloud/network API calls; pure C++ OpenVINO GenAI execution |

---

## Acceptance criteria checklist:

- [x] Application compiles in Release mode.
- [x] Application loads the model from a local directory (`models/tinyllama-1.1b-chat-int4` and `models/tinyllama-1.1b-chat-fp16`).
- [x] A user prompt produces non-empty generated output.
- [x] No hosted LLM API is called (pure C++ `ov::genai::LLMPipeline`).
- [x] Invalid inputs produce understandable errors rather than crashes.
- [x] Model path and device are configurable via CLI arguments and not hardcoded.
- [x] The application exits cleanly.
- [x] A clean build and smoke inference both pass (9/9 automated tests pass).
