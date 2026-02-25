# =================================================================
# PLEXOME FOUNDATION - Model Downloader for Windows
# Target: Coding models (GGUF format)
# =================================================================

$modelDir = "./models"

# Create models directory if it doesn't exist
if (!(Test-Path -Path $modelDir)) {
    New-Item -ItemType Directory -Path $modelDir | Out-Null
    Write-Host "[System] Created directory: $modelDir" -ForegroundColor Cyan
}

Write-Host "-------------------------------------------------------" -ForegroundColor Green
Write-Host "PLEXOME | Initializing Model Download (Windows Tier)" -ForegroundColor Green
Write-Host "-------------------------------------------------------" -ForegroundColor Green

function Download-PlexomeModel {
    param (
        [string]$Name,
        [string]$Url
    )

    $filePath = Join-Path -Path $modelDir -ChildPath $Name

    if (Test-Path -Path $filePath) {
        Write-Host "[Skip] $Name already exists." -ForegroundColor Yellow
    } else {
        Write-Host "[Downloading] $Name..." -ForegroundColor White
        try {
            # Using ProgressPreference to speed up download
            $ProgressPreference = 'SilentlyContinue'
            Invoke-WebRequest -Uri $Url -OutFile $filePath -ErrorAction Stop
            Write-Host "[Success] $Name downloaded." -ForegroundColor Green
        } catch {
            Write-Host "[Error] Failed to download $Name. Details: $_" -ForegroundColor Red
        }
    }
}

# 1. Light model for CPU-only nodes (DeepSeek-Coder 1.3B)
Download-PlexomeModel -Name "coder-1.3b.gguf" `
-Url "https://huggingface.co/mradermacher/deepseek-coder-1.3b-instruct-GGUF/resolve/main/deepseek-coder-1.3b-instruct.Q4_K_M.gguf"

# 2. Main coding model for Titan nodes (DeepSeek-Coder 7B)
Download-PlexomeModel -Name "coder-7b.gguf" `
-Url "https://huggingface.co/mradermacher/deepseek-coder-7b-instruct-v1.5-GGUF/resolve/main/deepseek-coder-7b-instruct-v1.5.Q4_K_M.gguf"

Write-Host "-------------------------------------------------------" -ForegroundColor Green
Write-Host "All models ready in $modelDir" -ForegroundColor Green
Write-Host "Ready to launch Plexome Swarm." -ForegroundColor Green
