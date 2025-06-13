# Crypto-MFT: Mid-Frequency Cryptocurrency Trading System (In Development)

A high-performance, real-time cryptocurrency trading system being built in C++20, designed for mid frequency trading across multiple exchanges. This project is currently in active development.

## Current Status

The project is in early development stage with the following components currently implemented:

- Basic project structure and build system
- Initial exchange adapter framework
- Configuration management system
- Core infrastructure components

## Planned Features

The following features are planned for implementation (implemented to varying degrees at current moment):

- **Multi-Exchange Support**: Simultaneous trading on Binance, Coinbase, and Kraken
- **Real-Time Market Data**: WebSocket-based order book streaming with sub-millisecond latency
- **Risk Management**: Real-time position tracking, drawdown monitoring, and circuit breakers
- **Backtesting Framework**: Historical data replay with configurable parameters
- **Performance Monitoring**: Prometheus metrics integration for system monitoring
- **Configuration Management**: YAML-based configuration with environment variable support

## System Requirements

- C++20 compatible compiler (GCC 10+ or Clang 10+)
- CMake 3.20 or higher
- Boost 1.74.0 or higher
- OpenSSL
- spdlog
- yaml-cpp
- prometheus-cpp
- GTest (for testing)

## Dependencies

The project automatically fetches and builds the following dependencies:
- nlohmann/json (v3.11.3)
- jwt-cpp (v0.7.1)
- moodycamel/ConcurrentQueue (v1.0.3)

## Building the Project

1. Clone the repository:
```bash
git clone https://github.com/adanilkov/crypto-hft.git
cd crypto-hft
```

2. Create a build directory and run CMake:
```bash
mkdir build && cd build
cmake ..
```

3. Build the project:
```bash
make -j$(nproc)
```

## Configuration

The system is configured using YAML files in the `configs/` directory. Key configuration files:

- `default_config.yaml`: Main configuration file
- `test_config.yaml`: Configuration for testing
- Environment variables can override configuration values

### Example Configuration

```yaml
exchanges:
  binance:
    enabled: true
    api_key: ""  # Set in environment
    api_secret: ""  # Set in environment
    testnet: false
    symbols:
      - "BTCUSDT"
      - "ETHUSDT"

trading:
  strategy: "statistical_arbitrage"
  pairs:
    - symbol1: "BTCUSDT"
      symbol2: "ETHUSDT"
      beta: 0.05
      z_entry: 2.0
      z_exit: 0.5

risk:
  max_drawdown: 0.1
  max_daily_loss: 0.05
  max_exposure: 0.3
```

## Project Structure

```
crypto-hft/
├── src/                  
│   ├── core/              # Core trading system components (Initial partial implementation)
│   ├── exchanges/         # Exchange adapters (Initial partial implementation)
│   ├── models/            # Trading models and strategies (Planned)
│   ├── infra/             # Infrastructure components (Initial partial implementation)
│   ├── config/            # Configuration management (Implemented)
│   └── utils/             # Utility functions
├── tests/                 # Unit and integration tests
├── configs/               # Configuration files
├── scripts/               # Utility scripts
├── external/              # External dependencies 
└── logs/                  # Log files

## Development Status

This project is actively being developed. The current focus is on:
1. Building the core exchange adapter framework
2. Implementing basic market data handling
3. Setting up the configuration system
4. Establishing the testing infrastructure

## Running the System

1. Set up your environment variables:
```bash
export COINBASE_API_KEY="your_api_key"
export COINBASE_API_SECRET="your_api_secret"
# Add other exchange credentials as needed
```
OR


2. Run the trading system:
```bash
./build/crypto-hft --config configs/default_config.yaml
```

## Backtesting

Run backtests using the provided script:
```bash
./scripts/run_backtest.sh --start-date 2023-01-01 --end-date 2023-12-31 --pair BTC-ETH
```

## Monitoring

The system exposes Prometheus metrics on port 9090 by default. Key metrics include:
- Order book depth and spread
- Trade execution latency
- Position sizes and PnL
- System resource usage

## Development

### Code Style

The project uses clang-format for code formatting. Format your code with:
```bash
clang-format -i $(find src tests -name "*.cpp" -o -name "*.hpp")
```

### Testing

Run the test suite:
```bash
cd build
make -j$(nproc)
./tests
```

## Contributing

We welcome contributions! The project is in early development, so there are many opportunities to help:

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

Please check the issues section for current development priorities and areas where help is needed.

## License

The MIT License (MIT)
Copyright (c) 2025 adanilkov

## Disclaimer

This software is in development and not ready for production use. Use at your own risk. The authors of this software are not responsible for any financial losses incurred.
