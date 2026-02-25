bool ConnectionManager::connect_to_seed(const std::string& host, int port) {
#ifdef _WIN32
        addrinfo hints = {0};
        addrinfo* result = nullptr;

        hints.ai_family = AF_INET;       // IPv4
        hints.ai_socktype = SOCK_STREAM; // TCP
        hints.ai_protocol = IPPROTO_TCP;

        std::string port_str = std::to_string(port);
        std::cout << "[Network] Resolving DNS for seed: " << host << "...\n";

        // Магия DNS-резолвинга
        DWORD dwRetval = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &result);
        if (dwRetval != 0) {
            std::cerr << "[Network] DNS resolution failed for " << host << ". Error code: " << dwRetval << "\n";
            return false;
        }

        SOCKET connect_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (connect_socket == INVALID_SOCKET) {
            freeaddrinfo(result);
            return false;
        }

        std::cout << "[Network] Attempting to connect to Swarm at " << host << ":" << port << "...\n";
        
        int connect_res = connect(connect_socket, result->ai_addr, (int)result->ai_addrlen);
        if (connect_res == SOCKET_ERROR) {
            std::cerr << "[Network] Failed to connect to seed.\n";
            closesocket(connect_socket);
            freeaddrinfo(result);
            return false;
        }

        // Обязательно чистим память после успешного резолва
        freeaddrinfo(result); 

        std::lock_guard<std::mutex> lock(sockets_mtx_);
        active_sockets_.push_back(connect_socket);
        std::cout << "[Network] Successfully connected to Swarm via DNS!\n";
        return true;
#else
        return false;
#endif
    }
