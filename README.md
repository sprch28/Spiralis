# Spiralis

**A custom High-Performance utility library**

Spiralis is a C++ utility library built around a simple philosophy:

> **High abstraction and high performance should not be mutually exclusive.**

The goal of Spiralis is to provide convenient, high-level abstractions while keeping their underlying implementation accessible. If you want to use an abstraction as-is, you can. If you need to understand or control what is happening underneath it, the layers are there to peel back.

Spiralis is primarily focused on performance-sensitive programming, low-level control, custom data structures, memory management, and general-purpose utilities.

> **Status:** Active Development

> **Pre-1.0:** Spiralis should be treated with caution, especially if using in production-grade environments. APIs and implementations are still subject to change.

---

## Philosophy

Spiralis is not intended to simply recreate the C++ standard library.

This project instead asks a different question:

**How much abstraction can be provided without losing control over the machine?**

A container should be easy to use, but its memory layout should still be understandable.

A smart pointer should provide ownership semantics, but its allocation and lifetime behavior should not be a black box.

A high-level utility should be convenient, but it should not require giving up control over the lower layers when performance or specialized behavior matters.

This leads to a general design principle throughout Spiralis:

**Use the abstraction when you want it. Peel it back when you don't.**

---

## What's Included

Spiralis is divided into several components rather than being a single collection of unrelated utilities.

### Core

The `core` library contains the infrastructure that much of Spiralis is built on:

* Custom allocators
* Exception handling
* Iterators
* Pointer types
* Compile-time type traits

### Containers

Spiralis currently provides:

* `sp::array`
* `sp::bitset`
* `sp::hash_map`
* `sp::hba`
* `sp::pair`
* `sp::string`

These containers are implemented independently rather than simply wrapping their STL equivalents.

Some are also designed around use cases that standard containers do not specifically target.

For example, `sp::hba` is designed around fragmented storage and frequent erasure while still maintaining efficient access and traversal.

### Math

The math library currently contains:

* Algorithms
* Bit manipulation utilities
* Hash functions
* 128-bit integer support
* General mathematical utilities
* Random number generation

### Parallel

The parallel library currently contains:

* SIMD utilities
* Threading utilities

These components are intended to provide lower-level building blocks for parallel and performance-sensitive workloads.

### I/O

Spiralis provides custom I/O utilities, including:

* Console functionality
* General I/O utilities

### Specialized Libraries

Some parts of Spiralis are intentionally kept separate from the core library.

For example, machine-learning functionality can be enabled separately and currently includes:

* `sp::tensor`
* Tokenization utilities

This keeps specialized functionality from becoming a requirement for programs that do not need it.

---

## Performance

Performance is one of the primary considerations when designing Spiralis.

However, performance is not treated as an excuse to make the public interface difficult to use.

The intended result is:

**High-level interface -> optimized implementation -> low-level control when necessary.**

Depending on the component, Spiralis could make use of:

* Custom memory allocators
* Cache-conscious data layouts
* SIMD
* Compiler intrinsics
* Branch-prediction hints
* Prefetching
* Bit manipulation
* Architecture-specific instructions
* Custom hashing
* Parallel execution
* Low-level system APIs

The exact implementation is considered part of the design rather than something that should be hidden simply for the sake of abstraction.

Benchmarks are used throughout development to determine whether an optimization actually improves performance rather than relying solely on assumptions about what "should" be faster.

Eventually, the `benchmarks` folder will contain performance tests on containers and functions, being compared both to their STL counterparts (where applicable) and the current industry standards.

---

## Memory

Memory management is a very important part of Spiralis.

The library contains its own allocator infrastructure, which is used by higher-level components nearly everywhere.

This allows Spiralis to control more than simply *what* gets allocated. It can also control how memory is obtained, laid out, reused, and released.

The same philosophy applies here as everywhere else in the library:

You can use the higher-level abstraction, or you can work closer to the allocator when you need to.

---

## Generic Programming

Spiralis makes extensive use of templates and compile-time type inspection.

The library is designed to work with user-defined types without requiring them to inherit from Spiralis classes.

Where possible, Spiralis detects what operations a type supports and adapts its behavior accordingly.

The type-traits system is still being expanded and is not yet considered complete, but provides almost all necessary functionality. Concepts (C++20 and above) are soon to be added.

---

## Portability

Spiralis currently targets little-endian 64-bit systems.
While big-endian expansion is a possibility, it isn't a huge priority.

### macOS

Spiralis should compile cleanly on macOS.

Newer Apple Silicon systems can make use of ARM SIMD capabilities where supported.

### Linux

Spiralis currently compiles cleanly with:

* Clang
* GCC

### Windows

Spiralis currently compiles with MinGW.

MSVC compatibility is currently being added.

### Endianness

Only little-endian systems are currently supported.

Big-endian compatibility may be added in the future, but is not currently a priority.

---

## Requirements

### C++17+

Spiralis requires **C++17 or newer**.

Earlier C++ standards are not currently supported, yet older compatibility is a possible future addition.

---

## Structure

The project is organized into several libraries:

```text
Spiralis/
├── setup/
│   └── init.hpp
│
├── core/
│   ├── allocators.hpp
│   ├── exceptions.hpp
│   ├── iterator.hpp
│   ├── pointer.hpp
│   └── type_traits.hpp
│
├── io/
│   ├── console.hpp
│   └── IO.hpp
│
├── math/
│   ├── algorithm.hpp
│   ├── bit_manip.hpp
│   ├── hashes.hpp
│   ├── int128.hpp
│   ├── math.hpp
│   └── random.hpp
│
├── containers/
│   ├── array.hpp
│   ├── bitset.hpp
│   ├── hash_map.hpp
│   ├── hba.hpp
│   ├── pair.hpp
│   ├── string.hpp
│   └── tensor.hpp
│
├── parallel/
│   ├── SIMD.hpp
│   └── thread.hpp
│
├── bench/
│   └── test.hpp
│
└── ml/
│   └── tokenizer.hpp
```

Currently, the `ml` file is not automatically pulled in.
To use the tokenizer and tensor, you must define `__SP_ML__ ` before including Spiralis. This can be done in the build command via `-d`, or by defining it manually before calling `#include <Spiralis/Spiralis.hpp>`

---

## Current Limitations

Spiralis is still under development, so compatibility is intentionally limited in some areas.

Currently:

* C++17 or newer is required.
* Only little-endian systems are supported.
* macOS support is currently strongest.
* Linux support currently targets Clang and GCC.
* Windows currently requires MinGW.
* MSVC compatibility is still being developed.
* Some platform-specific functionality has not yet been generalized.
* Specialized libraries may have additional dependencies or compatibility requirements.

These limitations are expected to change as the project develops.

---

## Stability

Spiralis is currently **pre-1.0**.

This means that:

* APIs can change.
* Implementations can change.
* Compatibility can change.
* Performance characteristics can change.
* Components may be added, removed, or redesigned.

Tests and benchmarks are used heavily during development, but passing tests should not be interpreted as a guarantee that every component is production-safe.

Until V1.0, use Spiralis with caution.

---

## Roadmap

Spiralis is actively being developed.

Some of the broader goals are:

* Expand platform compatibility
* Complete MSVC support
* Continue developing the custom allocator system
* Expand type_traits coverage
* Continue optimizing existing containers
* Expand SIMD support to AVX architectures
* Expand parallel utilities
* Develop specialized libraries
* `gui` folder for quickly developing simple UIs
* Improve testing and benchmarking
* Stabilize APIs for the eventual V1.0 release

The roadmap is intentionally flexible. Spiralis is a systems project, and implementation details will change when experimentation shows a better approach.

---

## Why Spiralis?

C++ already provides high-level abstractions.

It also provides extremely low-level control.

Spiralis is an attempt to make those two things coexist.

The ideal Spiralis interface is one where you can write:

```cpp
sp::array<int> values;
```

and not have to care about how the array works.

But if you *do* care, you should be able to look underneath it.

You should be able to see the allocator, data structure, 
and lifetime management.

You should be able to optimize it, replace it, or go another layer down.

**The abstraction is there when you want it. The control is there when you need it.**

---

## License

See `LICENSE` for licensing information.

---

**Spiralis — high abstraction, high performance, and control at any level.**
