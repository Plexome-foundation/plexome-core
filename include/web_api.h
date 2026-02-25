#pragma once
#include "stats_collector.h"
#include <sstream>

namespace plexome {

    /**
     * Lightweight Web API provider.
     * Generates HTML/JSON for the dashboard.
     */
    class WebUI {
    public:
        static std::string render_dashboard(const StatsCollector& stats) {
            auto s = stats.get_snapshot();
            std::stringstream html;
            html << "<html><head><title>Plexome Dashboard</title>";
            html << "<style>body{font-family:monospace; background:#000; color:#0f0; padding:20px;}</style>";
            html << "</head><body>";
            html << "<h1>PLEXOME SWARM MONITOR</h1>";
            html << "<hr>";
            html << "<div>ACTIVE NODES: " << s.active_nodes << "</div>";
            html << "<div>CURRENT MODEL: " << s.current_model << "</div>";
            html << "<div>TASKS IN FLIGHT: " << s.pending_tasks << "</div>";
            html << "<div>NODES TRAINING (LoRA): " << s.training_nodes_count << "</div>";
            html << "<hr>";
            html << "<p>System Status: OPTIMAL</p>";
            html << "</body></html>";
            return html.str();
        }
    };
}
