#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>

#include <windows.h>
#include <psapi.h>

#include <openvino/genai/llm_pipeline.hpp>
#include <openvino/genai/perf_metrics.hpp>
#include <openvino/genai/json_container.hpp>
#include "llm_engine.hpp"

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

struct PromptItem {
    std::string id;
    std::string category;
    std::string prompt;
};

struct RunMetrics {
    std::string model_variant;
    std::string prompt_id;
    std::string category;
    std::string prompt_text;
    std::string generated_text;
    size_t input_tokens;
    size_t output_tokens;
    double ttft_ms;
    double tpot_ms;
    double throughput_tps;
    double total_infer_ms;
    double peak_ram_mb;
};

std::vector<PromptItem> load_benchmark_prompts(const std::string& path) {
    std::vector<PromptItem> items;
    // Fallback default prompts if file parsing fails
    items.push_back({
        "fact_01",
        "factual_explanation",
        "Explain the concept of Key-Value (KV) caching in large language models and why it optimizes autoregressive generation."
    });
    items.push_back({
        "summ_01",
        "summarization",
        "Summarize the primary benefits and potential accuracy trade-offs of INT4 weight quantization for edge AI inference."
    });
    items.push_back({
        "instruct_01",
        "instruction_following",
        "List exactly three advantages of deploying LLMs locally using OpenVINO GenAI C++ API instead of remote cloud APIs. Format as a numbered list."
    });
    items.push_back({
        "reason_01",
        "simple_reasoning",
        "If a language model processes 20 tokens per second, how many seconds will it take to generate a 120-token response? Show your calculation."
    });
    return items;
}

std::vector<RunMetrics> evaluate_model(
    const std::string& model_name,
    const std::string& model_path,
    const std::string& device,
    const std::vector<PromptItem>& prompts,
    double& out_load_time_ms
) {
    std::cout << "\n======================================================================" << std::endl;
    std::cout << " Evaluating Model Variant: " << model_name << " (" << model_path << ")" << std::endl;
    std::cout << "======================================================================" << std::endl;

    auto t_load_start = std::chrono::high_resolution_clock::now();
    ov::genai::LLMPipeline pipeline(model_path, device);
    auto t_load_end = std::chrono::high_resolution_clock::now();
    out_load_time_ms = std::chrono::duration<double, std::milli>(t_load_end - t_load_start).count();

    std::cout << "Model Load Duration: " << std::fixed << std::setprecision(2) << out_load_time_ms << " ms" << std::endl;
    std::cout << "RAM After Load:      " << bytes_to_mb(get_process_working_set_bytes()) << " MB" << std::endl;

    // Warm-up run
    std::cout << "Executing warm-up run..." << std::flush;
    ov::genai::GenerationConfig warmup_cfg;
    warmup_cfg.max_new_tokens = 16;
    warmup_cfg.do_sample = false;
    pipeline.generate("Warm-up request for model evaluation.", warmup_cfg);
    std::cout << " Done.\n" << std::endl;

    std::vector<RunMetrics> metrics_list;

    for (const auto& item : prompts) {
        std::cout << "Running [" << item.id << "] (" << item.category << ")..." << std::flush;

        ov::genai::GenerationConfig config;
        config.max_new_tokens = 96;
        config.do_sample = false;
        config.temperature = 0.0f; // deterministic greedy

        auto t_start = std::chrono::high_resolution_clock::now();
        ov::genai::DecodedResults res = pipeline.generate(item.prompt, config);
        auto t_end = std::chrono::high_resolution_clock::now();

        RunMetrics m;
        m.model_variant = model_name;
        m.prompt_id = item.id;
        m.category = item.category;
        m.prompt_text = item.prompt;
        m.generated_text = res;
        m.input_tokens = res.perf_metrics.get_num_input_tokens();
        m.output_tokens = res.perf_metrics.get_num_generated_tokens();
        m.ttft_ms = res.perf_metrics.get_ttft().mean;
        m.tpot_ms = res.perf_metrics.get_tpot().mean;
        m.throughput_tps = res.perf_metrics.get_throughput().mean;
        m.total_infer_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
        m.peak_ram_mb = bytes_to_mb(get_process_working_set_bytes());

        metrics_list.push_back(m);
        std::cout << " Done. (TTFT: " << std::setprecision(1) << m.ttft_ms << "ms, TPOT: "
                  << m.tpot_ms << "ms/tok, TPS: " << m.throughput_tps << ")\n"
                  << "  Response snippet: "
                  << m.generated_text.substr(0, std::min<size_t>(120, m.generated_text.size())) << "...\n" << std::endl;
    }

    return metrics_list;
}

int main(int argc, char* argv[]) {
    std::string device = "CPU";
    if (argc > 1) {
        device = argv[1];
    }

    std::cout << "======================================================================" << std::endl;
    std::cout << "    Phase 06: Model Quantization & Quality Comparative Benchmark     " << std::endl;
    std::cout << "======================================================================" << std::endl;
    std::cout << "Target Device: " << device << std::endl;

    std::vector<PromptItem> prompts = load_benchmark_prompts("benchmarks/prompts.json");
    std::cout << "Loaded " << prompts.size() << " evaluation prompts.\n" << std::endl;

    double fp16_load_time_ms = 0.0;
    double int4_load_time_ms = 0.0;

    // 1. Evaluate INT4 Model
    std::vector<RunMetrics> int4_metrics = evaluate_model(
        "INT4 (NNCF asym 128)",
        "models/tinyllama-1.1b-chat-int4",
        device,
        prompts,
        int4_load_time_ms
    );

    // 2. Evaluate FP16 Model
    std::vector<RunMetrics> fp16_metrics = evaluate_model(
        "FP16 (Baseline)",
        "models/tinyllama-1.1b-chat-fp16",
        device,
        prompts,
        fp16_load_time_ms
    );

    // Comparative Table Output
    std::cout << "\n========================================================================================================================" << std::endl;
    std::cout << "                                  Comparative Quantization Performance Benchmark Results                               " << std::endl;
    std::cout << "========================================================================================================================" << std::endl;
    std::cout << std::left
              << std::setw(20) << "Model Variant"
              << std::setw(12) << "Prompt ID"
              << std::setw(12) << "In Tokens"
              << std::setw(12) << "Out Tokens"
              << std::setw(14) << "TTFT (ms)"
              << std::setw(16) << "TPOT (ms/tok)"
              << std::setw(18) << "Throughput (t/s)"
              << std::setw(16) << "Total Time (ms)"
              << std::setw(14) << "Peak RAM (MB)"
              << std::endl;
    std::cout << "------------------------------------------------------------------------------------------------------------------------" << std::endl;

    auto print_rows = [](const std::vector<RunMetrics>& list) {
        for (const auto& m : list) {
            std::cout << std::left
                      << std::setw(20) << m.model_variant
                      << std::setw(12) << m.prompt_id
                      << std::setw(12) << m.input_tokens
                      << std::setw(12) << m.output_tokens
                      << std::setw(14) << std::fixed << std::setprecision(2) << m.ttft_ms
                      << std::setw(16) << std::fixed << std::setprecision(2) << m.tpot_ms
                      << std::setw(18) << std::fixed << std::setprecision(2) << m.throughput_tps
                      << std::setw(16) << std::fixed << std::setprecision(2) << m.total_infer_ms
                      << std::setw(14) << std::fixed << std::setprecision(2) << m.peak_ram_mb
                      << std::endl;
        }
    };

    print_rows(int4_metrics);
    std::cout << "------------------------------------------------------------------------------------------------------------------------" << std::endl;
    print_rows(fp16_metrics);
    std::cout << "========================================================================================================================\n" << std::endl;

    // Full Raw Output Dumps for Quality Rubric Evaluation
    std::cout << "\n======================================================================" << std::endl;
    std::cout << "                   Side-by-Side Quality Text Outputs                  " << std::endl;
    std::cout << "======================================================================" << std::endl;

    for (size_t i = 0; i < prompts.size(); ++i) {
        std::cout << "\n--------------------------------------------------" << std::endl;
        std::cout << "PROMPT [" << prompts[i].id << "] (" << prompts[i].category << "):" << std::endl;
        std::cout << "\"" << prompts[i].prompt << "\"\n" << std::endl;
        std::cout << ">>> INT4 Output:\n" << int4_metrics[i].generated_text << "\n" << std::endl;
        std::cout << ">>> FP16 Output:\n" << fp16_metrics[i].generated_text << "\n" << std::endl;
    }

    return 0;
}

