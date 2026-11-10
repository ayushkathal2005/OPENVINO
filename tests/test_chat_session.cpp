#include <iostream>
#include <cassert>
#include <string>
#include <algorithm>
#include "llm_engine.hpp"
#include "chat_session.hpp"

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [ASSERTION FAILED] " << msg << " (" << #cond << ") at line " << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

bool test_parameter_validation() {
    std::cout << "[TEST 1] Generation parameter validation..." << std::endl;
    std::string err;

    // Valid parameters
    ov_llm::GenerationParameters valid_params;
    valid_params.max_new_tokens = 64;
    valid_params.temperature = 0.7f;
    valid_params.top_p = 0.9f;
    valid_params.repetition_penalty = 1.1f;
    TEST_ASSERT(ov_llm::LLMEngine::validate_params(valid_params, err), "Valid params should pass");

    // Invalid max_new_tokens = 0
    ov_llm::GenerationParameters zero_tokens = valid_params;
    zero_tokens.max_new_tokens = 0;
    TEST_ASSERT(!ov_llm::LLMEngine::validate_params(zero_tokens, err), "Zero max_tokens should fail");

    // Invalid temperature < 0
    ov_llm::GenerationParameters neg_temp = valid_params;
    neg_temp.temperature = -0.5f;
    TEST_ASSERT(!ov_llm::LLMEngine::validate_params(neg_temp, err), "Negative temperature should fail");

    // Invalid top_p <= 0
    ov_llm::GenerationParameters zero_topp = valid_params;
    zero_topp.top_p = 0.0f;
    TEST_ASSERT(!ov_llm::LLMEngine::validate_params(zero_topp, err), "Zero top_p should fail");

    // Invalid top_p > 1.0
    ov_llm::GenerationParameters excessive_topp = valid_params;
    excessive_topp.top_p = 1.5f;
    TEST_ASSERT(!ov_llm::LLMEngine::validate_params(excessive_topp, err), "top_p > 1.0 should fail");

    // Invalid repetition penalty <= 0
    ov_llm::GenerationParameters bad_rep = valid_params;
    bad_rep.repetition_penalty = 0.0f;
    TEST_ASSERT(!ov_llm::LLMEngine::validate_params(bad_rep, err), "Zero repetition penalty should fail");

    std::cout << "  -> PASS" << std::endl;
    return true;
}

bool test_chat_template_application(ov_llm::LLMEngine& engine) {
    std::cout << "[TEST 2] Chat template format verification..." << std::endl;
    ov::genai::ChatHistory history;
    
    ov::AnyMap sys_msg;
    sys_msg["role"] = std::string("system");
    sys_msg["content"] = std::string("You are a helpful assistant.");
    history.push_back(sys_msg);

    ov::AnyMap user_msg;
    user_msg["role"] = std::string("user");
    user_msg["content"] = std::string("Hello world.");
    history.push_back(user_msg);

    std::string formatted = engine.apply_chat_template(history, true);
    std::cout << "  Formatted template preview (first 100 chars): \"" 
              << formatted.substr(0, std::min<size_t>(100, formatted.size())) << "...\"" << std::endl;

    // TinyLlama template uses <|system|>, <|user|>, <|assistant|>
    TEST_ASSERT(formatted.find("<|user|>") != std::string::npos, "Template should contain <|user|>");
    TEST_ASSERT(formatted.find("Hello world.") != std::string::npos, "Template should contain prompt text");
    TEST_ASSERT(formatted.find("<|assistant|>") != std::string::npos, "Template should contain <|assistant|>");

    std::cout << "  -> PASS" << std::endl;
    return true;
}

bool test_multiturn_conversation_and_reset(ov_llm::LLMEngine& engine) {
    std::cout << "[TEST 3] Multi-turn conversation context retention and reset isolation..." << std::endl;

    ov_llm::GenerationParameters params;
    params.max_new_tokens = 64;
    params.temperature = 0.0f; // deterministic greedy
    params.do_sample = false;

    ov_llm::ChatSession session(engine, "You are a concise AI assistant.", params);
    TEST_ASSERT(session.turn_count() == 0, "Initial session must have 0 turns");

    // Turn 1: Establish knowledge
    std::string q1 = "My secret codeword is BLUEBERRY. What is my secret codeword?";
    std::cout << "  Turn 1 Prompt: " << q1 << std::endl;
    std::string r1 = session.send_message(q1);
    std::cout << "  Turn 1 Response: " << r1 << std::endl;
    TEST_ASSERT(!r1.empty(), "Turn 1 response must not be empty");
    TEST_ASSERT(session.turn_count() == 2, "Session must have 2 turns after Turn 1");

    // Turn 2: Recall knowledge established in Turn 1
    std::string q2 = "What was the secret codeword I just told you? Reply with only the single codeword.";
    std::cout << "  Turn 2 Prompt: " << q2 << std::endl;
    std::string r2 = session.send_message(q2);
    std::cout << "  Turn 2 Response: " << r2 << std::endl;
    TEST_ASSERT(!r2.empty(), "Turn 2 response must not be empty");
    TEST_ASSERT(session.turn_count() == 4, "Session must have 4 turns after Turn 2");

    // Convert response to upper case for case-insensitive check
    std::string r2_upper = r2;
    std::transform(r2_upper.begin(), r2_upper.end(), r2_upper.begin(), ::toupper);
    TEST_ASSERT(r2_upper.find("BLUEBERRY") != std::string::npos, 
                "Turn 2 must recall 'BLUEBERRY' from Turn 1 context");

    // Turn 3: Further continuation
    std::string q3 = "What kind of fruit is that?";
    std::cout << "  Turn 3 Prompt: " << q3 << std::endl;
    std::string r3 = session.send_message(q3);
    std::cout << "  Turn 3 Response: " << r3 << std::endl;
    TEST_ASSERT(!r3.empty(), "Turn 3 response must not be empty");
    TEST_ASSERT(session.turn_count() == 6, "Session must have 6 turns after Turn 3");

    // Test Session Reset
    std::cout << "  Resetting session..." << std::endl;
    session.reset();
    TEST_ASSERT(session.turn_count() == 0, "Reset session must have 0 turns");
    TEST_ASSERT(session.get_turns().empty(), "Turns list must be empty after reset");

    // Post-Reset Turn: Verify context isolation (no leakage of 'BLUEBERRY')
    std::string q_isolated = "What was my secret codeword?";
    std::cout << "  Post-Reset Prompt: " << q_isolated << std::endl;
    std::string r_isolated = session.send_message(q_isolated);
    std::cout << "  Post-Reset Response: " << r_isolated << std::endl;
    TEST_ASSERT(!r_isolated.empty(), "Post-reset response must not be empty");

    std::string r_iso_upper = r_isolated;
    std::transform(r_iso_upper.begin(), r_iso_upper.end(), r_iso_upper.begin(), ::toupper);
    TEST_ASSERT(r_iso_upper.find("BLUEBERRY") == std::string::npos,
                "Reset session must NOT leak 'BLUEBERRY' from previous session");

    std::cout << "  -> PASS" << std::endl;
    return true;
}

bool test_deterministic_greedy_reproducibility(ov_llm::LLMEngine& engine) {
    std::cout << "[TEST 4] Deterministic greedy decoding reproducibility..." << std::endl;

    ov_llm::GenerationParameters det_params;
    det_params.max_new_tokens = 32;
    det_params.temperature = 0.0f;
    det_params.do_sample = false;

    std::string prompt = "Name three planets in our solar system:";

    ov_llm::ChatSession session1(engine, "", det_params);
    std::string run1 = session1.send_message(prompt);

    ov_llm::ChatSession session2(engine, "", det_params);
    std::string run2 = session2.send_message(prompt);

    std::cout << "  Run 1: " << run1 << std::endl;
    std::cout << "  Run 2: " << run2 << std::endl;

    TEST_ASSERT(!run1.empty(), "Run 1 must not be empty");
    TEST_ASSERT(run1 == run2, "Deterministic runs with greedy decoding must be identical");

    std::cout << "  -> PASS" << std::endl;
    return true;
}

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "  Running Phase 04 Chat Sessions & Controls Tests" << std::endl;
    std::cout << "==================================================" << std::endl;

    if (!test_parameter_validation()) {
        std::cerr << "FAIL: Parameter validation tests failed." << std::endl;
        return 1;
    }

    // Load model for functional tests
    const std::string model_path = "models/tinyllama-1.1b-chat-int4";
    ov_llm::LLMEngine engine;
    if (!engine.load_model(model_path, "CPU")) {
        std::cerr << "FAIL: Could not load test model at: " << model_path << std::endl;
        return 1;
    }

    if (!test_chat_template_application(engine)) {
        std::cerr << "FAIL: Chat template tests failed." << std::endl;
        return 1;
    }

    if (!test_multiturn_conversation_and_reset(engine)) {
        std::cerr << "FAIL: Multi-turn conversation and reset tests failed." << std::endl;
        return 1;
    }

    if (!test_deterministic_greedy_reproducibility(engine)) {
        std::cerr << "FAIL: Deterministic reproducibility tests failed." << std::endl;
        return 1;
    }

    std::cout << "\n==================================================" << std::endl;
    std::cout << "  Phase 04 Test Summary: All Tests Passed" << std::endl;
    std::cout << "==================================================" << std::endl;

    return 0;
}

