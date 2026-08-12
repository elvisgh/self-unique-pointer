# SimpleUniquePointer

A minimal C++17 unique pointer. Header-only, zero external dependencies.

## Features

- **Move-only** ownership semantics — copy constructor/assignment deleted at compile time
- RAII destruction, `noexcept` move operations
- `reset()` / `reset(T*)`, `release()`, `swap()`
- `operator*`, `operator->`, `explicit operator bool`
- No external dependencies (pure standard library, no pthread)

## Quick Start

```cpp
#include "unique_pointer.h"
#include <cstdio>

struct Item { int id; };

int main() {
    UniquePointer<Item> p(new Item{42});
    printf("id = %d\n", p->id);

    UniquePointer<Item> q = std::move(p);   // ownership transferred, p is null
    p.reset();                               // destroy Item

    Item* raw = q.release();                 // surrender ownership
    delete raw;                              // caller's responsibility now
    return 0;
}
```

## Build & Test

```bash
cmake -S . -B build -DSIMPLE_UNIQUE_POINTER_BUILD_BENCHMARKS=ON
cmake --build build -j
./build/benchmarks/benchmark_unique_pointer
```

## Design Notes

- Copy semantics are `= delete`d, so misuse fails at compile time, not runtime
- Move operations are `noexcept` → the type stays usable in `std::vector`, `std::swap`, etc.
- `reset()` adopts the new pointer *before* deleting the old one → self-reset is safe
- Test suite (15 cases) covers move invariants, self-move-assignment, `release()`/`swap()`,
  batch lifecycle in `std::vector`, and compile-time copy-deletion checks

## License

MIT
