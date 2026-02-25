#pragma once
#include "stats_collector.h"
#include <fstream>
#include <string>

namespace plexome {

    class WebUIExporter {
    public:
        /**
         * Generates a stylized index.html file with live swarm statistics.
         */
        static void export_dashboard(const StatsCollector& stats, const std::string& filepath) {
            auto s = stats.get_snapshot();
            std::ofstream out(filepath);

            out << "<html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='2'>"
                << "<title>PLEXOME | Swarm Control</title>"
                << "<style>"
                << "body { background: #0a0a0a; color: #00ff41; font-family: 'Courier New', monospace; padding: 40px; }"
                << ".container { border: 1px solid #00ff41; padding: 20px; box-shadow: 0 0 15px #00ff41; }"
                << ".stat-row { display: flex; justify-content: space-between; margin-bottom: 10px; border-bottom: 1px solid #1a1a1a; }"
                << ".label { color: #888; }"
                << ".value { font-weight: bold; text-transform: uppercase; }"
                << "h1 { text-shadow: 0 0 10px #00ff41; }"
                << ".tag { background: #00ff41; color: #000; padding: 2px 8px; font-weight: bold; }"
                << "</style></head><body>"
                << "<div class='container'>"
                << "<h1>PLEXOME FOUNDATION | TERMINAL v1.0</h1>"
                << "<p>STATUS: <span class='tag'>ONLINE</span> | PROTOCOL: P2P-STORAGE-BUS</p><hr>"
                
                << "<div class='stat-row'><span class='label'>ACTIVE SWARM NODES:</span><span class='value'>" << s.active_nodes << "</span></div>"
                << "<div class='stat-row'><span class='label'>GLOBAL AI MODEL:</span><span class='value'>" << s.current_model << "</span></div>"
                << "<div class='stat-row'><span class='label'>PENDING TASKS:</span><span class='value'>" << s.pending_tasks << "</span></div>"
                << "<div class='stat-row'><span class='label'>COMPLETED BY NODE:</span><span class='value'>" << s.completed_tasks << "</span></div>"
                << "<div class='stat-row'><span class='label'>NODES IN TRAINING:</span><span class='value'>" << s.training_nodes_count << "</span></div>"
                << "<div class='stat-row'><span class='label'>NETWORK TRAFFIC:</span><span class='value'>" << s.network_throughput_mbps << " MBPS</span></div>"
                
                << "<hr><p>> _ SYSTEM PULSE: OK<br>> _ LISTENING ON PORT 7539...<br>> _ SYNCING SHARDS...</p>"
                << "</div></body></html>";
        }
    };
}
