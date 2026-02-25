# Plexome Core v2.0 🧠🌐

**Plexome** is a high-performance, decentralized AI swarm engine designed for autonomous continuous learning and distributed inference. It enables thousands of consumer-grade PCs to act as a single, collective intelligence.

---

## 🚀 The Vision: V2.0 Modular Architecture
The system has transitioned from a monolithic design to a **Modular Plugin Architecture**. This allows the network to update its core components (AI logic or Network stack) on-the-fly without restarting the node.



### Core Components:
* **`plexome_host.exe` (The Orchestrator):** A lightweight dispatcher that manages module lifecycles and routes tasks between blocks.
* **`pxm_ai.dll` (The Brain):** Multithreaded LLM engine based on `llama.cpp`. Optimized for **Phi-3-mini** with parallel context pooling and smart stop-token handling.
* **`pxm_network.dll` (The Senses):** Decentralized P2P stack using a custom Swarm protocol for real-time model synchronization and weight exchange.

---

## 🛠 Project Structure
The project is organized into isolated, maintainable blocks:

```text
/plexome-core
│   CMakeLists.txt          # Unified build system
├───include
│       module_interface.h  # Universal DLL contract (C-style API)
│       plexome_types.h     # Swarm-wide data structures (Roles & Tiers)
├───src
│   ├───host                # Main executable logic
│   └───modules
│       ├───ai              # AI Inference & LoRA logic
│       └───network         # P2P communication stack
└───models                  # Local storage for GGUF weights

🧠 Swarm Intelligence Features
Performance Tiers: Nodes automatically categorize themselves as Potato, Standard, or Titan (Queen) based on hardware benchmarks.

Decentralized Federated Learning: Nodes scan local/web data, generate 50MB LoRA adapters, and submit them for consensus-based merging.

Hallucination Protection: The "Council of Queens" verifies training results through weight-vector similarity checks before committing to the global model.

🏗 Build Instructions
Plexome Core is built using CMake and requires a C++20 compliant compiler (MSVC 2022+ recommended for Windows).

Clone with submodules:

Bash
git clone --recursive [https://github.com/Plexome-foundation/plexome-core](https://github.com/Plexome-foundation/plexome-core)
Build the system:

Bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
Run the node:

As a Seed: plexome_host.exe --seed

As a Peer: plexome_host.exe

🗺 Roadmap
[x] Transition to Modular DLL Architecture.

[ ] Phase 3 (Next): Implement automated Hardware Benchmarking ("The Parrot Meter").

[ ] Phase 4: Develop the Distributed Web-Scanner for automated knowledge injection.

[ ] Phase 5: Peer-to-peer LoRA weight aggregation and consensus engine.

Developed by Georgii and the AI Swarm Community. Built for the era of autonomous intelligence.
