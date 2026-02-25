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

