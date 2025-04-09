# 🚀 High-Performance Multi-Threaded HTTP Proxy Server

## 🌍 What is it?
A blazing-fast, **multi-threaded HTTP Proxy Server** that forwards client requests to target servers, caches responses using an **LRU strategy**, and serves cached responses to improve performance. Built with simplicity and reliability in mind.

---

## 🤔 Why this project?
Handling HTTP traffic efficiently while reducing redundant network requests is a real-world challenge. This project was built to:

- 🔁 **Reduce bandwidth usage** by caching frequently requested responses  
- 🔒 **Allow basic request filtering** for security and control  
- ⚡ **Serve multiple clients concurrently** using **POSIX threads (pthreads)**  
- 🧩 Provide a **clean abstraction** over low-level socket operations  
- 🪶 Offer a **lightweight and educational alternative** to large proxy systems

---

## ⚙️ How It Works
1. Accepts HTTP client connections on a specified port.
2. Spawns a new thread for each client connection (or uses a thread pool).
3. Parses the request using a custom **HTTP parser**.
4. Searches a **doubly linked list cache** for a matching response.
5. If cached, sends the response to the client and marks it as recently used.
6. If not cached, forwards the request to the destination server.
7. Caches the response (with LRU eviction if necessary) and sends it to the client.

---

## 🧱 Tech Stack

### 🔹 Custom Socket Library
Simplifies raw socket operations with reusable abstractions:

- `createServer(port)` – Starts a listening socket  
- `acceptClient()` – Accepts incoming connections  
- `createConnection(host, port)` – Connects to remote servers  
- `sendMessage()` / `recvAllData()` – Handles reliable I/O  

### 🔹 Custom HTTP Parser
- Parses HTTP methods, URLs, headers, and body  
- Gracefully handles malformed input  
- Builds HTTP request/response objects from raw data

### 🔹 🌀 LRU Caching System (O(n) Lookup)
- Based on a **doubly linked list**  
- Stores recently used responses  
- Moves accessed nodes to the front (LRU strategy)  
- Performs **O(n) linear search** on every lookup (no hashmap yet)  
- Thread-safe via **mutex locking**

> ℹ️ Future versions may upgrade to a HashMap + LinkedList combo for true **O(1)** lookup and insertion.

---

## 🧪 Features
✅ **Multi-threaded architecture** using `pthread`  
✅ **Basic LRU cache mechanism** with linked list  
✅ **High-performance socket communication**  
✅ **Custom HTTP request/response parsing**  
✅ **Signal handling and graceful shutdown**  
✅ **Extensible design for further enhancements**

---

## 🚀 Getting Started

### 📦 Prerequisites
- Linux/macOS  
- GCC Compiler  
- `make` tool

### 🔧 Build & Run
```bash
make
./proxy <PORT>
```

Example:
```bash
./proxy 8080
```

---

## 🤝 Contributing
Pull requests and ideas are welcome!

1. Fork this repo  
2. Create a feature or fix branch  
3. Submit a PR

---

💥 **Experience raw socket programming, thread handling, and caching in C!**

---
