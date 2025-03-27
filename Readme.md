# 🚀 High-Performance HTTP Proxy Server

## 🌍 What is it?
A blazing-fast, multi-threaded HTTP Proxy Server that efficiently forwards client requests to remote servers, caches responses, and serves cached data for improved performance. It is designed for reliability, speed, and scalability.

## 🤔 Why does it exist?
Handling HTTP requests efficiently while reducing redundant network traffic is a common challenge in networking and backend systems. This project was built to:
- **Optimize network performance** by caching frequently requested responses.
- **Enhance security** through request filtering and controlled forwarding.
- **Simplify socket programming** with an intuitive **Socket Library** that abstracts low-level networking complexities.
- **Provide a lightweight alternative** to traditional proxy solutions like Squid and Nginx.

## 🛠 How does it work?
1. Listens for incoming HTTP client requests.
2. Parses and validates the request using a custom **HTTP Parser**.
3. Checks for cached responses to avoid redundant network requests.
4. If not cached, forwards the request to the target server using the **Socket Library**.
5. Receives, processes, and caches the response for future requests.
6. Sends the response back to the client.

## 🏗 How was it built?
### 🔹 Custom **Socket Library**
We implemented a **robust and efficient socket handling library** that abstracts complex networking operations, making server-client communication seamless. Features include:
- **Easy server creation** (`createServer()`, `acceptClient()`)
- **Effortless client connections** (`createConnection()`)
- **Robust data transmission** (`sendMessage()`, `recvAllData()`)

### 🔹 Custom **HTTP Parser**
A lightweight **HTTP request and response parser** was developed to handle parsing efficiently:
- Extracts headers, methods, URLs, and request bodies.
- Constructs HTTP requests and responses from raw data.
- Handles malformed or incomplete HTTP messages gracefully.

### 🔹 Efficient **Caching System**
The proxy integrates a **Linked List-based caching mechanism** for quick lookups and reduced memory overhead:
- Stores frequently requested responses.
- Serves cached responses instantly when available.
- Avoids redundant requests to remote servers.

## 🚀 Getting Started
### 📌 Prerequisites
- Linux/macOS (recommended for socket operations)
- GCC Compiler

### 🏃‍♂️ Run the Proxy Server
```sh
make
./proxy <PORT>
```
Example:
```sh
./proxy 8080
```

## 🛠 Features
✅ **High-performance socket handling** with a custom socket library  
✅ **Multi-threaded request processing** for speed  
✅ **Custom-built HTTP parser** for fast request/response handling  
✅ **Intelligent caching system** to optimize performance  
✅ **Automatic error handling** for better stability  
✅ **Modular and extensible design** for easy customization  

## 🤝 Contributing
Contributions, issues, and feature requests are welcome! Feel free to fork and improve.


🚀 **Enjoy blazing-fast HTTP proxying!**

