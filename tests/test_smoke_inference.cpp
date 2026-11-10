#include <iostream>
#include <cassert>
#include <string>
#include "../src/llm_engine.hpp"

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "  Running Phase 03 C++ Local Inference Smoke Tests" << std::endl;
    std::cout << "==================================================" << std::endl;

    int tests_passed = 0;
    int total_tests = 0;

    auto run_test = [&](const std::string& name, auto test_fn) {
        total_tests++;
        std::cout << "\n[TEST " << total_tests << "] " << name << "..." << std::endl;
        try {
            if (test_fn()) {
                std::cout << "  -> PASS" << std::endl;
                tests_passed++;
            } else {
                std::cout << "  -> FAIL" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "  -> EXCEPTION: " << e.what() << std::endl;
            std::cout << "  -> FAIL" << std::endl;
        }
    };

    // Test 1: Empty model path validation
    run_test("Validation: Empty model path rejected", []() {
        std::string err;
        bool ok = ov_llm::LLMEngine::validate_model_path("", err);
        return (!ok && !err.empty());
    });

    // Test 2: Non-existent model path validation
    run_test("Validation: Non-existent model path rejected", []() {
        std::string err;
        bool ok = ov_llm::LLMEngine::validate_model_path("non_existent_path_12345", err);
        return (!ok && !err.empty());
    });

    // Test 3: Invalid device validation
    run_test("Validation: Unsupported device rejected", []() {
        std::string err;
        bool ok = ov_llm::LLMEngine::validate_device("INVALID_ACCELERATOR_DEVICE", err);
        return (!ok && !err.empty());
    });

    // Test 4: Valid CPU device validation
    run_test("Validation: CPU device accepted", []() {
        std::string err;
        bool ok = ov_llm::LLMEngine::validate_device("CPU", err);
        return (ok && err.empty());
    });

    // Test 5: Valid model path validation
    run_test("Validation: Real FP16 model directory accepted", []() {
        std::string err;
        bool ok = ov_llm::LLMEngine::validate_model_path("models/tinyllama-1.1b-chat-fp16", err);
        return (ok && err.empty());
    });

    // Test 6: Model loading into memory
    ov_llm::LLMEngine engine;
    run_test("Execution: Model load on CPU", [&]() {
        return engine.load_model("models/tinyllama-1.1b-chat-fp16", "CPU");
    });

    // Test 7: Empty prompt handling
    run_test("Execution: Empty prompt returns safely without crash", [&]() {
        std::string out = engine.generate("");
        return out.empty();
    });

    // Test 8: Valid prompt generates non-empty output
    run_test("Execution: Valid prompt produces non-empty output", [&]() {
        ov_llm::GenerationParameters params;
        params.max_new_tokens = 32;
        params.temperature = 0.0f; // deterministic
        params.do_sample = false;

        std::string prompt = "What is the capital of France? Answer:";
        std::cout << "  Prompt: " << prompt << std::endl;
        std::string out = engine.generate(prompt, params);
        std::cout << "  Output: " << out << std::endl;
        return !out.empty();
    });

    // Test 9: Sequential inference consistency
    run_test("Execution: Second sequential prompt produces non-empty output", [&]() {
        ov_llm::GenerationParameters params;
        params.max_new_tokens = 24;
        params.temperature = 0.0f;
        params.do_sample = false;

        std::string prompt = "2 + 2 = ";
        std::cout << "  Prompt: " << prompt << std::endl;
        std::string out = engine.generate(prompt, params);
        std::cout << "  Output: " << out << std::endl;
        return !out.empty();
    });

    std::cout << "\n==================================================" << std::endl;
    std::cout << "  Test Summary: " << tests_passed << "/" << total_tests << " Passed" << std::endl;
    std::cout << "==================================================" << std::endl;

    return (tests_passed == total_tests) ? 0 : 1;
}
