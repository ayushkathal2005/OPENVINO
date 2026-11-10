#include <iostream>
#include <string>
#include <vector>
#include "llm_engine.hpp"

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options]\n\n"
              << "Options:\n"
              << "  --model-path <path>   Path to local OpenVINO IR model directory (default: models/tinyllama-1.1b-chat-fp16)\n"
              << "  --device <device>     Target execution device [CPU, GPU.0, GPU.1] (default: CPU)\n"
              << "  --prompt <string>     Single prompt to process; if omitted, enters interactive mode\n"
              << "  --max-tokens <num>    Maximum new tokens to generate (default: 128)\n"
              << "  --temperature <val>   Sampling temperature (default: 0.7)\n"
              << "  --help, -h            Show this help message\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    std::string model_path = "models/tinyllama-1.1b-chat-fp16";
    std::string device = "CPU";
    std::string single_prompt = "";
    ov_llm::GenerationParameters params;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--model-path" && i + 1 < argc) {
            model_path = argv[++i];
        } else if (arg == "--device" && i + 1 < argc) {
            device = argv[++i];
        } else if (arg == "--prompt" && i + 1 < argc) {
            single_prompt = argv[++i];
        } else if (arg == "--max-tokens" && i + 1 < argc) {
            params.max_new_tokens = std::stoul(argv[++i]);
        } else if (arg == "--temperature" && i + 1 < argc) {
            params.temperature = std::stof(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    std::cout << "==================================================" << std::endl;
    std::cout << "  Local LLM Inference Workbench (OpenVINO GenAI)" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Model Path: " << model_path << std::endl;
    std::cout << "Device:     " << device << std::endl;
    std::cout << "Max Tokens: " << params.max_new_tokens << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;

    ov_llm::LLMEngine engine;
    if (!engine.load_model(model_path, device)) {
        std::cerr << "[FATAL] Failed to initialize model. Exiting." << std::endl;
        return 1;
    }

    // Streamer callback to print tokens in real time
    auto token_streamer = [](const std::string& token) {
        std::cout << token << std::flush;
        return true;
    };

    if (!single_prompt.empty()) {
        std::cout << "\n[User]: " << single_prompt << "\n[Assistant]: ";
        std::string response = engine.generate(single_prompt, params, token_streamer);
        std::cout << "\n" << std::endl;
        return 0;
    }

    // Interactive Loop Mode
    std::cout << "\nEntering interactive prompt loop. Type 'exit' or 'quit' to end.\n" << std::endl;

    std::string input_line;
    while (true) {
        std::cout << "[User]: ";
        if (!std::getline(std::cin, input_line)) {
            break;
        }

        // Trim whitespace check
        size_t first = input_line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            std::cout << "[Notice] Empty prompt received. Please enter a valid prompt.\n" << std::endl;
            continue;
        }

        std::string trimmed = input_line.substr(first, input_line.find_last_not_of(" \t\r\n") - first + 1);
        if (trimmed == "exit" || trimmed == "quit") {
            std::cout << "Exiting application. Goodbye." << std::endl;
            break;
        }

        std::cout << "[Assistant]: ";
        std::string response = engine.generate(trimmed, params, token_streamer);
        std::cout << "\n" << std::endl;
    }

    return 0;
}
