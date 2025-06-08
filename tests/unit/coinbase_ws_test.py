import websocket
import json

def on_open(ws):
    subscribe_message = {
        "type": "subscribe",
        "product_ids": ["BTC-USD", "ETH-USD"],
        "channels": ["ticker"]
    }
    print("Subscribing:", subscribe_message)
    ws.send(json.dumps(subscribe_message))

def on_message(ws, message):
    print("Received:", message)

def on_error(ws, error):
    print("Error:", error)

def on_close(ws, close_status_code, close_msg):
    print("Connection closed:", close_status_code, close_msg)

if __name__ == "__main__":
    websocket.enableTrace(True)
    ws = websocket.WebSocketApp(
        "wss://ws-feed.exchange.coinbase.com",
        on_open=on_open,
        on_message=on_message,
        on_error=on_error,
        on_close=on_close
    )
    ws.run_forever()