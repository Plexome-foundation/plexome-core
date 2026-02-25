#include "../../include/module_interface.h"
#include <iostream>

extern "C" {
    PXM_API PxmModuleInfo pxm_get_info() {
        return {
            "Plexome Network Stack",
            "2.0.0-alpha",
            "P2P communication and swarm synchronization"
        };
    }

    PXM_API PxmStatus pxm_init(const PxmConfig* config) {
        std::cout << "[Network] Opening port " << config->network_port << "..." << std::endl;
        // Здесь завтра будет инициализация сокетов
        return PxmStatus::OK;
    }

    PXM_API void pxm_shutdown() {
        std::cout << "[Network] Stopping P2P services." << std::endl;
    }
}
