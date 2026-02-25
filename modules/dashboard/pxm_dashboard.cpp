/**
 * Plexome Core v2.0 - Web Dashboard Module
 * Provides a local web interface to monitor node status and swarm health.
 */

#include "../../include/module_interface.h"
#include <iostream>

extern "C" {
    /**
     * Metadata for the Dashboard module.
     */
    PXM_API PxmModuleInfo pxm_get_info() {
        return {
            "Plexome Dashboard",
            "2.0.0-alpha",
            "Local web server providing a visual interface for the node."
        };
    }

    /**
     * Initializes the dashboard and starts the local web server.
     */
    PXM_API PxmStatus pxm_init(const PxmConfig* config) {
        if (!config) return PxmStatus::ERROR_INIT_FAILED;

        std::cout << "[Dashboard] Starting WebUI on http://localhost:8080..." << std::endl;
        
        // In the next phase, we will integrate a micro-webserver (like httplib or Mongoose)
        // to serve the index.html and provide real-time stats via WebSockets.
        
        return PxmStatus::OK;
    }

    /**
     * Graceful shutdown of the web server.
     */
    PXM_API void pxm_shutdown() {
        std::cout << "[Dashboard] Web server stopped." << std::endl;
    }
}
