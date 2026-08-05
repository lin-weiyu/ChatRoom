# ChatRoom

A multi-client chat room server built from scratch in C++ — learning network programming and full-stack fundamentals.

## Features (Planned)

- Multi-client TCP chat server
- Custom application protocol
- Chat rooms / channels
- Concurrent connection handling (epoll)
- Web frontend (WebSocket)

## Build

### Requirements

- C++17
- CMake >= 3.16
- GTest

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

### Run Tests

```bash
ctest --test-dir build --output-on-failure
```

## Technologies Used

- C++17
- CMake
- Linux Socket API
- epoll
- GoogleTest
- GitHub Actions (CI)
