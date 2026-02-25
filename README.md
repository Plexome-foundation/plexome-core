# 🧬 PLEXOME FOUNDATION | Swarm AI Node v2.0

**Plexome** is a decentralized, peer-to-peer AI network designed for distributed inference and federated learning. By applying **Enterprise Storage Principles (AI-RAID)** to neural networks, Plexome allows a swarm of standard computers to function as a single, massive virtual supercomputer.



## 🚀 Key Features (v2.0 Pro)

* **AI-RAID (Pipeline Parallelism):** Just like RAID-0 stripes data across disks, Plexome stripes LLM layers across nodes. If a model (e.g., 70B parameters) is too large for one GPU, the swarm splits the layers across multiple participants.
* **Hardware Benchmarking (PlexoParrots):** An integrated 2-second stress test evaluates node performance upon startup. The system automatically assigns a tier: **Potato**, **Standard**, or **Titan**.
* **Federated LoRA Injection:** Nodes can learn specialized technical data (e.g., HPE 3PAR, Primera, Alletra documentation) locally. These "knowledge deltas" (LoRA) are then synchronized across the swarm without sharing raw data.
* **Swarm Routing:** Intelligent tensor routing via port **7539**. Computation results travel through the pipeline with low latency using an optimized P2P bus.
* **Cross-Platform Core:** Native C++20 implementation for Windows (Winsock2) and Linux, featuring graceful shutdown and static linking capabilities.

## 🛠 Quick Start (Windows)

### Prerequisites
* Visual Studio 2022 (with "Desktop development with C++" and "CMake tools").
* At least 8GB of RAM (16GB+ recommended for Titan nodes).

### Build Instructions
1.  **Clone the Repo:** Open Visual Studio -> Clone a Repository -> `https://github.com/your-repo/plexome`.
2.  **Configure:** Wait for CMake to finish generating the cache.
3.  **Build:** Set configuration to `x64-Release` and press `Ctrl+Shift+B`.
4.  **Download Models:**
    ```powershell
    .\scripts\download_models.ps1
    ```

### Configuration (`plexome.conf`)
Ensure your config file is in the same directory as the `.exe`:
```ini
port=7539
is_seed=false
vram_limit_gb=8
storage_path=./data
