# Download and extract ShipNetSim data files from Dropbox.
# Usage: .\scripts\download_data.ps1 [target_dir]
#   target_dir defaults to src\data\ relative to the repository root.

param(
    [string]$TargetDir
)

$ErrorActionPreference = 'Stop'

$ScriptDir = $PSScriptRoot
$RepoRoot = (Resolve-Path "$ScriptDir\..").Path
if (-not $TargetDir) {
    $TargetDir = Join-Path $RepoRoot "src\data"
}

# Dropbox direct download link (dl=1 forces direct download)
$DownloadUrl = "https://www.dropbox.com/scl/fi/obf8duo6ewnez5q363vwn/data.zip?rlkey=9by8u5ci1miag453rhvp2ilmw&st=ycbk7bjc&dl=1"

$ZipFile = Join-Path ([System.IO.Path]::GetTempPath()) "shipnetsim_data_$([System.IO.Path]::GetRandomFileName()).zip"
$ExtractDir = Join-Path ([System.IO.Path]::GetTempPath()) "shipnetsim_extract_$([System.IO.Path]::GetRandomFileName())"

function Cleanup {
    if (Test-Path $ZipFile) { Remove-Item $ZipFile -Force -ErrorAction SilentlyContinue }
    if (Test-Path $ExtractDir) { Remove-Item $ExtractDir -Recurse -Force -ErrorAction SilentlyContinue }
}

try {
    Write-Host "=== ShipNetSim Data Downloader ==="
    Write-Host ""
    Write-Host "Target directory: $TargetDir"
    Write-Host ""

    # Download the zip file
    Write-Host "Downloading data files..."
    Invoke-WebRequest -Uri $DownloadUrl -OutFile $ZipFile -UseBasicParsing

    # Verify download succeeded
    if (-not (Test-Path $ZipFile) -or (Get-Item $ZipFile).Length -eq 0) {
        Write-Host "ERROR: Download failed or file is empty."
        Write-Host ""
        Write-Host "Please download manually from:"
        Write-Host "  $DownloadUrl"
        Write-Host ""
        Write-Host "Then extract the contents to: $TargetDir"
        exit 1
    }

    # Create target directory
    if (-not (Test-Path $TargetDir)) {
        New-Item -ItemType Directory -Path $TargetDir -Force | Out-Null
    }

    # Extract to a temporary directory first, then move files to target
    Write-Host "Extracting to $TargetDir..."
    Expand-Archive -Path $ZipFile -DestinationPath $ExtractDir -Force

    # If the zip contains a single top-level directory, move its contents up
    $TopLevel = Get-ChildItem -Path $ExtractDir
    if ($TopLevel.Count -eq 1 -and $TopLevel[0].PSIsContainer) {
        Copy-Item -Path (Join-Path $TopLevel[0].FullName "*") -Destination $TargetDir -Recurse -Force
    } else {
        Copy-Item -Path (Join-Path $ExtractDir "*") -Destination $TargetDir -Recurse -Force
    }

    # Verify key files exist
    Write-Host ""
    Write-Host "Verifying extraction..."
    $ExpectedFiles = @(
        "NE1_HR_LC_SR_W_DR.tif",
        "ne_110m_ocean.shp",
        "WaterDepth.tif"
    )

    $AllOk = $true
    foreach ($f in $ExpectedFiles) {
        $FilePath = Join-Path $TargetDir $f
        if (Test-Path $FilePath) {
            Write-Host "  [OK] $f"
        } else {
            Write-Host "  [MISSING] $f"
            $AllOk = $false
        }
    }

    Write-Host ""
    if ($AllOk) {
        Write-Host "Data files downloaded and extracted successfully."
    } else {
        Write-Host "WARNING: Some expected files are missing."
        Write-Host "The zip may have a subdirectory. Check $TargetDir and move files if needed."
    }
} finally {
    Cleanup
}
