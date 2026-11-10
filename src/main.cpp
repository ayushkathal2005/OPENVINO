#include <iostream>
#include <vector>
#include <string>
#include <exception>

// OpenVINO Core Runtime
#include <openvino/openvino.hpp>

// OpenVINO GenAI C++ API
#include <openvino/genai/llm_pipeline.hpp>

int main(int argc, char* argv[]) {
    std::cout << "==================================================" << std::endl;
    std::cout << "  OpenVINO GenAI C++ Environment Verification" << std::endl;
    std::cout << "==================================================" << std::endl;

    try {
        // 1. Initialize OpenVINO Core Runtime
        ov::Core core;
        ov::Version ov_version = ov::get_openvino_version();
        std::cout << "[INFO] OpenVINO Runtime Build: " << ov_version.description << std::endl;
        std::cout << "[INFO] OpenVINO API Version:   " << ov_version.buildNumber << std::endl;

        // 2. Query available devices
        std::vector<std::string> devices = core.get_available_devices();
        std::cout << "\n[INFO] Enumerating Available Devices (" << devices.size() << " detected):" << std::endl;

        bool cpu_available = false;
        bool gpu_available = false;
        bool npu_available = false;

        for (const auto& device : devices) {
            std::cout << "  + Device ID: " << device << std::endl;
            try {
                std::string full_name = core.get_property(device, ov::device::full_name);
                std::cout << "    - Full Name: " << full_name << std::endl;
            } catch (const std::exception& e) {
                std::cout << "    - Full Name: (query failed: " << e.what() << ")" << std::endl;
            }

            if (device.find("CPU") != std::string::npos) cpu_available = true;
            if (device.find("GPU") != std::string::npos) gpu_available = true;
            if (device.find("NPU") != std::string::npos) npu_available = true;
        }

        std::cout << "\n[DEVICE SUMMARY]" << std::endl;
        std::cout << "  CPU Acceleration: " << (cpu_available ? "READY (Baseline Target)" : "NOT FOUND") << std::endl;
        std::cout << "  GPU Acceleration: " << (gpu_available ? "READY (Candidate Target)" : "NOT AVAILABLE") << std::endl;
        std::cout << "  NPU Acceleration: " << (npu_available ? "READY" : "NOT AVAILABLE") << std::endl;

        // 3. Exercise OpenVINO GenAI Types and Config to prove library linkage
        std::cout << "\n[INFO] Validating OpenVINO GenAI Linkage..." << std::endl;
        ov::genai::GenerationConfig gen_config;
        gen_config.max_new_tokens = 64;
        gen_config.temperature = 0.7f;
        gen_config.top_p = 0.9f;

        std::cout << "  + GenAI GenerationConfig instantiated successfully." << std::endl;
        std::cout << "    - max_new_tokens: " << gen_config.max_new_tokens << std::endl;
        std::cout << "    - temperature:    " << gen_config.temperature << std::endl;
        std::cout << "    - top_p:          " << gen_config.top_p << std::endl;

        std::cout << "\n[SUCCESS] OpenVINO Core and GenAI C++ toolchain verified successfully." << std::endl;
        return 0;

    } catch (const std::exception& ex) {
        std::cerr << "\n[ERROR] Exception caught during environment verification: " << ex.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "\n[FATAL] Unknown exception caught during environment verification." << std::endl;
        return 2;
    }
}

