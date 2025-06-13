#include "MarketDataEngine.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace crypto_hft {

MarketDataEngine::MarketDataEngine(std::vector<std::shared_ptr<IExchangeAdapter>> adapters) 
    : adapters_(adapters), running_(false) {
    logger_ = spdlog::stdout_color_mt("market_data_engine");
    logger_->set_level(spdlog::level::debug);
}

MarketDataEngine::~MarketDataEngine() {
    stop();
}

void MarketDataEngine::initialize(const std::vector<std::string>& symbols) {
    if (running_) {
        logger_->warn("MarketDataEngine already initialized");
        return;
    }

    // Create order books for each symbol
    {
        std::lock_guard<std::mutex> lock(orderBooksMutex_);
        for (const auto& symbol : symbols) {
            orderBooks_[symbol] = std::make_shared<OrderBook>(symbol);
        }
    }

    // Register callbacks with each adapter
    for (auto& adapter : adapters_) {
        adapter->registerOrderBookCallback([this](const OrderBookSnapshot& snapshot) {
            handleOrderBookSnapshot(snapshot);
        });

        adapter->registerOrderBookDeltaCallback([this](const OrderBookDelta& delta) {
            handleOrderBookDelta(delta);
        });
    }

    // Start processing threads
    running_ = true;
    for (auto& adapter : adapters_) {
        receiverThreads_.emplace_back(&MarketDataEngine::receiverThread, this, adapter);
    }
    dispatcherThread_ = std::thread(&MarketDataEngine::dispatcherThread, this);
}

void MarketDataEngine::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    
    // Wait for all threads to finish
    for (auto& thread : receiverThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    receiverThreads_.clear();

    if (dispatcherThread_.joinable()) {
        dispatcherThread_.join();
    }
}

void MarketDataEngine::registerCallback(std::function<void(const MarketUpdate&)> callback) {
    updateCallback_ = std::move(callback);
}

std::shared_ptr<OrderBook> MarketDataEngine::getOrderBook(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(orderBooksMutex_);
    auto it = orderBooks_.find(symbol);
    if (it != orderBooks_.end()) {
        return it->second;
    }
    return nullptr;
}

void MarketDataEngine::receiverThread(std::shared_ptr<IExchangeAdapter> adapter) {
    logger_->info("Started receiver thread for adapter: {}", adapter->getName());
    
    while (running_) {
        // The adapter's callbacks will be called from its own thread
        // We just need to keep this thread alive
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void MarketDataEngine::dispatcherThread() {
    logger_->info("Started dispatcher thread");
    
    MarketUpdate update;
    while (running_) {
        if (updateQueue_.try_dequeue(update)) {
            // Notify subscribers
            if (updateCallback_) {
                updateCallback_(update);
            }
        } else {
            // Avoid busy waiting
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
}

void MarketDataEngine::handleOrderBookSnapshot(const OrderBookSnapshot& snapshot) {
    auto orderBook = getOrderBook(snapshot.symbol);
    if (!orderBook) {
        logger_->error("Received snapshot for unknown symbol: {}", snapshot.symbol);
        return;
    }

    // Convert snapshot to maps
    std::map<double, double> bids;
    std::map<double, double> asks;
    
    for (const auto& bid : snapshot.bids) {
        bids[bid.first] = bid.second;
    }
    
    for (const auto& ask : snapshot.asks) {
        asks[ask.first] = ask.second;
    }

    // Update order book
    orderBook->setSnapshot(bids, asks);

    // Create market update
    MarketUpdate update;
    update.symbol = snapshot.symbol;
    update.timestamp = snapshot.timestamp;
    update.isSnapshot = true;
    
    if (!bids.empty()) {
        update.bidPrice = bids.begin()->first;
        update.bidSize = bids.begin()->second;
    }
    
    if (!asks.empty()) {
        update.askPrice = asks.begin()->first;
        update.askSize = asks.begin()->second;
    }

    // Queue update for dispatcher
    updateQueue_.enqueue(update);
}

void MarketDataEngine::handleOrderBookDelta(const OrderBookDelta& delta) {
    auto orderBook = getOrderBook(delta.symbol);
    if (!orderBook) {
        logger_->error("Received delta for unknown symbol: {}", delta.symbol);
        return;
    }

    // Apply updates to order book
    for (const auto& bid : delta.bidUpdates) {
        if (bid.second == 0) {
            orderBook->removeBid(bid.first);
        } else {
            orderBook->updateBid(bid.first, bid.second);
        }
    }

    for (const auto& ask : delta.askUpdates) {
        if (ask.second == 0) {
            orderBook->removeAsk(ask.first);
        } else {
            orderBook->updateAsk(ask.first, ask.second);
        }
    }

    // Create market update
    MarketUpdate update;
    update.symbol = delta.symbol;
    update.timestamp = delta.timestamp;
    update.isSnapshot = false;
    update.bidPrice = orderBook->getBestBid();
    update.askPrice = orderBook->getBestAsk();
    update.bidSize = orderBook->getBidVolume(update.bidPrice);
    update.askSize = orderBook->getAskVolume(update.askPrice);

    // Queue update for dispatcher
    updateQueue_.enqueue(update);
}

} // namespace crypto_hft 