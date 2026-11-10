#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include "llm_engine.hpp"
#include "chat_session.hpp"

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options]\n\n"
              << "Options:\n"
              << "  --model-path <path>      Path to local OpenVINO IR model directory (default: models/tinyllama-1.1b-chat-fp16)\n"
              << "  --device <device>        Target execution device [CPU, GPU.0, GPU.1] (default: CPU)\n"
              << "  --prompt <string>        Single prompt to process; if omitted, enters interactive chat mode\n"
              << "  --system-prompt <str>    System prompt instruction for the session\n"
              << "  --max-tokens <num>       Maximum new tokens to generate (default: 128)\n"
              << "  --temperature <val>      Sampling temperature (default: 0.7)\n"
              << "  --top-p <val>            Nucleus sampling top_p (default: 0.9)\n"
              << "  --rep-penalty <val>      Repetition penalty (default: 1.0)\n"
              << "  --deterministic          Disable sampling and use greedy decoding (temperature=0.0)\n"
              << "  --help, -h               Show this help message\n\n"
              << "Interactive Commands:\n"
              << "  /reset, reset            Clear conversation history and restart session\n"
              << "  /history                 Display recorded conversation history\n"
              << "  /system <text>           Set new system prompt\n"
              << "  /temp <val>              Adjust temperature\n"
              << "  /topp <val>              Adjust top-p\n"
              << "  /tokens <val>            Adjust max new tokens\n"
              << "  /help                    Display this command list\n"
              << "  exit, quit               Terminate application\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    std::string model_path = "models/tinyllama-1.1b-chat-fp16";
    std::string device = "CPU";
    std::string single_prompt = "";
    std::string system_prompt = "You are a helpful, respectful, and honest local AI assistant.";
    ov_llm::GenerationParameters params;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--model-path" && i + 1 < argc) {
            model_path = argv[++i];
        } else if (arg == "--device" && i + 1 < argc) {
            device = argv[++i];
        } else if (arg == "--prompt" && i + 1 < argc) {
            single_prompt = argv[++i];
        } else if (arg == "--system-prompt" && i + 1 < argc) {
            system_prompt = argv[++i];
        } else if (arg == "--max-tokens" && i + 1 < argc) {
            params.max_new_tokens = std::stoul(argv[++i]);
        } else if (arg == "--temperature" && i + 1 < argc) {
            params.temperature = std::stof(argv[++i]);
        } else if (arg == "--top-p" && i + 1 < argc) {
            params.top_p = std::stof(argv[++i]);
        } else if (arg == "--rep-penalty" && i + 1 < argc) {
            params.repetition_penalty = std::stof(argv[++i]);
        } else if (arg == "--deterministic") {
            params.do_sample = false;
            params.temperature = 0.0f;
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    std::string err;
    if (!ov_llm::LLMEngine::validate_params(params, err)) {
        std::cerr << "[FATAL] Invalid parameters: " << err << std::endl;
        return 1;
    }

    std::cout << "==================================================" << std::endl;
    std::cout << "  Local LLM Inference Workbench (OpenVINO GenAI)" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Model Path:    " << model_path << std::endl;
    std::cout << "Device:        " << device << std::endl;
    std::cout << "Max Tokens:    " << params.max_new_tokens << std::endl;
    std::cout << "Temperature:   " << params.temperature << (params.do_sample ? " (Sampling)" : " (Greedy)") << std::endl;
    std::cout << "Top-P:         " << params.top_p << std::endl;
    std::cout << "Rep. Penalty:  " << params.repetition_penalty << std::endl;
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

    // Single prompt mode
    if (!single_prompt.empty()) {
        std::cout << "\n[User]: " << single_prompt << "\n[Assistant]: ";
        std::string response = engine.generate(single_prompt, params, token_streamer);
        std::cout << "\n" << std::endl;
        return 0;
    }

    // Stateful Chat Session
    ov_llm::ChatSession session(engine, system_prompt, params);

    std::cout << "\nEntering interactive chat session." << std::endl;
    std::cout << "Commands: '/reset' to clear history, '/history' to view turns, 'exit' to quit.\n" << std::endl;

    std::string input_line;
    while (true) {
        std::cout << "[User]: ";
        if (!std::getline(std::cin, input_line)) {
            break;
        }

        // Trim whitespace
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

        if (trimmed == "/reset" || trimmed == "reset") {
            session.reset();
            std::cout << "[System] Conversation history cleared. Fresh session started.\n" << std::endl;
            continue;
        }

        if (trimmed == "/history") {
            std::cout << "\n--- Conversation History (" << session.turn_count() << " turns) ---\n"
                      << session.get_formatted_history()
                      << "----------------------------------------\n" << std::endl;
            continue;
        }

        if (trimmed == "/help") {
            print_usage(argv[0]);
            continue;
        }

        if (trimmed.rfind("/system ", 0) == 0) {
            std::string sys = trimmed.substr(8);
            session.set_system_prompt(sys);
            std::cout << "[System] System prompt updated to: \"" << sys << "\"\n" << std::endl;
            continue;
        }

        if (trimmed.rfind("/temp ", 0) == 0) {
            float t = std::stof(trimmed.substr(6));
            auto& p = session.get_generation_params();
            p.temperature = t;
            p.do_sample = (t > 0.0f);
            std::cout << "[System] Temperature set to " << t << " (" << (p.do_sample ? "Sampling" : "Greedy") << ")\n" << std::endl;
            continue;
        }

        if (trimmed.rfind("/topp ", 0) == 0) {
            float tp = std::stof(trimmed.substr(6));
            session.get_generation_params().top_p = tp;
            std::cout << "[System] Top-p set to " << tp << "\n" << std::endl;
            continue;
        }

        if (trimmed.rfind("/tokens ", 0) == 0) {
            size_t tk = std::stoul(trimmed.substr(8));
            session.get_generation_params().max_new_tokens = tk;
            std::cout << "[System] Max tokens set to " << tk << "\n" << std::endl;
            continue;
        }

        std::cout << "[Assistant]: ";
        std::string response = session.send_message(trimmed, token_streamer);
        std::cout << "\n" << std::endl;
    }

    return 0;
}
