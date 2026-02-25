#pragma once
#include "stats_collector.h"
#include <iostream>
#include <string>
#include <thread>

namespace plexome {

    class CLIInterface {
    public:
        void run_loop(StatsCollector& stats) {
            std::string command;
            std::cout << "Plexome CLI Ready. Type 'help' for commands." << std::endl;
            
            while (true) {
                std::cout << "pxm> ";
                std::cin >> command;

                if (command == "stats") {
                    auto s = stats.get_snapshot();
                    std::cout << "--- Swarm Status ---" << std::endl;
                    std::cout << "Model: " << s.current_model << std::endl;
                    std::cout << "Tasks Done: " << s.completed_tasks << std::endl;
                } else if (command == "exit") {
                    break;
                } else if (command == "help") {
                    std::cout << "Commands: stats, peers, tasks, exit" << std::endl;
                }
            }
        }
    };
}
