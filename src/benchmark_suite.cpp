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
#include <cmath>
#include <filesystem>

#include <windows.h>
#include <psapi.h>

#include <openvino/genai/llm_pipeline.hpp>
#include <openvino/genai/perf_metrics.hpp>
#include "llm_engine.hpp"

namespace fs = std::filesystem;

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

struct IterationRecord {
    std::string config_name;
    std::string model_path;
    std::string device;
    std::string prompt_id;
    std::string category;
    size_t repetition;
    size_t input_tokens;
    size_t output_tokens;
    double ttft_ms;
    double tpot_ms;
    double throughput_tps;
    double total_infer_ms;
    double peak_ram_mb;
};

struct AggregatedStats {
    double mean = 0.0;
    double stddev = 0.0;
};

AggregatedStats calculate_stats(const std::vector<double>& values) {
    AggregatedStats s;
    if (values.empty()) return s;
    double sum = 0.0;
    for (double v : values) sum += v;
    s.mean = sum / values.size();

    if (values.size() > 1) {
        double sq_sum = 0.0;
        for (double v : values) {
            sq_sum += (v - s.mean) * (v - s.mean);
        }
        s.stddev = std::sqrt(sq_sum / (values.size() - 1));
    }
    return s;
}

std::vector<PromptItem> get_benchmark_prompts() {
    return {
        {"fact_01", "factual_explanation", "Explain the concept of Key-Value (KV) caching in large language models and why it optimizes autoregressive generation."},
        {"summ_01", "summarization", "Summarize the primary benefits and potential accuracy trade-offs of INT4 weight quantization for edge AI inference."},
        {"instruct_01", "instruction_following", "List exactly three advantages of deploying LLMs locally using OpenVINO GenAI C++ API instead of remote cloud APIs. Format as a numbered list."},
        {"reason_01", "simple_reasoning", "If a language model processes 20 tokens per second, how many seconds will it take to generate a 120-token response? Show your calculation."}
    };
}

void run_benchmark_configuration(
    const std::string& config_name,
    const std::string& model_path,
    const std::string& device,
    size_t num_repetitions,
    std::vector<IterationRecord>& all_records
) {
    std::cout << "\n======================================================================" << std::endl;
    std::cout << " CONFIGURATION: " << config_name << " [" << device << "]" << std::endl;
    std::cout << " Model Path:    " << model_path << std::endl;
    std::cout << " Repetitions:   " << num_repetitions << " per prompt" << std::endl;
    std::cout << "======================================================================" << std::endl;

    auto t_load_start = std::chrono::high_resolution_clock::now();
    ov::genai::LLMPipeline pipeline(model_path, device);
    auto t_load_end = std::chrono::high_resolution_clock::now();
    double load_time_ms = std::chrono::duration<double, std::milli>(t_load_end - t_load_start).count();

    std::cout << "Cold Load Duration: " << std::fixed << std::setprecision(2) << load_time_ms << " ms" << std::endl;
    std::cout << "RAM After Load:     " << bytes_to_mb(get_process_working_set_bytes()) << " MB" << std::endl;

    // Warm-up run
    std::cout << "Executing pipeline warm-up run..." << std::flush;
    ov::genai::GenerationConfig warmup_cfg;
    warmup_cfg.max_new_tokens = 16;
    warmup_cfg.do_sample = false;
    pipeline.generate("Pipeline compilation warm-up prompt.", warmup_cfg);
    std::cout << " Done.\n" << std::endl;

    auto prompts = get_benchmark_prompts();

    for (const auto& item : prompts) {
        std::cout << "--> Prompt [" << item.id << "] (" << item.category << "):" << std::endl;

        for (size_t rep = 1; rep <= num_repetitions; ++rep) {
            std::cout << "    Rep " << rep << "/" << num_repetitions << "... " << std::flush;

            ov::genai::GenerationConfig config;
            config.max_new_tokens = 64;
            config.do_sample = false;
            config.temperature = 0.0f; // deterministic greedy

            auto t_start = std::chrono::high_resolution_clock::now();
            ov::genai::DecodedResults res = pipeline.generate(item.prompt, config);
            auto t_end = std::chrono::high_resolution_clock::now();

            IterationRecord r;
            r.config_name = config_name;
            r.model_path = model_path;
            r.device = device;
            r.prompt_id = item.id;
            r.category = item.category;
            r.repetition = rep;
            r.input_tokens = res.perf_metrics.get_num_input_tokens();
            r.output_tokens = res.perf_metrics.get_num_generated_tokens();
            r.ttft_ms = res.perf_metrics.get_ttft().mean;
            r.tpot_ms = res.perf_metrics.get_tpot().mean;
            r.throughput_tps = res.perf_metrics.get_throughput().mean;
            r.total_infer_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
            r.peak_ram_mb = bytes_to_mb(get_process_working_set_bytes());

            all_records.push_back(r);

            std::cout << "Done (TTFT: " << std::setprecision(1) << r.ttft_ms << "ms, TPOT: "
                      << r.tpot_ms << "ms/tok, TPS: " << r.throughput_tps << ", RAM: " << r.peak_ram_mb << "MB)" << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    size_t repetitions = 5;
    if (argc > 1) {
        repetitions = std::stoul(argv[1]);
    }

    std::cout << "======================================================================" << std::endl;
    std::cout << "      Phase 07: Multi-Run Benchmarking & Resource Profiling Suite     " << std::endl;
    std::cout << "======================================================================" << std::endl;
    std::cout << "Repetitions per Prompt: " << repetitions << std::endl;

    std::vector<IterationRecord> records;

    // 1. Run CPU INT4 (Primary Workload)
    run_benchmark_configuration("CPU_INT4", "models/tinyllama-1.1b-chat-int4", "CPU", repetitions, records);

    // 2. Run CPU FP16 (Baseline Workload)
    run_benchmark_configuration("CPU_FP16", "models/tinyllama-1.1b-chat-fp16", "CPU", repetitions, records);

    // 3. Run GPU.0 INT4 (iGPU Accelerated Workload)
    run_benchmark_configuration("GPU.0_INT4", "models/tinyllama-1.1b-chat-int4", "GPU.0", repetitions, records);

    // Ensure output directory exists
    fs::create_directories("benchmarks/results");
    std::string csv_path = "benchmarks/results/benchmark_results.csv";
    std::ofstream csv(csv_path);

    if (csv.is_open()) {
        csv << "config,device,model_path,prompt_id,category,repetition,input_tokens,output_tokens,ttft_ms,tpot_ms,throughput_tps,total_infer_ms,peak_ram_mb\n";
        for (const auto& r : records) {
            csv << r.config_name << ","
                << r.device << ","
                << r.model_path << ","
                << r.prompt_id << ","
                << r.category << ","
                << r.repetition << ","
                << r.input_tokens << ","
                << r.output_tokens << ","
                << std::fixed << std::setprecision(2) << r.ttft_ms << ","
                << r.tpot_ms << ","
                << r.throughput_tps << ","
                << r.total_infer_ms << ","
                << r.peak_ram_mb << "\n";
        }
        csv.close();
        std::cout << "\n[OK] Raw benchmark iterations written to: " << csv_path << " (" << records.size() << " rows)" << std::endl;
    } else {
        std::cerr << "[ERROR] Could not open CSV file for writing: " << csv_path << std::endl;
    }

    // Statistical Summary Display
    std::cout << "\n========================================================================================================================" << std::endl;
    std::cout << "                               Statistical Performance Summary Table (Mean +/- StdDev)                                  " << std::endl;
    std::cout << "========================================================================================================================" << std::endl;
    std::cout << std::left
              << std::setw(14) << "Configuration"
              << std::setw(12) << "Prompt ID"
              << std::setw(8)  << "N"
              << std::setw(22) << "TTFT (ms)"
              << std::setw(22) << "TPOT (ms/tok)"
              << std::setw(22) << "Throughput (t/s)"
              << std::setw(18) << "RAM Peak (MB)"
              << std::endl;
    std::cout << "------------------------------------------------------------------------------------------------------------------------" << std::endl;

    std::vector<std::string> configs = {"CPU_INT4", "CPU_FP16", "GPU.0_INT4"};
    auto prompts = get_benchmark_prompts();

    for (const auto& cfg : configs) {
        for (const auto& p : prompts) {
            std::vector<double> ttfts, tpots, tpss, rams;
            for (const auto& r : records) {
                if (r.config_name == cfg && r.prompt_id == p.id) {
                    ttfts.push_back(r.ttft_ms);
                    tpots.push_back(r.tpot_ms);
                    tpss.push_back(r.throughput_tps);
                    rams.push_back(r.peak_ram_mb);
                }
            }

            auto s_ttft = calculate_stats(ttfts);
            auto s_tpot = calculate_stats(tpots);
            auto s_tps  = calculate_stats(tpss);
            auto s_ram  = calculate_stats(rams);

            std::stringstream ss_ttft, ss_tpot, ss_tps, ss_ram;
            ss_ttft << std::fixed << std::setprecision(1) << s_ttft.mean << " +/- " << s_ttft.stddev;
            ss_tpot << std::fixed << std::setprecision(1) << s_tpot.mean << " +/- " << s_tpot.stddev;
            ss_tps  << std::fixed << std::setprecision(1) << s_tps.mean  << " +/- " << s_tps.stddev;
            ss_ram  << std::fixed << std::setprecision(1) << s_ram.mean;

            std::cout << std::left
                      << std::setw(14) << cfg
                      << std::setw(12) << p.id
                      << std::setw(8)  << ttfts.size()
                      << std::setw(22) << ss_ttft.str()
                      << std::setw(22) << ss_tpot.str()
                      << std::setw(22) << ss_tps.str()
                      << std::setw(18) << ss_ram.str()
                      << std::endl;
        }
        std::cout << "------------------------------------------------------------------------------------------------------------------------" << std::endl;
    }

    std::cout << "========================================================================================================================\n" << std::endl;

    return 0;
}

