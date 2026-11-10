#pragma once

#include "llm_engine.hpp"
#include <string>
#include <vector>
#include <functional>
#include <openvino/genai/chat_history.hpp>

namespace ov_llm {

struct ChatTurn {
    std::string role;
    std::string content;
};

class ChatSession {
public:
    explicit ChatSession(
        LLMEngine& engine,
        const std::string& system_prompt = "",
        const GenerationParameters& params = GenerationParameters()
    );

    ~ChatSession() = default;

    // Send user message, execute generation, append turn to history, return assistant response
    std::string send_message(
        const std::string& user_message,
        std::function<bool(const std::string&)> streamer = nullptr
    );

    // Reset session, clearing all prior user/assistant turns. Re-applies system prompt if set.
    void reset();

    // System prompt configuration
    void set_system_prompt(const std::string& system_prompt);
    const std::string& get_system_prompt() const { return m_system_prompt; }

    // Generation parameters configuration
    void set_generation_params(const GenerationParameters& params) { m_params = params; }
    const GenerationParameters& get_generation_params() const { return m_params; }
    GenerationParameters& get_generation_params() { return m_params; }

    // Session inspection
    size_t turn_count() const { return m_turns.size(); }
    const std::vector<ChatTurn>& get_turns() const { return m_turns; }
    const ov::genai::ChatHistory& get_raw_history() const { return m_history; }
    std::string get_formatted_history() const;

private:
    LLMEngine& m_engine;
    std::string m_system_prompt;
    GenerationParameters m_params;
    ov::genai::ChatHistory m_history;
    std::vector<ChatTurn> m_turns;

    void rebuild_history();
};

} // namespace ov_llm

