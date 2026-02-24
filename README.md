# PLEXOME Architecture: The Storage-First AI

## 1. Design Philosophy
Unlike typical LLM deployments, PLEXOME treats neural network weights as a **Distributed Logical Volume**. We focus on high-availability, low-latency weight streaming, and federated intelligence.

## 2. P.S.B. (Plexome Storage Bus)
The core transport layer.
* **Sharding:** Models are split into 4MB - 8MB blocks (Pages).
* **Direct Access:** Nodes use memory-mapped files (mmap) to access local caches.
* **QUIC Protocol:** All inter-node communication happens over encrypted UDP/QUIC to handle packet loss and jitter in consumer internet connections.

## 3. Memory Tiering (L1/L2/L3)
* **L1 (VRAM/Hot):** Active attention layers for immediate compute.
* **L2 (Local RAM/Warm):** Prefetched layers stored in system memory.
* **L3 (Network/Cold):** Weights stored on peer nodes across the global fabric.

## 4. Federated Evolution
Weights are updated via asynchronous LoRA training.
* **Validation:** Nodes perform "Proof-of-Logic" checks on merged weights.
* **Consensus:** Only updates with high fitness scores are merged into the global model state.
