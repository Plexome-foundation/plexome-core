#!/bin/bash
# Plexome Model Downloader

mkdir -p models
echo "Downloading DeepSeek-Coder 1.3B (GGUF)..."

# Using curl to download from Hugging Face (example link)
curl -L https://huggingface.co/TheBloke/deepseek-coder-1.3B-instruct-GGUF/resolve/main/deepseek-coder-1.3b-instruct.Q4_K_M.gguf -o models/coder-1.3b.gguf

echo "Model ready in ./models/coder-1.3b.gguf"
