cmake_minimum_required(VERSION 3.20)
project(local_llm_openvino CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find OpenVINO Core Runtime
find_package(OpenVINO REQUIRED COMPONENTS Runtime)

# Find OpenVINO GenAI
find_package(OpenVINOGenAI REQUIRED)

# Phase 01: Environment Verification Target
add_executable(env_check src/main.cpp)

target_link_libraries(env_check PRIVATE
    openvino::runtime
    openvino::genai
)

# On Windows, copy runtime DLLs to output directory or configure path
if(WIN32)
    add_custom_command(TARGET env_check POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Build complete for env_check. Ready to run with OpenVINO runtime environment."
    )
endif()

