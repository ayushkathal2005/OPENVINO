#include "chat_session.hpp"

#include <iostream>
#include <sstream>

namespace ov_llm {

ChatSession::ChatSession(
    LLMEngine& engine,
    const std::string& system_prompt,
    const GenerationParameters& params
) : m_engine(engine), m_system_prompt(system_prompt), m_params(params) {
    reset();
}

void ChatSession::rebuild_history() {
    m_history.clear();
    if (!m_system_prompt.empty()) {
        ov::AnyMap sys_msg;
        sys_msg["role"] = std::string("system");
        sys_msg["content"] = m_system_prompt;
        m_history.push_back(sys_msg);
    }
    for (const auto& turn : m_turns) {
        ov::AnyMap msg;
        msg["role"] = turn.role;
        msg["content"] = turn.content;
        m_history.push_back(msg);
    }
}

void ChatSession::set_system_prompt(const std::string& system_prompt) {
    m_system_prompt = system_prompt;
    rebuild_history();
}

void ChatSession::reset() {
    m_turns.clear();
    m_history.clear();
    if (!m_system_prompt.empty()) {
        ov::AnyMap sys_msg;
        sys_msg["role"] = std::string("system");
        sys_msg["content"] = m_system_prompt;
        m_history.push_back(sys_msg);
    }
}

std::string ChatSession::send_message(
    const std::string& user_message,
    std::function<bool(const std::string&)> streamer
) {
    if (user_message.empty()) {
        std::cerr << "[ChatSession Warning] Empty user message ignored." << std::endl;
        return "";
    }

    if (!m_engine.is_loaded()) {
        std::cerr << "[ChatSession Error] LLMEngine is not loaded." << std::endl;
        return "";
    }

    // Add user message to conversation history
    ov::AnyMap user_msg;
    user_msg["role"] = std::string("user");
    user_msg["content"] = user_message;
    m_history.push_back(user_msg);

    // Execute generation with current session parameters
    std::string response = m_engine.generate(m_history, m_params, streamer);

    if (response.empty()) {
        std::cerr << "[ChatSession Warning] Generation produced empty response or failed." << std::endl;
        // Roll back uncommitted user message to keep conversation balanced
        m_history.pop_back();
        return "";
    }

    // Add assistant response to conversation history
    ov::AnyMap asst_msg;
    asst_msg["role"] = std::string("assistant");
    asst_msg["content"] = response;
    m_history.push_back(asst_msg);

    // Record verified turns
    m_turns.push_back({"user", user_message});
    m_turns.push_back({"assistant", response});

    return response;
}

std::string ChatSession::get_formatted_history() const {
    std::stringstream ss;
    if (!m_system_prompt.empty()) {
        ss << "[System]: " << m_system_prompt << "\n\n";
    }
    for (const auto& turn : m_turns) {
        if (turn.role == "user") {
            ss << "[User]: " << turn.content << "\n\n";
        } else if (turn.role == "assistant") {
            ss << "[Assistant]: " << turn.content << "\n\n";
        } else {
            ss << "[" << turn.role << "]: " << turn.content << "\n\n";
        }
    }
    return ss.str();
}

} // namespace ov_llm

