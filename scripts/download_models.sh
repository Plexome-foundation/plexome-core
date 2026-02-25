#!/bin/bash

# =================================================================
# PLEXOME FOUNDATION - Model Downloader for Linux
# Target: Coding models (GGUF format for llama.cpp/Plexome Core)
# =================================================================

# Ensure we are in the project root
MODEL_DIR="./models"
mkdir -p "$MODEL_DIR"

echo "-------------------------------------------------------"
echo "PLEXOME | Initializing Model Download (Linux Tier)"
echo "-------------------------------------------------------"

# Function to download models
download_model() {
    local NAME=$1
    local URL=$2
    local FILE_PATH="$MODEL_DIR/$NAME"

    if [ -f "$FILE_PATH" ]; then
        echo "[Skip] $NAME already exists."
    else
        echo "[Downloading] $NAME..."
        curl -L "$URL" -o "$FILE_PATH"
        if [ $? -eq 0 ]; then
            echo "[Success] $NAME downloaded."
        else
            echo "[Error] Failed to download $NAME."
            return 1
        fi
    fi
}

# 1. Light model for CPU-only nodes (DeepSeek-Coder 1.3B)
# Ideal for low-RAM VPS and Archivist nodes
download_model "coder-1.3b.gguf" \
"https://huggingface.co/mradermacher/deepseek-coder-1.3b-instruct-GGUF/resolve/main/deepseek-coder-1.3b-instruct.Q4_K_M.gguf"

# 2. Main coding model for Titan nodes (DeepSeek-Coder 7B)
# Required for reliable code generation
download_model "coder-7b.gguf" \
"https://huggingface.co/mradermacher/deepseek-coder-7b-instruct-v1.5-GGUF/resolve/main/deepseek-coder-7b-instruct-v1.5.Q4_K_M.gguf"

echo "-------------------------------------------------------"
echo "All models ready in $MODEL_DIR"
echo "Ready to launch Plexome Swarm."
