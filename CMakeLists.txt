cmake_minimum_required(VERSION 3.20)
project(local_llm_openvino CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find OpenVINO Core Runtime
find_package(OpenVINO REQUIRED COMPONENTS Runtime)

# Find OpenVINO GenAI
find_package(OpenVINOGenAI REQUIRED)

# Common library for LLM Engine
add_library(ov_llm_core STATIC
    src/llm_engine.cpp
    src/llm_engine.hpp
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

# Phase 03 Target: Main Local LLM Inference Application
add_executable(llm_inference src/main.cpp)
target_link_libraries(llm_inference PRIVATE ov_llm_core)

# Phase 03 Target: Smoke & Unit Tests
add_executable(test_smoke_inference tests/test_smoke_inference.cpp)
target_link_libraries(test_smoke_inference PRIVATE ov_llm_core)
