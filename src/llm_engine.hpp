#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <openvino/genai/llm_pipeline.hpp>

namespace ov_llm {

struct GenerationParameters {
    size_t max_new_tokens = 128;
    float temperature = 0.7f;
    float top_p = 0.9f;
    bool do_sample = true;
};

class LLMEngine {
public:
    LLMEngine() = default;
    ~LLMEngine() = default;

    // Initialize pipeline with local model directory and device target
    bool load_model(const std::string& model_path, const std::string& device = "CPU");

    // Generate response for a single prompt
    std::string generate(
        const std::string& prompt,
        const GenerationParameters& params = GenerationParameters(),
        std::function<bool(const std::string&)> streamer = nullptr
    );

    // Query engine status
    bool is_loaded() const { return m_pipeline != nullptr; }
    const std::string& get_model_path() const { return m_model_path; }
    const std::string& get_device() const { return m_device; }

    // Static validation helpers
    static bool validate_model_path(const std::string& model_path, std::string& error_msg);
    static bool validate_device(const std::string& device, std::string& error_msg);

private:
    std::string m_model_path;
    std::string m_device;
    std::unique_ptr<ov::genai::LLMPipeline> m_pipeline;
};

} // namespace ov_llm
