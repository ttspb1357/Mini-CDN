# Mini CDN — HTTP Server with Edge Caching

A fully functional Content Delivery Network built from scratch in C++ using raw POSIX sockets, multithreading, and HTTP/1.1 protocol implementation.

## Overview

This project demonstrates how real CDNs (Cloudflare, AWS CloudFront, Akamai) work by implementing:
- **Origin Server** (Port 8080) — The master data store
- **Edge Servers** (Ports 8081, 8082) — Cached copies with intelligent fallback to origin
- **Caching Logic** — Smart cache management with cache hit/miss handling
- **Concurrent Architecture** — Multithreading for handling multiple simultaneous clients

## Features

**Built from Scratch** — No web frameworks. Raw POSIX sockets, HTTP protocol, and C++ threading  
**HTTP/1.1 Implementation** — Manual request parsing and response building  
**Concurrent Clients** — Multithreading with std::thread for simultaneous connections  
**Edge Caching** — Intelligent cache on edge servers with origin fallback on cache miss  
**Content-Type Detection** — Automatic MIME type detection (.html, .css, .js, .png, .jpg, etc.)  
**Dynamic File Serving** — Serve different files based on requested path  
**Error Handling** — Proper HTTP error responses (404, 403, 500)  
**Security** — Path traversal attack prevention  
**Frontend** — HTML, CSS, JavaScript with responsive design  

## Architecture
```text
Browser Request
↓
Edge Server (8081 or 8082)
├─ Check local cache
│ ├─ Cache hit? → Serve from cache (milliseconds) ✓
│ └─ Cache miss? → Connect to origin
│
Origin Server (8080)
└─ Read from ./public
└─ Send response back to edge
└─ Edge caches file
└─ Edge serves to browser
```


## Technical Details

| Aspect | Technology |
|--------|-----------|
| **Language** | C++ |
| **Network** | POSIX Sockets (raw TCP) |
| **Protocol** | HTTP/1.1 (manually implemented) |
| **Concurrency** | C++ std::thread (multithreading) |
| **Compilation** | g++ on Linux/macOS |
| **Frontend** | HTML, CSS, JavaScript |

## Project Structure
```text
http_server/
├── src/
│ └── main.cpp # All server logic
├── public/
│ ├── index.html # Landing page
│ ├── about.html # Technical details
│ ├── contact.html # Getting started
│ ├── style.css # Styling
│ └── script.js # Frontend interactivity
├── cache_asia/ # Edge Asia cache (auto-created)
├── cache_europe/ # Edge Europe cache (auto-created)
├── README.md # This file
├── .gitignore
└── server # Compiled executable
```

## Requirements

- **OS:** Linux or macOS (or Windows with WSL)
- **Compiler:** g++ (C++11 or later)
- **Terminals:** 4+ (one for each server + testing)

## Compilation

```bash
cd http_server
g++ -o server src/main.cpp
```

Output: Executable file named `server`

## Running the Project

Open **4 separate terminal windows** in the http_server directory.

**Terminal 1 — Origin Server (Port 8080)**
```bash
./server 8080
```
Output:

Starting server on port 8080 as origin
Socket created successfully
Socket bound to port 8080
Server listening on port 8080...


**Terminal 2 — Edge Asia (Port 8081)**
```bash
./server 8081
```
Output:

Starting server on port 8081 as edge_asia
Server listening on port 8081...


**Terminal 3 — Edge Europe (Port 8082)**
```bash
./server 8082
```
Output:

Starting server on port 8082 as edge_europe
Server listening on port 8082...


**Terminal 4 — Testing**

Visit in browser or use curl (see below).

## Testing

### Test 1: Visit in Browser

Open these URLs in your browser:

- **Origin Server:** `localhost:8080/`
- **Edge Asia:** `localhost:8081/`
- **Edge Europe:** `localhost:8082/`

### Test 2: Command Line with curl

**First Request (Cache Miss):**
```bash
curl localhost:8081/
```

Check Terminal 2 output:

[edge_asia] Cache miss: / - fetching from origin
[edge_asia] Cached: /
[edge_asia] Response sent


**Second Request (Cache Hit):**
```bash
curl localhost:8081/
```

Check Terminal 2 output:

[edge_asia] Cache hit: /
[edge_asia] Response sent


Much faster — file served from cache!

### Test 3: Different Files

```bash
curl localhost:8081/about.html      # Cache miss first time
curl localhost:8081/about.html      # Cache hit second time

curl localhost:8082/contact.html    # Different edge server
curl localhost:8081/style.css       # CSS file
```

### Test 4: Non-existent Files

```bash
curl localhost:8081/notexist.html
```

Returns 404 Not Found, **NOT cached** (only 200 OK responses are cached).

### Test 5: Multiple Concurrent Requests

Terminal 4:
```bash
(curl localhost:8081/ & curl localhost:8082/ &)
```

Both edge servers handle requests simultaneously.

## Key Learning Outcomes

This project teaches:

- **Socket Programming** — Raw TCP sockets, client/server architecture
- **HTTP Protocol** — Request/response format, headers, status codes
- **Concurrency** — Multithreading, thread safety, concurrent request handling
- **Caching** — Cache hit/miss logic, when to cache, when to fetch
- **Systems Design** — Master-replica pattern, edge computing, CDN concepts
- **Networking** — Client connecting to server, inter-server communication
- **Full Stack** — Backend (C++ server) + Frontend (HTML/CSS/JS)

## Future Improvements

- Cache expiration (TTL — Time To Live)
- Cache invalidation when origin files change
- Metrics collection (hit rate, latency, throughput)
- Multiple origin servers
- Load balancing between edges
- Gzip compression
- Persistent cache across restarts
- HTTPS/TLS support

## Real-World Applications

This mini-CDN demonstrates concepts used by:
- **Cloudflare** — Global CDN with 200+ edge locations
- **AWS CloudFront** — Amazon's content delivery service
- **Akamai** — Enterprise CDN for streaming and web acceleration

## Author Notes

Built to understand how real CDNs work at the system level. This project started with raw sockets and HTTP protocol fundamentals, then evolved into a complete edge caching system with a professional frontend.

Not using any frameworks, builds genuine understanding of how the internet actually works.
