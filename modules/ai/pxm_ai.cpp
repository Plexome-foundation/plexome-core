#include "../../include/module_interface.h"
#include "llama.h"
#include <iostream>

struct AiInternalState {
    llama_model* model = nullptr;
    llama_context* ctx = nullptr;
};

static AiInternalState g_state;

extern "C" {
    PXM_API PxmModuleInfo pxm_get_info() {
        return {
            "Plexome AI Engine",
            "2.0.1-alpha",
            "Hardware-accelerated inference (llama.cpp core)"
        };
    }

    PXM_API PxmStatus pxm_init(const PxmConfig* config) {
        std::cout << "[AI] Initializing llama backend..." << std::endl;
        
        // Глобальная инициализация llama.cpp (делается один раз)
        llama_backend_init();

        // В будущем здесь будет загрузка модели из config->model_path
        if (config->model_path && strlen(config->model_path) > 0) {
            std::cout << "[AI] Target model: " << config->model_path << std::endl;
        }

        return PxmStatus::OK;
    }

    PXM_API void pxm_shutdown() {
        if (g_state.ctx) llama_free(g_state.ctx);
        if (g_state.model) llama_free_model(g_state.model);
        
        llama_backend_free();
        std::cout << "[AI] Llama backend freed." << std::endl;
    }
}
