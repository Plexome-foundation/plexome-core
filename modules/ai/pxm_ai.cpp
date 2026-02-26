/**
 * Plexome Core v2.0 - AI Engine (Resilient Version)
 * Author: Georgii
 */

#include "../../include/module_interface.h"
#include "llama.h"
#include <iostream>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

struct AiState {
    llama_model* model = nullptr;
    llama_context* ctx = nullptr;
    bool model_ready = false;
};

static AiState g_state;

extern "C" {
    PXM_API PxmModuleInfo pxm_get_info() {
        return { "Plexome AI Engine", "2.1.1-resilient", "AI unit with lazy-loading and recovery support." };
    }

    PXM_API PxmStatus pxm_init(const PxmConfig* config) {
        if (!config) return PxmStatus::ERROR_INIT_FAILED;

        std::cout << "[AI] Initializing AI backend (Resilient Mode)..." << std::endl;
        llama_backend_init();

        // Проверяем наличие файла, но НЕ возвращаем ошибку инициализации, если его нет
        if (!config->model_path || !fs::exists(config->model_path)) {
            std::cout << "[AI] WARNING: Model file not found. AI features will be limited until download finishes." << std::endl;
            g_state.model_ready = false;
            return PxmStatus::OK; // Возвращаем OK, чтобы не «уронить» Хост
        }

        // Если файл есть — грузим как обычно
        auto mparams = llama_model_default_params();
        if (config->tier >= PerformanceTier::TITAN) {
            mparams.n_gpu_layers = 99;
        }

        g_state.model = llama_load_model_from_file(config->model_path, mparams);
        if (g_state.model) {
            auto cparams = llama_context_default_params();
            cparams.n_ctx = 2048;
            g_state.ctx = llama_new_context_with_model(g_state.model, cparams);
            g_state.model_ready = (g_state.ctx != nullptr);
        }

        if (g_state.model_ready) {
            std::cout << "[AI] Model loaded and ready for inference." << std::endl;
        }

        return PxmStatus::OK;
    }

    PXM_API const char* pxm_generate(const char* prompt) {
        if (!g_state.model_ready) {
            // Вместо падения — вежливый ответ системы
            return "[System] AI engine is offline: Model weights missing. Please wait for 'pxm_updater' to sync data.";
        }
        
        // Здесь завтра будет реальная логика llama_decode
        return "Plexome AI is processing your request..."; 
    }

    PXM_API void pxm_shutdown() {
        if (g_state.ctx) llama_free(g_state.ctx);
        if (g_state.model) llama_free_model(g_state.model);
        llama_backend_free();
        std::cout << "[AI] Resources released." << std::endl;
    }
}
