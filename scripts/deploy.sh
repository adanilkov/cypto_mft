#!/bin/bash

# Exit on error
set -e

# Configuration
BUILD_DIR="build"
INSTALL_DIR="/opt/crypto_hft"
CONFIG_DIR="/etc/crypto_hft"
LOG_DIR="/var/log/crypto_hft"
DATA_DIR="/var/lib/crypto_hft"

# Create necessary directories
mkdir -p "$BUILD_DIR"
mkdir -p "$INSTALL_DIR"
mkdir -p "$CONFIG_DIR"
mkdir -p "$LOG_DIR"
mkdir -p "$DATA_DIR"

# Build the project
echo "Building project..."
cd "$BUILD_DIR"
cmake ..
make -j$(nproc)

# Install
echo "Installing..."
sudo make install

# Copy configuration files
echo "Copying configuration files..."
sudo cp ../configs/default_config.yaml "$CONFIG_DIR/"

# Set up systemd service
echo "Setting up systemd service..."
sudo tee /etc/systemd/system/crypto_hft.service << EOF
[Unit]
Description=Crypto HFT Trading System
After=network.target

[Service]
Type=simple
User=crypto_hft
Group=crypto_hft
WorkingDirectory=$INSTALL_DIR
ExecStart=$INSTALL_DIR/bin/crypto_hft
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF

# Create system user if it doesn't exist
if ! id "crypto_hft" &>/dev/null; then
    echo "Creating system user..."
    sudo useradd -r -s /bin/false crypto_hft
fi

# Set permissions
echo "Setting permissions..."
sudo chown -R crypto_hft:crypto_hft "$INSTALL_DIR"
sudo chown -R crypto_hft:crypto_hft "$CONFIG_DIR"
sudo chown -R crypto_hft:crypto_hft "$LOG_DIR"
sudo chown -R crypto_hft:crypto_hft "$DATA_DIR"

# Reload systemd
echo "Reloading systemd..."
sudo systemctl daemon-reload

# Enable and start service
echo "Enabling and starting service..."
sudo systemctl enable crypto_hft
sudo systemctl start crypto_hft

echo "Deployment complete!"
echo "Check service status with: sudo systemctl status crypto_hft"
echo "View logs with: sudo journalctl -u crypto_hft -f" 