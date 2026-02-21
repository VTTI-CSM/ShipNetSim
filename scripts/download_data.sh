#!/usr/bin/env bash
# Download and extract ShipNetSim data files from OneDrive.
# Usage: ./scripts/download_data.sh [target_dir]
#   target_dir defaults to src/data/ relative to the repository root.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TARGET_DIR="${1:-$REPO_ROOT/src/data}"
ZIP_FILE="$(mktemp /tmp/shipnetsim_data_XXXXXX.zip)"

# Dropbox direct download link (dl=1 forces direct download)
DOWNLOAD_URL="https://www.dropbox.com/scl/fi/obf8duo6ewnez5q363vwn/data.zip?rlkey=9by8u5ci1miag453rhvp2ilmw&st=ycbk7bjc&dl=1"

cleanup() {
    rm -f "$ZIP_FILE"
    rm -rf /tmp/shipnetsim_extract_* 2>/dev/null || true
}
trap cleanup EXIT

echo "=== ShipNetSim Data Downloader ==="
echo ""
echo "Target directory: $TARGET_DIR"
echo ""

# Check for required tools
if ! command -v curl &> /dev/null && ! command -v wget &> /dev/null; then
    echo "ERROR: curl or wget is required but neither was found."
    exit 1
fi

if ! command -v unzip &> /dev/null; then
    echo "ERROR: unzip is required but was not found."
    exit 1
fi

# Download the zip file
echo "Downloading data files..."
if command -v curl &> /dev/null; then
    curl -L -o "$ZIP_FILE" "$DOWNLOAD_URL" --progress-bar
elif command -v wget &> /dev/null; then
    wget -O "$ZIP_FILE" "$DOWNLOAD_URL" --show-progress
fi

# Verify download succeeded
if [ ! -s "$ZIP_FILE" ]; then
    echo "ERROR: Download failed or file is empty."
    echo ""
    echo "Please download manually from:"
    echo "  $ONEDRIVE_SHARE_URL"
    echo ""
    echo "Then extract the contents to: $TARGET_DIR"
    exit 1
fi

# Create target directory
mkdir -p "$TARGET_DIR"

# Extract to a temporary directory first, then move files to target
EXTRACT_DIR="$(mktemp -d /tmp/shipnetsim_extract_XXXXXX)"
echo "Extracting to $TARGET_DIR..."
unzip -o "$ZIP_FILE" -d "$EXTRACT_DIR"

# If the zip contains a single top-level directory, move its contents up
TOPLEVEL=("$EXTRACT_DIR"/*)
if [ ${#TOPLEVEL[@]} -eq 1 ] && [ -d "${TOPLEVEL[0]}" ]; then
    cp -a "${TOPLEVEL[0]}"/* "$TARGET_DIR"/
else
    cp -a "$EXTRACT_DIR"/* "$TARGET_DIR"/
fi
rm -rf "$EXTRACT_DIR"

# Verify key files exist
echo ""
echo "Verifying extraction..."
EXPECTED_FILES=(
    "NE1_HR_LC_SR_W_DR.tif"
    "ne_110m_ocean.shp"
    "WaterDepth.tif"
)

ALL_OK=true
for f in "${EXPECTED_FILES[@]}"; do
    if [ -f "$TARGET_DIR/$f" ]; then
        echo "  [OK] $f"
    else
        echo "  [MISSING] $f"
        ALL_OK=false
    fi
done

echo ""
if $ALL_OK; then
    echo "Data files downloaded and extracted successfully."
else
    echo "WARNING: Some expected files are missing."
    echo "The zip may have a subdirectory. Check $TARGET_DIR and move files if needed."
fi
