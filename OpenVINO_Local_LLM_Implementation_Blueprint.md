# Local LLM on OpenVINO --- Implementation Blueprint

## Phase-gated engineering plan and execution contract for Antigravity

**Assignment ID:** SWFW_5S_13\
**Project name:** Local LLM Inference & Optimization Workbench\
**Primary requirement:** Run a 1B--3B parameter language model entirely
locally using the OpenVINO GenAI C++ API.\
**Implementation principle:** One phase at a time. A phase is not
complete until its acceptance criteria, tests, evidence, Git commit,
GitHub push, and remote verification have all succeeded.

------------------------------------------------------------------------

# 0. Instructions to the AI coding agent

You are implementing this project inside the user's existing workspace.
This document is the controlling specification. Follow it literally.

## 0.1 Non-negotiable phase gate

**You MUST NOT begin Phase N+1 until Phase N is formally complete.**

A phase is formally complete only when every condition below is true:

1.  All tasks listed for the phase are implemented.
2.  Every mandatory acceptance criterion passes.
3.  All phase-specific tests have been run successfully.
4.  The test commands and actual results are recorded in that phase's
    report.
5.  No known blocker or unresolved critical defect remains in the phase.
6.  The phase changes have been committed to Git.
7.  The commit has been pushed to the configured GitHub remote.
8.  The pushed commit has been verified against the remote branch.
9.  The phase status file has been updated with the commit hash, push
    verification, test evidence, and completion date.
10. The user has reviewed the phase report and explicitly approved
    moving to the next phase.

If any condition is false, mark the phase `BLOCKED` or
`READY_FOR_REVIEW`, explain exactly what is missing, and STOP. Do not
start the next phase. Do not silently skip a failing test. Do not mark a
phase complete based on code inspection alone.

## 0.2 Work protocol

At the beginning of every session:

1.  Read this blueprint completely.
2.  Read `docs/PHASE_STATUS.md`, if it exists.
3.  Read the current phase report, if it exists.
4.  Run `git status`, inspect the current branch, latest commit, and
    configured remotes.
5.  Identify the first phase that is not formally complete.
6.  Work only on that phase.
7.  Before changing files, state the phase, its objective, and the
    acceptance criteria being targeted.
8.  Do not implement future-phase features early, even if they seem
    convenient. You may add only the minimum prerequisite needed for the
    current phase.
9.  At the end, run the phase gate and prepare the report.
10. Commit and push only after all mandatory tests pass.
11. Stop and request user review before starting the next phase.

If the repository already contains work, inspect and preserve it. Do not
overwrite, delete, reset, or reformat unrelated user work.

## 0.3 Truthfulness and engineering integrity

-   Never fabricate test results, hardware details, benchmark values,
    screenshots, logs, or GitHub push success.
-   Never write `PASS` unless the command was actually executed and
    passed.
-   Never write `PUSHED` unless the remote contains the exact local
    commit.
-   Never claim that a model ran offline unless an offline test was
    actually performed.
-   If a test cannot be run because hardware, dependencies, credentials,
    or network access are unavailable, record it as `NOT RUN` with the
    reason. It is not a pass.
-   Do not use a cloud LLM API as the inference backend. The model
    inference path must remain local.
-   Network access is allowed for initial dependency/model downloads and
    Git operations. Runtime inference must not depend on a remote model
    API.
-   Do not commit API keys, access tokens, passwords, private keys,
    `.env` files, downloaded model weights, model caches, build outputs,
    or personal machine-specific paths.
-   Never run `git push --force`, `git reset --hard`, or destructive
    cleanup commands without explicit user approval.
-   Do not change the required C++ inference backend to Python. Python
    may be used for model export, setup utilities, and offline analysis
    only.
-   Prefer official OpenVINO GenAI APIs and examples. Do not invent API
    names or copy obsolete API examples without checking the installed
    version's documentation and headers.

## 0.4 Stop-and-ask conditions

Stop and ask the user instead of guessing if:

-   There is no GitHub remote and repository creation/remote URL is
    required.
-   Git authentication fails.
-   The current branch contains uncommitted user work that could be
    affected.
-   The available hardware/OS is not known and affects a technical
    decision.
-   The selected model is not supported by the current OpenVINO GenAI
    release or fails export.
-   The installed OpenVINO and OpenVINO GenAI versions are incompatible.
-   A dependency requires administrator access or a system-wide change.
-   A destructive action, license decision, public repository setting,
    or release/tag operation is required but not specified.

------------------------------------------------------------------------

# 1. Project outcome

Build a reliable, documented, locally running LLM application that uses
the **OpenVINO GenAI C++ API** for inference.

The project should demonstrate:

-   Local deployment of one model in the 1B--3B parameter range.
-   C++ inference through `ov::genai::LLMPipeline` or the current
    supported equivalent.
-   Local prompt processing and text generation.
-   Multi-turn chat if supported by the selected model/API version.
-   Model precision/weight quantization experiments.
-   Understanding and measured investigation of KV cache behavior.
-   Performance measurements: model load time, TTFT, TPOT, throughput,
    prompt/output token counts, and resource usage where measurable.
-   Reproducible tests and benchmark methodology.
-   A GitHub repository with phase-wise commits and evidence.

## 1.1 Definition of success

The minimum successful submission is not merely a program that compiles.
It must:

1.  Load a locally stored 1B--3B model using OpenVINO GenAI C++.
2.  Generate a response without calling a hosted LLM API.
3.  support repeatable prompt execution.
4.  Report real inference metrics.
5.  Include at least one quantization comparison, subject to model and
    hardware support.
6.  Explain how the KV cache works and demonstrate its memory/latency
    implications through a controlled experiment.
7.  Provide build/run instructions that another developer can follow.
8.  Have a clean Git history showing one verified commit per completed
    phase.

A graphical interface is optional. A correct C++ inference engine and
defensible measurements take priority over UI polish.

------------------------------------------------------------------------

# 2. Proposed architecture

``` text
                         USER
                           |
                           v
                CLI / Optional Local UI
                           |
                           v
                  C++ Application Layer
                  - Input validation
                  - Session handling
                  - Generation settings
                  - Output formatting
                           |
                           v
                  OpenVINO GenAI C++ API
                    ov::genai::LLMPipeline
                           |
              +------------+------------+
              |                         |
              v                         v
          Tokenizer               Generation config
              |                         |
              +------------+------------+
                           |
                           v
                   OpenVINO Runtime
                           |
                +----------+----------+
                |          |          |
                v          v          v
               CPU        GPU        NPU
                \          |          /
                 \         |         /
                  +--------+--------+
                           |
                           v
                Local OpenVINO IR Model
                 - model weights
                 - tokenizer assets
                 - generation config
                 - chat template
```

The actual device depends on the machine, installed drivers, OpenVINO
device availability, and model/operator support. Do not promise GPU or
NPU support until the target device has been detected and inference has
been verified on it.

## 2.1 Technology choices

  -----------------------------------------------------------------------
  Layer                               Required/preferred technology
  ----------------------------------- -----------------------------------
  Inference application               C++

  Language standard                   Use the standard required by the
                                      installed OpenVINO GenAI package
                                      and official sample; prefer C++17
                                      if compatible

  LLM pipeline                        OpenVINO GenAI C++ API

  Runtime                             OpenVINO Runtime

  Build system                        CMake

  Compiler                            MSVC on Windows; GCC/Clang on Linux

  Model source                        Hugging Face or official
                                      pre-converted OpenVINO model (recommended)

  Model conversion                    Optimum Intel / `optimum-cli` where
                                      applicable

  Optional setup/evaluation scripts   Python

  Benchmark output                    CSV plus Markdown report

  Optional visualization              Lightweight local dashboard, only
                                      after core inference is complete
  -----------------------------------------------------------------------

**Important:** Installing a Python wheel does not automatically prove
that the C++ headers, libraries, and CMake package configuration are
installed. Use the official C++-capable OpenVINO/OpenVINO GenAI
distribution or documented build/install process. Verify this in Phase
1.

## 2.2 Model-selection policy

Start with one model only. A practical initial candidate is:

-   `TinyLlama/TinyLlama-1.1B-Chat-v1.0`

It appears in the OpenVINO GenAI project's own quick-start material.
However, the agent must verify compatibility against the current
official supported-model list and the installed release before using it.

If it is incompatible, choose another currently supported 1B--3B
text-generation model. Record:

-   Hugging Face model ID.
-   Parameter count.
-   Model license.
-   Model architecture.
-   Tokenizer and chat-template availability.
-   OpenVINO GenAI version tested.
-   Conversion command and precision.
-   Model source URL and model revision/commit where practical.

Do not switch models repeatedly. A model change requires recording the
reason and re-running the affected tests.

------------------------------------------------------------------------

# 3. Repository structure

Create or adapt the following structure. Keep the project simple; do not
add frameworks without a clear need.

``` text
local-llm-openvino/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── .gitignore
├── .editorconfig
│
├── config/
│   └── app_config.example.json
│
├── models/
│   └── README.md
│
├── scripts/
│   ├── export_model.py
│   ├── collect_environment.py
│   └── verify_phase.py
│
├── src/
│   ├── main.cpp
│   ├── llm_engine.hpp
│   ├── llm_engine.cpp
│   ├── chat_session.hpp
│   ├── chat_session.cpp
│   ├── benchmark.hpp
│   └── benchmark.cpp
│
├── tests/
│   ├── CMakeLists.txt
│   ├── test_config.cpp
│   ├── test_input_validation.cpp
│   └── test_smoke_inference.cpp
│
├── benchmarks/
│   ├── README.md
│   ├── prompts.json
│   ├── results/
│   │   └── .gitkeep
│   └── plots/
│       └── .gitkeep
│
├── docs/
│   ├── ARCHITECTURE.md
│   ├── ENVIRONMENT.md
│   ├── MODEL_CARD.md
│   ├── QUANTIZATION.md
│   ├── KV_CACHE.md
│   ├── BENCHMARK_METHODOLOGY.md
│   ├── PHASE_STATUS.md
│   └── phase-reports/
│       └── .gitkeep
│
└── build/                  # generated; never commit
```

Adapt this structure only when the actual implementation justifies it.
Do not create empty abstractions merely to match the tree.

## 3.1 Git ignore requirements

At minimum, ignore:

``` gitignore
# Build output
build/
cmake-build-*/
out/
Debug/
Release/

# Downloaded models and model caches
models/*
!models/README.md
*.bin
*.safetensors
*.onnx
*.gguf
*.blob

# Python
.venv/
venv/
__pycache__/
*.py[cod]

# IDE / OS
.vscode/
.idea/
.DS_Store
Thumbs.db

# Secrets and local settings
.env
.env.*
!.env.example
*.pem
*.key

# Logs and temporary output
*.log
*.tmp
```

Review the ignore rules before the first commit. Do not accidentally
ignore source files or documentation.

------------------------------------------------------------------------

# 4. Strict phase plan

There are nine mandatory phases. Each phase has a hard gate. The next
phase is locked until the current phase is formally complete and
user-approved.

## Phase 0 --- Repository, machine audit, and project contract

**Objective:** Establish a safe, reproducible workspace and identify the
machine's real capabilities before selecting a model or promising
acceleration.

### Tasks

1.  Inspect the existing workspace and Git status.
2.  Confirm whether the project directory is already a Git repository.
3.  Record OS, OS version, CPU model, physical/logical cores, RAM, free
    disk space, and available GPU/NPU devices.
4.  Detect installed compiler, CMake, Git, Python, OpenVINO, and
    OpenVINO GenAI versions if present.
5.  Check whether the machine has an Intel CPU/GPU/NPU; do not assume it
    does.
6.  Inspect available OpenVINO devices using the official
    runtime/device-query mechanism.
7.  Confirm the GitHub remote and current branch.
8.  Create the repository structure and `.gitignore`.
9.  Create `docs/PHASE_STATUS.md` and the Phase 0 report.
10. Create a short project scope and out-of-scope list.

### Required files

-   `README.md` --- initial project overview and current status.
-   `.gitignore`
-   `docs/ENVIRONMENT.md`
-   `docs/PHASE_STATUS.md`
-   `docs/phase-reports/PHASE_00_REPORT.md`

### Acceptance criteria

-   [ ] Hardware and OS information is recorded from actual inspection.
-   [ ] OpenVINO device availability is recorded; unsupported devices
    are clearly marked unavailable.
-   [ ] Git repository and target branch are known.
-   [ ] GitHub remote is verified or the project is explicitly blocked
    pending its setup.
-   [ ] Repository structure exists.
-   [ ] Model weights, build output, secrets, and local caches are
    excluded from Git.
-   [ ] No unrelated user files were modified or removed.

### Tests / verification

-   `git status`
-   `git remote -v`
-   `git branch --show-current`
-   Tool/version checks for compiler, CMake, Python, and Git.
-   OpenVINO device detection, if OpenVINO is installed.

### Git checkpoint

Commit message:

``` text
chore(phase-00): establish project workspace and hardware audit
```

Push to the configured GitHub branch and verify the remote commit.

### Gate

**Do not start Phase 1 until Phase 0 is committed, pushed, remotely
verified, documented, and approved by the user.**

------------------------------------------------------------------------

## Phase 1 --- C++ toolchain and OpenVINO GenAI environment

**Objective:** Prove that the machine can compile and run a minimal C++
program linked to OpenVINO GenAI.

### Tasks

1.  Select the official installation route for the detected OS.
2.  Install a compatible OpenVINO Runtime and OpenVINO GenAI C++
    distribution.
3.  Install or verify CMake and a supported C++ compiler.
4.  Record exact versions and installation paths.
5.  Inspect the installed headers and CMake package files.
6.  Study the official OpenVINO GenAI C++ sample and its CMake
    configuration.
7.  Create a minimal CMake project.
8.  Compile a small program that initializes the OpenVINO runtime and
    reports available devices.
9.  Compile a minimal GenAI-linked sample or official sample without
    model inference.
10. Document the exact setup and build commands.

### Required implementation

-   Root `CMakeLists.txt`.
-   `src/main.cpp` or a dedicated environment-check source file.
-   `docs/ENVIRONMENT.md` updated with tested versions and commands.

Use the package names, imported targets, and link configuration from the
installed release's official C++ sample. Do not guess CMake target names
from memory.

### Acceptance criteria

-   [ ] CMake configures successfully.
-   [ ] C++ source compiles successfully.
-   [ ] Executable links to OpenVINO libraries.
-   [ ] Runtime initializes without missing-DLL/shared-library errors.
-   [ ] Available devices are printed or otherwise recorded.
-   [ ] A clean rebuild succeeds.
-   [ ] Setup steps are documented and reproducible.

### Tests / verification

-   Configure from a clean build directory.
-   Build in the intended configuration, preferably Release.
-   Run the environment-check executable.
-   Run the official or equivalent minimal GenAI C++ sample.
-   Record command output and versions in `PHASE_01_REPORT.md`.

### Git checkpoint

``` text
build(phase-01): configure and verify OpenVINO GenAI C++ toolchain
```

Push and verify the remote commit.

### Gate

If the C++ GenAI sample does not compile and run, stop. Do not
download/convert the main model or begin Phase 2.

------------------------------------------------------------------------

## Phase 2 --- Model selection, download, and OpenVINO conversion

**Objective:** Produce a valid local OpenVINO IR model and verify its
assets before implementing the application.

### Tasks

1.  Re-check the official OpenVINO GenAI supported-model list.
2.  Select one supported 1B--3B text-generation model.
3.  Verify the model license and access requirements.
4.  Record the exact model ID and revision.
5.  Create an isolated Python environment for export tooling.
6.  Install the documented compatible `optimum-intel` and export
    dependencies.
7.  Export the model to OpenVINO IR.
8.  Start with one precision that is practical for the machine; preserve
    a baseline suitable for later comparison.
9.  If supported and storage permits, export a second quantized variant
    for later experiments.
10. Verify that the output directory contains the expected model,
    tokenizer, generation, and configuration assets.
11. Record export duration, model directory size, and any
    warnings/errors.
12. Add model download/export instructions to `models/README.md`.

### Conversion policy

Use the official documented conversion syntax for the selected model and
installed `optimum-intel` version. A typical documented pattern is:

``` bash
optimum-cli export openvino --model <MODEL_ID> <OUTPUT_DIRECTORY>
```

Weight-format options such as `fp16`, `int8`, and `int4` may be
available, but verify the exact options and model support before using
them. Do not assume every architecture supports every precision path.

### Acceptance criteria

-   [ ] Model is within the 1B--3B assignment range.
-   [ ] Model is listed as supported or its compatibility is explicitly
    validated.
-   [ ] License and model source are documented.
-   [ ] Export command completes successfully.
-   [ ] OpenVINO IR files and tokenizer assets exist locally.
-   [ ] Model path is configurable and not hardcoded to one developer's
    machine.
-   [ ] Model weights are not committed to Git.
-   [ ] Model directory size and export details are recorded.

### Tests / verification

-   Check required files and non-zero file sizes.
-   Verify tokenizer/configuration files.
-   Run the official documented model validation or a minimal load test
    if available.
-   Confirm that the model directory is ignored by Git.
-   Confirm that a clean checkout can reproduce model preparation using
    documented instructions.

### Git checkpoint

``` text
feat(phase-02): add reproducible local model preparation workflow
```

Commit scripts and documentation, not model weights. Push and verify.

### Gate

Do not implement the full inference engine until the model export and
asset validation pass.

------------------------------------------------------------------------

## Phase 3 --- First real C++ local inference

**Objective:** Generate a response from the local model using the
OpenVINO GenAI C++ API.

### Tasks

1.  Implement a small `LLMEngine` wrapper around the current supported
    C++ GenAI pipeline.
2.  Load the model from a command-line or configuration-supplied local
    path.
3.  Select a device through configuration/CLI, defaulting to CPU as the
    baseline.
4.  Validate the model path and device before loading.
5.  Accept a prompt from standard input or command-line arguments.
6.  Configure generation parameters such as maximum new tokens.
7.  Generate and print the response.
8.  Handle empty prompts, invalid model paths, unsupported devices, and
    runtime exceptions.
9.  Keep inference local; no hosted LLM endpoint or API key may be used.
10. Add a clear exit command for the interactive loop.

### API policy

Use the C++ API exposed by the installed OpenVINO GenAI release.
`ov::genai::LLMPipeline` is the intended high-level pipeline where
supported. Check official headers and samples for exact signatures. Do
not copy stale API examples without compiling them.

### Acceptance criteria

-   [ ] Application compiles in Release mode.
-   [ ] Application loads the model from a local directory.
-   [ ] A user prompt produces non-empty generated output.
-   [ ] No hosted LLM API is called.
-   [ ] Invalid inputs produce understandable errors rather than
    crashes.
-   [ ] Model path and device are not hardcoded to one machine.
-   [ ] The application exits cleanly.
-   [ ] A clean build and smoke inference both pass.

### Tests / verification

-   Valid prompt.
-   Empty prompt.
-   Invalid model path.
-   Unsupported device.
-   Multiple sequential prompts.
-   Exit command.
-   Run at least one inference with network disconnected after all
    required assets have been downloaded, where practical, to
    substantiate local-only behavior.

### Git checkpoint

``` text
feat(phase-03): implement local C++ LLM inference with OpenVINO GenAI
```

Push and verify.

### Gate

Do not begin conversation-state or KV-cache experiments until a
repeatable single-prompt C++ inference succeeds.

------------------------------------------------------------------------

## Phase 4 --- Chat sessions and generation controls

**Objective:** Turn the single-prompt demo into a usable local chat
application without losing control of the inference path.

### Tasks

1.  Implement a chat/session abstraction.
2.  Use the current documented OpenVINO GenAI chat-history mechanism
    where supported.
3.  Verify the model's chat template and tokenizer configuration.
4.  Preserve conversation turns correctly.
5.  Add a clear/reset conversation command.
6.  Add generation configuration for maximum new tokens and supported
    sampling parameters.
7.  Use deterministic settings for benchmark tests where possible.
8.  Keep interactive settings separate from benchmark settings.
9.  Ensure user-visible errors are handled gracefully.
10. Document any model/API limitations around chat history.

### Acceptance criteria

-   [ ] At least two consecutive turns work as a conversation.
-   [ ] Conversation reset starts a clean session.
-   [ ] Chat template is applied correctly.
-   [ ] Generation parameters are validated.
-   [ ] Session state does not leak into a new session.
-   [ ] The program remains local-only.
-   [ ] Tests cover session creation, continuation, reset, and invalid
    configuration.

### Tests / verification

Use a fixed conversation with at least three turns. Verify that the
model receives the expected history and that reset removes previous
context. Record any API-specific limitations.

### Git checkpoint

``` text
feat(phase-04): add chat session and generation configuration
```

Push and verify.

### Gate

Do not claim KV-cache management is implemented merely because the chat
session works. Phase 5 must investigate and document the runtime cache
behavior separately.

------------------------------------------------------------------------

## Phase 5 --- KV cache understanding and controlled experiment

**Objective:** Understand how OpenVINO GenAI manages attention key/value
state and measure the effect of increasing context length.

### Required conceptual understanding

For autoregressive transformer generation, each attention layer computes
key and value tensors for tokens. The KV cache stores these states so
that previously processed tokens do not need to be recomputed at every
generation step.

The approximate cache size depends on:

-   Number of transformer layers.
-   Number of KV heads.
-   Head dimension.
-   Cache data type/precision.
-   Batch size.
-   Number of cached tokens.

A common estimate is:

``` text
KV cache bytes ≈
2 × layers × cached_tokens × KV_heads × head_dimension
  × batch_size × bytes_per_element
```

This is an estimate, not a substitute for measuring the actual runtime.
Architecture-specific details and runtime allocation can change observed
memory.

### Important distinctions

-   Conversation history is the logical text/messages retained by the
    application.
-   KV cache is the model's cached attention state.
-   Weight quantization compresses model weights.
-   KV-cache quantization, where supported, changes the
    precision/storage of cached attention state.
-   These are related but not interchangeable.

### Tasks

1.  Read the current official OpenVINO GenAI KV-cache and
    state-management documentation.
2.  Inspect the installed release's supported cache configuration
    options.
3.  Document whether the selected pipeline manages KV cache
    automatically.
4.  Identify which cache controls are actually exposed in the installed
    C++ API.
5.  Do not implement a custom cache unless the assignment requires it
    and the official API cannot meet the requirement.
6.  Design a fixed prompt set with increasing input/context lengths.
7.  Run tests at several context lengths that fit the machine, for
    example 128, 512, 1,024, and 2,048 tokens.
8.  Measure latency and process memory before and after generation.
9.  Repeat runs and record the environment and model precision.
10. If a documented cache-size limit, cache eviction, or cache
    quantization feature is supported, run a separate controlled test
    and document its configuration.
11. If a feature is not supported by the installed version/device/model,
    record it as unsupported rather than simulating it.

### Acceptance criteria

-   [ ] `docs/KV_CACHE.md` explains keys, values, token growth, and
    memory implications accurately.
-   [ ] Application-level chat history is distinguished from runtime KV
    cache.
-   [ ] The runtime's actual cache-management behavior is documented
    with official references.
-   [ ] At least three context lengths are tested, if hardware permits.
-   [ ] Memory and latency observations are recorded with methodology.
-   [ ] Any cache limit/eviction/quantization feature is used only if
    verified in the installed API.
-   [ ] Unsupported features are explicitly identified.
-   [ ] No fabricated cache measurements or unsupported claims appear in
    the report.

### Git checkpoint

``` text
feat(phase-05): document and benchmark OpenVINO KV cache behavior
```

Push and verify.

### Gate

Do not start Phase 6 until the KV-cache report is complete and the user
has reviewed the distinction between application history and runtime
cache.

------------------------------------------------------------------------

## Phase 6 --- Quantization and model-quality comparison

**Objective:** Compare model precision variants using a controlled and
repeatable methodology.

### Tasks

1.  Select two or more precision variants supported by the chosen model
    and export tool.
2.  Prefer a baseline plus at least one compressed-weight variant.
3.  Keep model architecture, tokenizer, prompts, generation settings,
    and device constant where possible.
4.  Record model directory size and process memory.
5.  Measure load time, TTFT, TPOT, throughput, and total generation
    time.
6.  Use a fixed prompt suite covering factual explanation,
    summarization, instruction following, and simple reasoning.
7.  Use the same maximum output-token limit and decoding settings.
8.  Compare output quality with a documented rubric.
9.  Record any export warnings or unsupported quantization operations.
10. Explain the trade-off between memory, speed, and quality.

### Quality rubric

Score each output using the same criteria:

-   Relevance to the prompt.
-   Factual consistency.
-   Instruction adherence.
-   Completeness.
-   Readability.

Use a consistent 1--5 scale and preserve example outputs. Human review
is acceptable. Do not claim a statistically validated quality
improvement from a tiny subjective sample.

### Acceptance criteria

-   [ ] At least two valid precision variants are evaluated, if
    supported by the selected model.
-   [ ] Exact export commands and precision settings are recorded.
-   [ ] Same prompt suite and generation settings are used for fair
    comparison.
-   [ ] Model size and measured memory are recorded.
-   [ ] TTFT, TPOT, throughput, and total time are recorded where
    available.
-   [ ] Output-quality rubric and sample results are included.
-   [ ] No performance or quality result is invented.
-   [ ] `docs/QUANTIZATION.md` explains the measured trade-offs.

### Tests / verification

Run each variant with the same benchmark prompts and settings. Use
warm-up runs and multiple measured repetitions. Record failed or skipped
runs rather than silently excluding them.

### Git checkpoint

``` text
feat(phase-06): add quantization comparison and quality evaluation
```

Push and verify.

### Gate

Do not start Phase 7 until the quantization report contains reproducible
data or a clearly documented hardware/model limitation.

------------------------------------------------------------------------

## Phase 7 --- Benchmarking and resource profiling

**Objective:** Produce defensible performance evidence for the final
internship submission.

### Metrics to capture

  -----------------------------------------------------------------------
  Metric                  Meaning                 Source/method
  ----------------------- ----------------------- -----------------------
  Model load time         Time to initialize the  Runtime timing
                          pipeline/model          

  TTFT                    Time to first generated OpenVINO GenAI
                          token                   performance metrics

  TPOT                    Average time per output OpenVINO GenAI
                          token                   performance metrics

  Throughput              Generated tokens per    OpenVINO GenAI
                          second                  performance metrics

  Input tokens            Prompt token count      Pipeline
                                                  metrics/tokenizer

  Output tokens           Generated token count   Pipeline metrics/result

  Total generation time   Full generation         Pipeline metrics
                          duration                

  Peak process memory     Maximum process memory  OS process monitor
                          during run              

  Device utilization      CPU/GPU/NPU utilization OS/vendor monitoring
                          where measurable        tool
  -----------------------------------------------------------------------

OpenVINO GenAI exposes performance metrics such as TTFT, TPOT,
throughput, generation duration, token counts, and load time. Use the
actual metrics exposed by the installed C++ API. Do not assume process
RAM or hardware utilization is provided by `PerfMetrics`; collect those
separately.

### Benchmark methodology

1.  Record machine configuration, OS, runtime version, model ID,
    precision, and device.
2.  Use a fixed prompt dataset checked into the repository.
3.  Use fixed generation settings.
4.  Separate model-load/cold-start time from warm inference.
5.  Run at least one warm-up request before measured requests.
6.  Run a minimum of five measured repetitions per test configuration;
    ten or more is preferable when practical.
7.  Report mean and standard deviation where enough repetitions exist.
8.  Keep the same device, prompt, output limit, and model precision when
    comparing one variable.
9.  Do not compare results from different machines as if they were a
    controlled experiment.
10. Save raw measurements to CSV.
11. Generate plots from the raw CSV; do not manually invent graph
    values.
12. Include failed runs and explain exclusions.

### Required outputs

-   `benchmarks/prompts.json`
-   `benchmarks/results/<run-id>.csv`
-   `docs/BENCHMARK_METHODOLOGY.md`
-   Benchmark plots, if generated.
-   `docs/phase-reports/PHASE_07_REPORT.md`

### Acceptance criteria

-   [ ] Benchmark prompts and settings are version-controlled.
-   [ ] At least five measured repetitions are completed per primary
    configuration, where hardware permits.
-   [ ] Warm-up and cold-start methodology is documented.
-   [ ] Raw results are saved as CSV.
-   [ ] Mean and standard deviation are reported where appropriate.
-   [ ] Metrics are sourced from the runtime or a documented OS monitor.
-   [ ] Comparisons change one major variable at a time.
-   [ ] Results can be regenerated using documented commands.
-   [ ] Limitations and skipped tests are transparent.

### Git checkpoint

``` text
feat(phase-07): add reproducible inference benchmark and profiling reports
```

Push and verify.

### Gate

Do not begin optional UI work until the benchmark methodology and raw
result files have been reviewed.

------------------------------------------------------------------------

## Phase 8 --- Optional local interface and usability polish

**Objective:** Make the completed inference engine easier to demonstrate
without weakening the local C++ inference requirement.

This phase is optional for technical completion. The core assignment can
be satisfied with a robust CLI. Only start this phase after Phases 0--7
are complete and approved.

### Possible features

-   Clean local chat interface.
-   Model/device/precision information.
-   Prompt and response display.
-   Generation settings.
-   New-chat/reset control.
-   TTFT, TPOT, throughput, and memory display.
-   Benchmark history and comparison charts.
-   Clear indication that inference runs locally.

### Architecture constraint

The UI must call the local C++ inference engine. It must not replace the
C++ inference path with a Python LLM API or cloud service.

If an HTTP bridge is added, it must bind to localhost by default,
validate inputs, handle errors, and document its API. Do not expose an
unauthenticated inference server to the public network.

### Acceptance criteria

-   [ ] UI invokes the local C++ engine.
-   [ ] No cloud LLM inference is introduced.
-   [ ] Errors and loading states are handled.
-   [ ] Device/model/precision shown in the UI match actual runtime
    configuration.
-   [ ] Metrics shown are real measurements.
-   [ ] Local-only network binding is verified.

### Git checkpoint

``` text
feat(phase-08): add optional local inference dashboard
```

Push and verify.

------------------------------------------------------------------------

## Phase 9 --- Final hardening, documentation, and submission package

**Objective:** Prepare a reproducible, professional GitHub submission
and demonstration.

### Tasks

1.  Perform a clean checkout into a fresh directory or clean worktree.
2.  Follow README instructions from scratch.
3.  Rebuild the C++ application.
4.  Download/prepare the model using documented instructions.
5.  Run smoke inference.
6.  Run the benchmark command on the available machine.
7.  Verify that no model weights, secrets, caches, or build output are
    tracked.
8.  Review code formatting and error handling.
9.  Review all documentation for inaccurate claims.
10. Create a final architecture diagram.
11. Create a final report summarizing design, implementation,
    experiments, results, and limitations.
12. Record a short demonstration video if required by the internship
    submission.
13. Verify all phase commits exist on GitHub.
14. Verify the repository's default branch contains the final commit.

### Required final deliverables

-   Working C++ source code.
-   CMake build configuration.
-   Model export/download instructions.
-   README with exact setup/build/run commands.
-   Architecture documentation.
-   Model and license details.
-   Quantization report.
-   KV-cache report.
-   Benchmark methodology and raw CSV results.
-   Screenshots or demonstration video, if available.
-   Phase reports and Git history.

### Acceptance criteria

-   [ ] Fresh checkout build succeeds.
-   [ ] Local model inference succeeds.
-   [ ] Runtime path does not call a hosted LLM API.
-   [ ] Required tests pass.
-   [ ] Reports contain real evidence and limitations.
-   [ ] GitHub repository contains all source and documentation.
-   [ ] GitHub repository does not contain model weights, secrets, or
    generated build files.
-   [ ] Every mandatory phase has a verified commit and report.
-   [ ] README is sufficient for another developer to reproduce the core
    demo.

### Git checkpoint

``` text
docs(phase-09): finalize implementation documentation and submission package
```

Push and verify. Request final user review.

------------------------------------------------------------------------

# 5. GitHub commit and push protocol

Every completed phase must have its own commit and push. Do not
accumulate several completed phases into one commit.

## 5.1 Branch policy

-   Use the existing project branch if the user already has a
    convention.
-   Otherwise, use `main` for this solo project unless the user requests
    a development branch.
-   Do not create a new remote repository or change visibility without
    user confirmation.
-   Do not rewrite published history.
-   Do not force-push.

## 5.2 Required sequence at the end of every phase

Run the equivalent of:

``` bash
git status
git diff --check
git diff
```

Review the staged file list. Ensure no secrets, model files, build
outputs, or unrelated files are included.

Then:

``` bash
git add <only-the-files-for-this-phase>
git commit -m "type(phase-XX): concise phase summary"
git push origin <current-branch>
```

After pushing, verify:

``` bash
git rev-parse HEAD
git ls-remote origin refs/heads/<current-branch>
git status
```

The local `HEAD` hash must match the remote branch hash returned by
`git ls-remote`. The working tree should be clean, except for explicitly
documented ignored/generated files.

If the push is rejected or remote verification fails:

1.  Mark the phase `BLOCKED`.
2.  Do not start the next phase.
3.  Preserve all local work.
4.  Inspect the remote/local divergence safely.
5.  Ask the user before resolving conflicts if user changes may be
    affected.
6.  Retry only after the issue is understood.

## 5.3 Commit message convention

Use Conventional Commit-style messages:

``` text
chore(phase-00): establish project workspace and hardware audit
build(phase-01): configure and verify OpenVINO GenAI C++ toolchain
feat(phase-02): add reproducible local model preparation workflow
feat(phase-03): implement local C++ LLM inference with OpenVINO GenAI
feat(phase-04): add chat session and generation configuration
feat(phase-05): document and benchmark OpenVINO KV cache behavior
feat(phase-06): add quantization comparison and quality evaluation
feat(phase-07): add reproducible inference benchmark and profiling reports
feat(phase-08): add optional local inference dashboard
docs(phase-09): finalize implementation documentation and submission package
```

## 5.4 Optional phase tags

After a phase is fully accepted and pushed, create an annotated tag if
the user wants milestone tags:

``` bash
git tag -a phase-03-complete -m "Phase 3 completed and verified"
git push origin phase-03-complete
```

Do not tag a phase before its gate is satisfied.

------------------------------------------------------------------------

# 6. Phase status tracking and enforcement

Create and maintain `docs/PHASE_STATUS.md`.

Use this status vocabulary:

-   `NOT_STARTED` --- work has not begun.
-   `IN_PROGRESS` --- current phase is being implemented.
-   `BLOCKED` --- a dependency, failure, or external action prevents
    completion.
-   `READY_FOR_REVIEW` --- implementation and tests are done, but user
    approval and/or final remote verification is pending.
-   `COMPLETE` --- all gate conditions passed, push verified, and user
    approved.

Suggested status table:

  ----------------------------------------------------------------------------
  Phase          Status        Local tests Commit      Remote      User
                                                       verified    approval
  -------------- ------------- ----------- ----------- ----------- -----------
  00 ---         NOT_STARTED   ---         ---         ---         ---
  Workspace                                                        
  audit                                                            

  01 --- C++     NOT_STARTED   ---         ---         ---         ---
  environment                                                      

  02 --- Model   NOT_STARTED   ---         ---         ---         ---
  preparation                                                      

  03 --- Local   NOT_STARTED   ---         ---         ---         ---
  inference                                                        

  04 --- Chat    NOT_STARTED   ---         ---         ---         ---
  sessions                                                         

  05 --- KV      NOT_STARTED   ---         ---         ---         ---
  cache                                                            

  06 ---         NOT_STARTED   ---         ---         ---         ---
  Quantization                                                     

  07 ---         NOT_STARTED   ---         ---         ---         ---
  Benchmarking                                                     

  08 ---         NOT_STARTED   ---         ---         ---         ---
  Optional UI                                                      

  09 --- Final   NOT_STARTED   ---         ---         ---         ---
  submission                                                       
  ----------------------------------------------------------------------------

For each phase report, include:

``` text
Phase:
Status:
Start date:
Completion date:
Environment:
Files changed:
Implementation summary:
Commands executed:
Test results:
Acceptance criteria checklist:
Known issues:
Skipped tests and reasons:
Commit hash:
Remote branch:
Remote hash:
Push verification:
User approval:
Next permitted phase:
```

## 6.1 Gate enforcement script

Implement `scripts/verify_phase.py` during Phase 0 or Phase 1. It
should:

1.  Read the phase status file or a structured status file.
2.  Refuse to validate Phase N if Phase N-1 is not `COMPLETE`.
3.  Refuse completion if required acceptance evidence is missing.
4.  Refuse completion if commit hash or remote verification is missing.
5.  Print the first unmet condition and exit with a non-zero status.
6.  Never automatically mark a phase complete.
7.  Never edit Git history or push by itself.

The script is a guardrail, not a replacement for actual tests or user
approval. The agent must still follow this document even if the script
is unavailable.

------------------------------------------------------------------------

# 7. Testing strategy

Use the simplest test that proves each behavior. Avoid adding a large
testing framework unless it is justified.

## 7.1 Test levels

1.  **Build tests:** CMake configure and compile.
2.  **Unit tests:** Input validation, configuration validation, session
    logic.
3.  **Smoke tests:** Load model and generate a short response.
4.  **Integration tests:** End-to-end local inference and chat session.
5.  **Performance tests:** Controlled prompt and generation
    measurements.
6.  **Reproducibility tests:** Fresh checkout setup and execution.

## 7.2 Test result labels

-   `PASS` --- executed and passed.
-   `FAIL` --- executed and failed.
-   `NOT RUN` --- not executed.
-   `BLOCKED` --- cannot execute due to a known external dependency.
-   `NOT APPLICABLE` --- genuinely irrelevant, with explanation.

Never convert `NOT RUN` or `BLOCKED` into `PASS`.

## 7.3 Definition of done for any code change

-   Code compiles.
-   Relevant tests pass.
-   No new compiler warnings are ignored without explanation.
-   Error paths are considered.
-   Documentation is updated.
-   No secrets or large model assets are added.
-   Current phase acceptance criteria remain satisfied.

------------------------------------------------------------------------

# 8. Benchmark data format

Save raw results in CSV. Suggested columns:

``` csv
run_id,timestamp,os,cpu,gpu,ram_gb,openvino_version,genai_version,model_id,precision,device,prompt_id,input_tokens,output_tokens,load_time_ms,ttft_ms,tpot_ms,throughput_tokens_sec,total_generation_ms,peak_process_memory_mb,notes
```

Use blank values when a metric is unavailable. Do not fill missing
values with estimates.

For comparisons, include: - Model and exact revision. - Precision/export
configuration. - Device. - Prompt ID and token counts. - Generation
configuration. - Number of warm-up runs. - Number of measured runs. -
Mean and standard deviation. - Any thermal, power, or background-load
caveats.

------------------------------------------------------------------------

# 9. Security, privacy, and reliability rules

1.  All inference must run locally through OpenVINO GenAI.
2.  Do not send prompts to external model providers.
3.  Do not add analytics or telemetry that uploads prompts.
4.  Do not expose a local server beyond localhost unless the user
    explicitly requests it.
5.  Keep model downloads and caches outside Git.
6.  Use `.env.example` only for non-secret variable names; never commit
    real `.env`.
7.  Do not log sensitive user prompts by default.
8.  Validate model paths and user input.
9.  Catch and report runtime exceptions.
10. Do not silently fall back from CPU/GPU/NPU to another device. If
    fallback is implemented, show and log the actual device used.
11. Do not claim a device was used merely because it was requested;
    verify actual runtime selection.
12. Document model licensing and attribution.

------------------------------------------------------------------------

# 10. Scope control: what NOT to build initially

Do not spend time on these before the mandatory phases are complete:

-   Model training or fine-tuning.
-   RAG or vector databases.
-   Multi-agent workflows.
-   Cloud deployment.
-   Authentication systems.
-   User accounts and persistent databases.
-   Complex React dashboards.
-   Docker/Kubernetes deployment.
-   Multi-model orchestration.
-   Custom transformer implementation.
-   Custom KV-cache implementation when the OpenVINO pipeline already
    manages it.

These features do not prove the core assignment better than a reliable
C++ local inference engine and controlled optimization experiments.

------------------------------------------------------------------------

# 11. Final demonstration plan

The final demo should be short and evidence-driven:

1.  Show the machine/runtime and selected model.
2.  Show that the model files are local.
3.  Build and launch the C++ application.
4.  Enter a prompt and display the generated response.
5.  Run a short multi-turn conversation.
6.  Show the actual device used.
7.  Show TTFT, TPOT, throughput, and memory measurements.
8.  Compare at least two precision variants if supported.
9.  Explain KV cache and show the context-length experiment.
10. Show the GitHub repository, phase reports, and phase-wise commit
    history.

Do not edit benchmark values into the demo. Use actual outputs from the
recorded runs.

------------------------------------------------------------------------

# 12. Official technical references

Use current official documentation for the exact installed release. If
documentation and installed headers disagree, investigate the version
mismatch and report it; do not guess.

1.  OpenVINO core repository:
    https://github.com/openvinotoolkit/openvino
2.  OpenVINO GenAI repository:
    https://github.com/openvinotoolkit/openvino.genai
3.  OpenVINO GenAI documentation:
    https://openvinotoolkit.github.io/openvino.genai/
4.  Supported models:
    https://openvinotoolkit.github.io/openvino.genai/docs/supported-models/
5.  Model conversion:
    https://openvinotoolkit.github.io/openvino.genai/docs/guides/model-preparation/convert-to-openvino/
6.  Performance metrics:
    https://openvinotoolkit.github.io/openvino.genai/docs/guides/performance-metrics/
7.  OpenVINO GenAI build instructions:
    https://github.com/openvinotoolkit/openvino.genai/blob/master/src/docs/BUILD.md

------------------------------------------------------------------------

# 13. Copy-ready Antigravity master instruction

Paste the following instruction into Antigravity after placing this
blueprint in the project root.

> Read `IMPLEMENTATION_BLUEPRINT.md` completely before taking action.
>
> Treat it as the project's binding implementation contract.
>
> Start with Phase 0 only. Inspect the workspace, Git status, current
> branch, remotes, OS, hardware, toolchain, and existing files. Do not
> assume hardware specifications or overwrite existing work.
>
> Before coding, report the current phase, its objective, planned files,
> required tests, and acceptance criteria.
>
> Implement only the current phase. Do not begin a later phase, create
> future-phase functionality, or mark a phase complete while any
> current-phase requirement is unresolved.
>
> At the end of the phase, run every required test, record the exact
> commands and real results in `docs/phase-reports/PHASE_XX_REPORT.md`,
> update `docs/PHASE_STATUS.md`, review the diff, and check for
> secrets/model files/build artifacts.
>
> Commit and push that phase as a separate Git commit. Verify that the
> GitHub remote branch contains the exact local commit hash. If there is
> no remote, authentication fails, tests fail, or remote verification
> fails, mark the phase BLOCKED and stop.
>
> A phase is not COMPLETE until its tests pass, its report and evidence
> are present, its commit is pushed and verified, and I explicitly
> approve the phase. After presenting the report, STOP and wait for my
> approval before starting the next phase.
>
> Never fabricate test results, performance numbers, hardware support,
> or GitHub push success. Never use a cloud LLM API for inference. The
> actual inference engine must use the OpenVINO GenAI C++ API.
>
> Your first response must be a Phase 0 audit plan and a list of
> information you need from me. Do not start Phase 1 in the same session
> unless Phase 0 has passed its gate and I have approved it.

------------------------------------------------------------------------

# 14. Immediate next action

**Start with Phase 0 only.** The first Antigravity task is to audit the
machine and repository, verify the GitHub remote, and establish the
project controls. It must not install a random stack, download a model,
or start inference before that audit is complete.

**The governing rule is simple: no evidence, no completion; no verified
push, no phase closure; no user approval, no next phase.**
