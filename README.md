# MiniHTTP

A lightweight **HTTP/1.1** server built from scratch using the [muduo](https://github.com/chenshuo/muduo) network library (C++11).  
**No third-party HTTP libraries** — every byte of HTTP parsing is hand-written.

---

## Features

- **Custom HTTP Parser** — 4-state machine (`ExpectRequestLine → ExpectHeaders → ExpectBody → GotCompleteRequest`) that processes data byte by byte from `muduo::Buffer`
- **URL Router** — `std::map`-based route table with separate GET/POST handlers
- **Static File Serving** — Automatic MIME type detection (40+ types), directory traversal protection (`/../` → 403), file size limit (100 MB → 413)
- **Keep-Alive** — HTTP/1.1 persistent connections with automatic pipeline parsing
- **Multi-threaded** — Uses `EventLoopThreadPool` (one loop per thread), configurable via command line
- **Security** — Rejects directory traversal, validates file existence, handles partial TCP packets gracefully

---

## Architecture

```
Client Request
     │
     ▼
┌─────────────┐
│  TcpServer  │  (muduo — accept connection)
└──────┬──────┘
       │ onMessage callback
       ▼
┌─────────────┐
│ HttpRequest │  (hand-written parser — state machine)
└──────┬──────┘
       │ parsed
       ▼
┌─────────────┐
│   Router    │  (URL → handler mapping)
└──────┬──────┘
       │ handler found
       ▼
┌──────────────────┐
│ StaticFileHandler │  (file → MIME → content → response)
└──────┬───────────┘
       │
       ▼
┌──────────────┐
│ HttpResponse  │  (status line + headers + body)
└──────┬───────┘
       │ appended to Buffer
       ▼
   conn->send()  →  Client receives HTTP response
```

### Source Layout

```
http-server/
├── CMakeLists.txt              Build configuration
├── main.cc                     Entry point & route registration
├── http/
│   ├── HttpRequest.h / .cc     HTTP request parser (state machine)
│   ├── HttpResponse.h / .cc    HTTP response builder
│   ├── Router.h                URL routing table
│   └── HttpServer.h / .cc      Main framework (integrates all layers)
├── handler/
│   └── StaticFileHandler.h / .cc   Static file serving + MIME types
└── www/
    ├── index.html              Demo page
    └── style.css               Demo stylesheet
```

---

## Quick Start

### Prerequisites

- **Linux** (kernel ≥ 2.6.28) or WSL2
- **GCC** ≥ 4.8 (C++11) or Clang ≥ 3.5
- **CMake** ≥ 2.6
- **Boost** (headers only)
- **muduo** compiled and installed (see [muduo-tutorial](https://github.com/chenshuo/muduo-tutorial))

### Build

```bash
cd http-server
mkdir build && cd build
cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ..
make -j$(nproc)
```

### Run

```bash
# Start the server (port 8080, 4 threads, document root ./www)
./bin/http-server 8080 4 ../www &

# Test
curl -v http://localhost:8080/
curl http://localhost:8080/health
curl -I http://localhost:8080/style.css

# Stress test
ab -n 1000 -c 10 http://localhost:8080/
```

### Endpoints

| Method | Path            | Description              |
|--------|-----------------|--------------------------|
| GET    | `/`             | Static index.html        |
| GET    | `/about`        | Server information       |
| GET    | `/health`       | Health check (JSON)      |
| GET    | `/style.css`    | CSS file (MIME: text/css)|
| GET    | `/nonexistent`  | 404 demo                 |
| GET    | `/../etc/passwd`| Security test → 403      |

---

## HTTP Parser Design

The parser is a **4-state finite state machine** that reads from `muduo::Buffer`:

```
kExpectRequestLine
    │  findCRLF() → parse "GET /path HTTP/1.1"
    ▼
kExpectHeaders
    │  findCRLF() → parse "Host: ..." etc.
    │  empty line (CRLF only) → done
    ▼
kExpectBody (if Content-Length > 0)
    │  wait for `Content-Length` bytes
    ▼
kGotCompleteRequest → ready for routing
```

After a request is processed, `reset()` returns to `kExpectRequestLine` for the next request on the same connection (Keep-Alive).

---

## Security

- **Directory traversal protection**: requests containing `..` are rejected with `403 Forbidden`
- **File type validation**: only regular files are served (symlinks, directories → 403)
- **File size limit**: files exceeding 100 MB return `413 Payload Too Large`
- **Safe file opening**: `std::ifstream` with binary mode, checked for success

---

## Built With

- [muduo](https://github.com/chenshuo/muduo) — Reactor-based C++ network library
- C++11 — `std::function`, `std::bind`, `std::map`, `std::unique_ptr`
- Boost — `boost::any` (for per-connection context)

---

## License

MIT License — feel free to use, modify, and distribute.
