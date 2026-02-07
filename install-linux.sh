#!/bin/bash
# FastLog Linux Installer - Downloads and installs FastLog

set -e

REPO="AGDNoob/FastLog"
BINARY_NAME="fastlog"
INSTALL_DIR="/usr/local/bin"
DOWNLOAD_URL="https://github.com/$REPO/releases/latest/download/fastlog-linux-x64"

echo "FastLog Installer"
echo "================="

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Error: Please run with sudo"
    echo "Usage: curl -fsSL https://raw.githubusercontent.com/$REPO/main/dist/install-linux.sh | sudo bash"
    exit 1
fi

# Check for curl or wget
if command -v curl &> /dev/null; then
    DOWNLOADER="curl -fsSL -o"
elif command -v wget &> /dev/null; then
    DOWNLOADER="wget -q -O"
else
    echo "Error: curl or wget required"
    exit 1
fi

# Download
echo "Downloading from GitHub..."
TMP_FILE=$(mktemp)
$DOWNLOADER "$TMP_FILE" "$DOWNLOAD_URL"

# Install
echo "Installing to $INSTALL_DIR/$BINARY_NAME..."
mv "$TMP_FILE" "$INSTALL_DIR/$BINARY_NAME"
chmod +x "$INSTALL_DIR/$BINARY_NAME"

echo ""
echo "Done! Run 'fastlog --help' to get started."
