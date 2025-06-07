#!/bin/bash

# Exit on error
set -e

# Configuration
BUILD_DIR="build"
DATA_DIR="data/historical"
RESULTS_DIR="data/backtest_results"
CONFIG_FILE="configs/default_config.yaml"

# Parse command line arguments
START_DATE=""
END_DATE=""
PAIR=""
while [[ $# -gt 0 ]]; do
    case $1 in
        --start-date)
            START_DATE="$2"
            shift 2
            ;;
        --end-date)
            END_DATE="$2"
            shift 2
            ;;
        --pair)
            PAIR="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Validate arguments
if [ -z "$START_DATE" ] || [ -z "$END_DATE" ] || [ -z "$PAIR" ]; then
    echo "Usage: $0 --start-date YYYY-MM-DD --end-date YYYY-MM-DD --pair SYMBOL1-SYMBOL2"
    exit 1
fi

# Create results directory
mkdir -p "$RESULTS_DIR"

# Update configuration for backtest
echo "Updating configuration for backtest..."
TEMP_CONFIG=$(mktemp)
cp "$CONFIG_FILE" "$TEMP_CONFIG"

# Modify configuration for backtest
yq e ".backtesting.enabled = true" -i "$TEMP_CONFIG"
yq e ".backtesting.start_date = \"$START_DATE\"" -i "$TEMP_CONFIG"
yq e ".backtesting.end_date = \"$END_DATE\"" -i "$TEMP_CONFIG"

# Split pair into symbols
SYMBOL1=$(echo "$PAIR" | cut -d'-' -f1)
SYMBOL2=$(echo "$PAIR" | cut -d'-' -f2)

# Update trading pairs
yq e ".trading.pairs[0].symbol1 = \"$SYMBOL1\"" -i "$TEMP_CONFIG"
yq e ".trading.pairs[0].symbol2 = \"$SYMBOL2\"" -i "$TEMP_CONFIG"

# Build the project if needed
if [ ! -f "$BUILD_DIR/crypto_hft" ]; then
    echo "Building project..."
    cd "$BUILD_DIR"
    cmake ..
    make -j$(nproc)
    cd ..
fi

# Run backtest
echo "Running backtest..."
"$BUILD_DIR/crypto_hft" --config "$TEMP_CONFIG" --backtest > "$RESULTS_DIR/backtest_${PAIR}_${START_DATE}_${END_DATE}.log"

# Generate performance report
echo "Generating performance report..."
"$BUILD_DIR/crypto_hft" --analyze "$RESULTS_DIR/backtest_${PAIR}_${START_DATE}_${END_DATE}.log" > "$RESULTS_DIR/report_${PAIR}_${START_DATE}_${END_DATE}.txt"

# Clean up
rm "$TEMP_CONFIG"

echo "Backtest complete!"
echo "Results saved in: $RESULTS_DIR"
echo "View report with: cat $RESULTS_DIR/report_${PAIR}_${START_DATE}_${END_DATE}.txt" 