#include "llm_engine.hpp"

#include <iostream>
#include <filesystem>
#include <sstream>
#include <openvino/openvino.hpp>

namespace fs = std::filesystem;

namespace ov_llm {

bool LLMEngine::validate_model_path(const std::string& model_path, std::string& error_msg) {
    if (model_path.empty()) {
        error_msg = "Model path cannot be empty.";
        return false;
    }

    fs::path p(model_path);
    if (!fs::exists(p)) {
        error_msg = "Directory does not exist: " + model_path;
        return false;
    }

    if (!fs::is_directory(p)) {
        error_msg = "Path is not a directory: " + model_path;
        return false;
    }

    fs::path xml_path = p / "openvino_model.xml";
    fs::path bin_path = p / "openvino_model.bin";

    if (!fs::exists(xml_path)) {
        error_msg = "Missing model topology file: " + xml_path.string();
        return false;
    }

    if (!fs::exists(bin_path)) {
        error_msg = "Missing model weights file: " + bin_path.string();
        return false;
    }

    return true;
}

bool LLMEngine::validate_device(const std::string& device, std::string& error_msg) {
    if (device.empty()) {
        error_msg = "Device string cannot be empty.";
        return false;
    }

    try {
        ov::Core core;
        std::vector<std::string> available = core.get_available_devices();
        bool found = false;
        std::stringstream ss;

        for (size_t i = 0; i < available.size(); ++i) {
            ss << available[i];
            if (i + 1 < available.size()) ss << ", ";

            // Match exact or prefix (e.g. GPU matches GPU.0)
            if (available[i] == device || (device == "GPU" && available[i].rfind("GPU", 0) == 0)) {
                found = true;
            }
        }

        if (!found) {
            error_msg = "Requested device '" + device + "' is not available. Available devices: [" + ss.str() + "]";
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        error_msg = std::string("Device validation exception: ") + e.what();
        return false;
    }
}

bool LLMEngine::load_model(const std::string& model_path, const std::string& device) {
    std::string err;
    if (!validate_model_path(model_path, err)) {
        std::cerr << "[LLMEngine Error] " << err << std::endl;
        return false;
    }

    if (!validate_device(device, err)) {
        std::cerr << "[LLMEngine Error] " << err << std::endl;
        return false;
    }

    try {
        std::cout << "[LLMEngine] Initializing ov::genai::LLMPipeline on device: " << device << "..." << std::endl;
        m_pipeline = std::make_unique<ov::genai::LLMPipeline>(model_path, device);
        m_model_path = model_path;
        m_device = device;
        std::cout << "[LLMEngine] Model successfully loaded into memory." << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[LLMEngine Error] Failed to initialize LLMPipeline: " << e.what() << std::endl;
        m_pipeline.reset();
        return false;
    }
}

std::string LLMEngine::generate(
    const std::string& prompt,
    const GenerationParameters& params,
    std::function<bool(const std::string&)> streamer
) {
    if (!is_loaded()) {
        std::cerr << "[LLMEngine Error] No model is loaded." << std::endl;
        return "";
    }

    if (prompt.empty()) {
        std::cerr << "[LLMEngine Warning] Prompt is empty." << std::endl;
        return "";
    }

    try {
        ov::genai::GenerationConfig config;
        config.max_new_tokens = params.max_new_tokens;
        config.temperature = params.temperature;
        config.top_p = params.top_p;
        config.do_sample = params.do_sample;

        if (streamer != nullptr) {
            auto ov_streamer = [&streamer](std::string token) {
                bool keep_running = streamer(token);
                return keep_running ? ov::genai::StreamingStatus::RUNNING : ov::genai::StreamingStatus::STOP;
            };
            return m_pipeline->generate(prompt, config, ov_streamer);
        } else {
            return m_pipeline->generate(prompt, config);
        }
    } catch (const std::exception& e) {
        std::cerr << "[LLMEngine Error] Generation failed: " << e.what() << std::endl;
        return "";
    }
}

} // namespace ov_llm
