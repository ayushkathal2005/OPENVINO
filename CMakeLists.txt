cmake_minimum_required(VERSION 3.20)
project(local_llm_openvino CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find OpenVINO Core Runtime
find_package(OpenVINO REQUIRED COMPONENTS Runtime)

# Find OpenVINO GenAI
find_package(OpenVINOGenAI REQUIRED)

# Common library for LLM Engine and Chat Session
add_library(ov_llm_core STATIC
    src/llm_engine.cpp
    src/llm_engine.hpp
    src/chat_session.cpp
    src/chat_session.hpp
)

target_include_directories(ov_llm_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ov_llm_core PUBLIC
    openvino::runtime
    openvino::genai
)

# Phase 01 Target: Environment Verification
add_executable(env_check src/main.cpp)
target_link_libraries(env_check PRIVATE ov_llm_core)

# Phase 03/04 Target: Main Local LLM Inference Application
add_executable(llm_inference src/main.cpp)
target_link_libraries(llm_inference PRIVATE ov_llm_core)

# Phase 05 Target: KV Cache Scaling Benchmark
add_executable(benchmark_kv src/benchmark_kv.cpp)
target_link_libraries(benchmark_kv PRIVATE ov_llm_core psapi)

# Phase 06 Target: Quantization Comparison Benchmark
add_executable(benchmark_quantization src/benchmark_quantization.cpp)
target_link_libraries(benchmark_quantization PRIVATE ov_llm_core psapi)

# Phase 07 Target: Multi-Run Benchmark Suite
add_executable(benchmark_suite src/benchmark_suite.cpp)
target_link_libraries(benchmark_suite PRIVATE ov_llm_core psapi)

# Automated Tests
enable_testing()

# Phase 03 Target: Smoke & Unit Tests
add_executable(test_smoke_inference tests/test_smoke_inference.cpp)
target_link_libraries(test_smoke_inference PRIVATE ov_llm_core)
add_test(NAME test_smoke_inference COMMAND test_smoke_inference)
set_tests_properties(test_smoke_inference PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

# Phase 04 Target: Chat Session & Generation Controls Tests
add_executable(test_chat_session tests/test_chat_session.cpp)
target_link_libraries(test_chat_session PRIVATE ov_llm_core)
add_test(NAME test_chat_session COMMAND test_chat_session)
set_tests_properties(test_chat_session PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
