#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>

#include <windows.h>
#include <psapi.h>

#include <openvino/genai/llm_pipeline.hpp>
#include <openvino/genai/perf_metrics.hpp>
#include "llm_engine.hpp"

// Utility to query current process Working Set (Resident RAM) in bytes
size_t get_process_working_set_bytes() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;
}

double bytes_to_mb(size_t bytes) {
    return static_cast<double>(bytes) / (1024.0 * 1024.0);
}

// Generate prompt of approximately target_tokens using repeated natural sentences
std::string synthesize_prompt(ov::genai::Tokenizer& tokenizer, size_t target_tokens) {
    const std::string seed_phrase = "The quick brown fox jumps over the lazy dog in the tranquil summer forest. ";
    std::string prompt = seed_phrase;

    // Expand prompt until token count exceeds target_tokens
    while (true) {
        auto encoded = tokenizer.encode(prompt);
        size_t token_count = encoded.input_ids.get_shape()[1];
        if (token_count >= target_tokens) {
            break;
        }
        prompt += seed_phrase;
    }

    // Truncate to exact target_tokens by slicing tokens and decoding back
    auto encoded = tokenizer.encode(prompt);
    int64_t* token_data = encoded.input_ids.data<int64_t>();
    std::vector<int64_t> exact_tokens(token_data, token_data + target_tokens);
    return tokenizer.decode(exact_tokens);
}

struct BenchmarkResult {
    size_t target_context_length;
    size_t actual_input_tokens;
    size_t output_tokens;
    double ttft_ms;
    double tpot_ms;
    double throughput_tps;
    double total_infer_ms;
    double ram_before_mb;
    double ram_after_mb;
    double ram_delta_mb;
};

BenchmarkResult run_context_benchmark(
    ov::genai::LLMPipeline& pipeline,
    ov::genai::Tokenizer& tokenizer,
    size_t target_tokens,
    size_t max_new_tokens = 32
) {
    BenchmarkResult res;
    res.target_context_length = target_tokens;
    res.output_tokens = max_new_tokens;

    std::string prompt = synthesize_prompt(tokenizer, target_tokens);
    auto token_check = tokenizer.encode(prompt);
    res.actual_input_tokens = token_check.input_ids.get_shape()[1];

    ov::genai::GenerationConfig config;
    config.max_new_tokens = max_new_tokens;
    config.do_sample = false;
    config.temperature = 0.0f; // deterministic greedy

    res.ram_before_mb = bytes_to_mb(get_process_working_set_bytes());

    auto t_start = std::chrono::high_resolution_clock::now();
    ov::genai::DecodedResults results = pipeline.generate(prompt, config);
    auto t_end = std::chrono::high_resolution_clock::now();

    res.ram_after_mb = bytes_to_mb(get_process_working_set_bytes());
    res.ram_delta_mb = res.ram_after_mb - res.ram_before_mb;

    ov::genai::PerfMetrics metrics = results.perf_metrics;
    res.ttft_ms = metrics.get_ttft().mean;
    res.tpot_ms = metrics.get_tpot().mean;
    res.throughput_tps = metrics.get_throughput().mean;
    res.total_infer_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

    return res;
}

int main(int argc, char* argv[]) {
    std::string model_path = "models/tinyllama-1.1b-chat-int4";
    std::string device = "CPU";

    if (argc > 1) {
        model_path = argv[1];
    }
    if (argc > 2) {
        device = argv[2];
    }

    std::cout << "======================================================================" << std::endl;
    std::cout << "   Phase 05: OpenVINO GenAI KV Cache Controlled Scaling Benchmark     " << std::endl;
    std::cout << "======================================================================" << std::endl;
    std::cout << "Model:  " << model_path << std::endl;
    std::cout << "Device: " << device << std::endl;

    size_t initial_ram_bytes = get_process_working_set_bytes();
    std::cout << "Initial Process RAM: " << std::fixed << std::setprecision(2)
              << bytes_to_mb(initial_ram_bytes) << " MB" << std::endl;

    std::cout << "\nLoading model into OpenVINO GenAI pipeline..." << std::endl;
    ov::genai::LLMPipeline pipeline(model_path, device);
    ov::genai::Tokenizer tokenizer = pipeline.get_tokenizer();

    size_t loaded_ram_bytes = get_process_working_set_bytes();
    std::cout << "RAM After Model Load: " << bytes_to_mb(loaded_ram_bytes) << " MB "
              << "(Model Footprint in RAM: +" << bytes_to_mb(loaded_ram_bytes - initial_ram_bytes) << " MB)\n" << std::endl;

    // Warm-up run
    std::cout << "Running warm-up inference (64 tokens)..." << std::endl;
    ov::genai::GenerationConfig warmup_cfg;
    warmup_cfg.max_new_tokens = 16;
    warmup_cfg.do_sample = false;
    pipeline.generate("Warm-up prompt for OpenVINO pipeline compilation.", warmup_cfg);
    std::cout << "Warm-up complete.\n" << std::endl;

    // Context length tiers: 128, 512, 1024, 2048
    std::vector<size_t> test_contexts = {128, 512, 1024, 2048};
    std::vector<BenchmarkResult> results;

    std::cout << "Executing Controlled Context Length Scaling Benchmark (Generating 32 tokens):\n" << std::endl;

    for (size_t ctx : test_contexts) {
        std::cout << "--> Testing context length: " << ctx << " tokens..." << std::flush;
        BenchmarkResult r = run_context_benchmark(pipeline, tokenizer, ctx, 32);
        results.push_back(r);
        std::cout << " Done. (TTFT: " << std::setprecision(2) << r.ttft_ms << " ms, TPOT: "
                  << r.tpot_ms << " ms/tok, RAM: " << r.ram_after_mb << " MB)" << std::endl;
    }

    std::cout << "\n========================================================================================================" << std::endl;
    std::cout << "                             OpenVINO GenAI KV Cache Benchmark Results Table                            " << std::endl;
    std::cout << "========================================================================================================" << std::endl;
    std::cout << std::left
              << std::setw(14) << "Target Ctx"
              << std::setw(14) << "Input Tokens"
              << std::setw(12) << "New Tokens"
              << std::setw(14) << "TTFT (ms)"
              << std::setw(16) << "TPOT (ms/tok)"
              << std::setw(18) << "Throughput (t/s)"
              << std::setw(16) << "Total Time (ms)"
              << std::setw(14) << "RAM Peak (MB)"
              << std::endl;
    std::cout << "--------------------------------------------------------------------------------------------------------" << std::endl;

    for (const auto& r : results) {
        std::cout << std::left
                  << std::setw(14) << r.target_context_length
                  << std::setw(14) << r.actual_input_tokens
                  << std::setw(12) << r.output_tokens
                  << std::setw(14) << std::fixed << std::setprecision(2) << r.ttft_ms
                  << std::setw(16) << std::fixed << std::setprecision(2) << r.tpot_ms
                  << std::setw(18) << std::fixed << std::setprecision(2) << r.throughput_tps
                  << std::setw(16) << std::fixed << std::setprecision(2) << r.total_infer_ms
                  << std::setw(14) << std::fixed << std::setprecision(2) << r.ram_after_mb
                  << std::endl;
    }
    std::cout << "========================================================================================================\n" << std::endl;

    return 0;
}
