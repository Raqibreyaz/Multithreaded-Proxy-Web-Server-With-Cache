# 🚀 Multi-Threaded HTTP Proxy Server in C

## 🌐 What Is It?

A high-performance, multi-threaded HTTP proxy server written in **pure C** that:

* Handles multiple clients using a **thread pool**
* Caches HTTP responses using a **custom LRU strategy**
* **Filters blocked sites**, manages cache files
* **Rewrites HTML** to adapt links for the proxy path
* Is now **Docker-compatible**, with a production-ready build system

Built from scratch to learn low-level networking, multithreading, caching, HTML processing, and signal-safe server design.

---

## ✨ What’s New in This Version

* ✅ **Thread Pool** support with bounded queue — no more per-client threads
* ✅ **SIGPIPE ignored** to handle unexpected client disconnects gracefully
* ✅ **HTML rewriting engine** that injects `<base>`, rewrites `href/src` links
* ✅ **Cache invalidation** using last modified time of files
* ✅ **Filename sanitization** for cached URLs with special characters
* ✅ **Blocked site filtering** from a config file
* ✅ **Custom error responses** sent to client
* ✅ **Memory-safe HTTP parsing** and realloc-safe buffer growth
* ✅ **Docker support** for containerized deployment

---

## 🧱 Architecture Overview

### 1. 🔌 Socket & Connection Handling

* Listens on a configurable port.
* Accepts clients and **queues them** to a thread-safe `ClientQueue`.

### 2. 🧵 Thread Pool

* Fixed number of worker threads initialized at server startup.
* Threads block on condition variable until a client is enqueued.
* Each thread handles full client life-cycle (`recv`, fetch, rewrite, `send`).

### 3. 📦 Caching (LRU)

* Uses a **custom LRU cache** with linked list for eviction.
* Shared between threads — guarded by a **mutex lock**.
* Cache entries have timestamps to support **cache invalidation**.

### 4. ✏️ HTML Rewriter(In Progress)

* Injects `<base>` tag for relative path correction.
* Rewrites `src`, `href`, `action`, `poster` etc. to include `/url=?` path.
* Rewrites CSS `url(...)` inside `<style>` and attributes like `srcset`.

### 5. 🛡️ Blocked Sites Filter

* Reads a list of blocked domains from a JSON file.
* Blocks them by returning a custom error message.

---

## 🧪 Features at a Glance

| Feature                  | Description                                      |
| ------------------------ | ------------------------------------------------ |
| ✅ Multi-threaded         | Thread pool using `pthread`                      |
| ✅ Thread-safe cache      | Mutex-protected LRU cache                        |
| ✅ HTML rewriting         | Base injection + relative path fixer             |
| ✅ Docker support         | Portable, reproducible builds                    |
| ✅ Signal handling        | Ignores `SIGPIPE`, handles clean shutdown        |
| ✅ Cache invalidation     | Based on file modification timestamps            |
| ✅ Error handling         | Custom error pages                               |
| ✅ Clean modular design   | Separated concerns (server, fetch, cache, utils) |
| ✅ URL sanitization       | Cache-safe filenames                             |
| ✅ No 3rd-party libraries | Only POSIX, OpenSSL for HTTPS                    |

---

## 🚀 Getting Started

### 📦 Prerequisites

* Linux or WSL
* GCC compiler (`gcc`)
* `make` tool
* Docker (optional)

---

### 🔧 Build & Run (Native)

```bash
make
./server
```

Example:

```bash
./server
```

---

### 🐳 Run with Docker (In Progress)

#### 1. Build Docker Image

```bash
docker build -t my-proxy .
```

#### 2. Run Container

```bash
docker run -p 8080:8080 my-proxy
```

> Your proxy will now listen on port `8080` of your machine.

---

## 📁 File Structure

```bash
.
├── src/                   # Source files (server, cache, fetcher, etc.)
├── include/               # Header files
├── build/                 # Compiled output
├── blocked-sites.json     # List of blocked domains
├── Makefile               # Build system
├── Dockerfile             # For Docker builds
├── README.md              # You're reading this
└── dev-log.txt            # Maintained list of bug fixes + improvements
```

---

## 📓 Dev Log Highlights

Here are just a few of the issues solved during development:

* 🧨 Crash on `send()` when client disconnects → `SIGPIPE` ignored
* 📦 Stack smashing on unbounded copies → moved to heap + length-safe copies
* 🔐 Cache not syncing across threads → used pointer-based locks
* 📁 Special chars in URLs breaking filenames → sanitized URLs
* 🔁 Clients blocked from retrying blocked sites → JSON-based filter
* 🌐 Relative HTML links causing proxy confusion → `html-rewriter` system
* 🧠 Complexity in parsing/fetching → modular `fetch()` abstraction

> Full log is in [`dev-log.txt`](./dev-log.txt)

---

## 💡 Learning Outcomes

* Thread synchronization with mutexes and condition variables
* Memory-safe string operations in C
* Handling real-world edge cases in socket I/O
* Parsing and transforming HTML without external libs
* Designing a maintainable multi-file C project
* Dockerizing a raw C-based network service

---

## 🤝 Contributing

Pull requests and feature ideas are welcome!

1. Fork the repo
2. Create a feature or fix branch
3. Submit a PR with a clear title and description

---
