# Project Delta
A performance-first, **data-driven game engine** built from the ground up with a focus on cache locality, minimal overhead, and deterministic memory allocation patterns.

This project is currently a **work-in-progress,** moving away from standard library abstractions to provide a leaner more transparent development environment.

> ![NOTE]
> This engine is in an active state of flux. APIs and memory structures are subject to significant changes as the threading and allocation systems are refined.

## 🚀 Key Features
- **Zero-Overhead Data Driven Design:** Engineered around efficient data layouts to maximize CPU cache hits and minimize pipeline stalls
- **Minimal STL Dependency:** To maintain total control over binary size and memory patterns, the project avoids the STL, utilizing only essential low-level headers (e.g. \<bit\> for bit_width).
- **Hot-reloading:** Due to its unique launching design, the engine is able to hot-reload the whole game without a need to close and start the whole application again.
- **Custom Threading API:** (In-Development) A lightweight, platform-agnostic threading wrapper designed specifically for high-performance task scheduling and synchronization.

## 🧠 Memory Management
The engine treats memory as a first-class citizen. Rather than relying on general-purpose heap allocations, it implements a suite of **Custom O(1) Allocators** to eliminate latency spikes:
- **Linear/Stack Allocators:** For high-speed, frame-based temporary storage.
- **Pool Allocators:** Providing constant-time allocation and deallocation for fixed-size game objects.
- **Custom Slab Management:** Efficiently handling larger, persistent memory blocks without the unpredictability of `malloc` or `new`.

## 🛠 Tech Stack
- **Language:** C++,
- **Build System:** CMake,
- **Target Architectures:** amd64 (and arm64 being considered).

## 🏗 Roadmap
- [] Finalize Thread API and Task Scheduling.
- [] Benchmarks
- [] Implement SIMD math library integrations.
- [] Expand Memory Profiling tools for real-time heap visualization.
- [] Integrate a low-level Vulkan/Direct3D 12 rendering abstraction