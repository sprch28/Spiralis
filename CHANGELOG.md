# Changelog

## [Unreleased]

### Performance — `include/Spiralis/containers/tensor.hpp` & `include/Spiralis/container-funcs/tensor-funcs.hpp`

I added a new constructor in the tensor file and edited the functions to use it. 
This does in fact make them a lot more efficient it actually saves us a shit ton of memset size and
lets us function with memory bandwidth at much larger sizes.

#### Changed
- Added private `_uninit_tag` sentinel and a corresponding flat uninitialized constructor to `tensor` — allocates and sets up 1D shape/stride metadata without zero-initializing the data buffer
- `arange(start, stop, step)` — replaced `tensor(size)` (allocate + memset) with `tensor(_uninit_tag{}, size)` (allocate only); eliminates a redundant write pass over the entire buffer
- `arange(stop)` — same change as above
- `linspace(start, stop, steps)` — same change as above

---

### Added — `include/Spiralis/containers/array.hpp`

#### New functions
- `binary_search(value)` — O(log n) binary search on a sorted array; returns the index of a matching element or `size()` if not found
- `binary_search_by(value, comp)` — same as `binary_search` but uses a custom strict-weak-ordering comparator
- `lower_bound(value)` — returns the index of the first element not less than `value`; array must be sorted ascending
- `lower_bound_by(value, comp)` — same as `lower_bound` but uses a custom comparator
- `upper_bound(value, sort_first=false)` — returns the index of the first element greater than `value`; optional `sort_first` parameter sorts the array before searching
- `upper_bound_by(value, comp, sort_first=false)` — same as `upper_bound` but uses a custom comparator
- `transform_(func)` — applies `func` to every element in-place, replacing each with `func(element)`; returns `*this` for chaining
- `transform_to<U>(func)` — applies `func` to every element and returns a new `array<U>` of the same size; `U` is the result type

---
### Added — `include/Spiralis/containers/hash_map.hpp`

I added a few new functions to speed things up a bit, and replaced old functions with these ones 
wherever it wasn't efficient enough. Everything should still work as intended though.

#### New functions
- `get_or_insert_with(key, callable)` / `get_or_insert_with_as(key, callable)` — returns the existing value for `key` if present; otherwise invokes `callable()` exactly once to construct the value, inserts it, and returns a reference to it. Callable is never invoked on a hit.
- `merge(other)` — inserts all entries from `other` into `*this`, skipping keys that already exist. Returns the number of entries actually inserted.
- `merge_into(other)` — moves all entries from `*this` into `other`, skipping keys already present in `other`. Trivially-copyable entries are copied; non-trivial entries are move-assigned. Clears `*this` after the operation. Returns the number of entries actually moved.

#### Documentation
- `get_or_insert_with_as` (const & rvalue overloads) — `@tparam`, `@param key`, `@param callable`, `@return` tags; rvalue overload uses `@copydoc`
- `get_or_insert_with` (const & rvalue overloads) — `@param` / `@return` tags; rvalue overload uses `@copydoc`
- `merge` — `@param other`, `@return` tags
- `merge_into` — `@param other`, `@return` tags; `@note` on trivial vs non-trivial move behaviour

---


### Documentation — `include/Spiralis/containers/pair.hpp`

#### Added
- Class-level `@tparam T1`/`@tparam T2` doc
- `first`/`second` members — `///<` inline docs
- Default constructor — `@brief`
- Forwarding constructor — `@brief`, `@tparam U1`/`@tparam U2`
- `operator<<` — `@brief`, `@return`

---

### Fixes — `include/Spiralis/containers/array.hpp`

New update: as of 10/14/2026 I fixed some more issues in the array file, mainly that in some places it was calling `predicate(_data[i-1])` instead of `_data[i-1] == value'`so it should work a bit better now.
Also there was some issues with the function returning the wrong data value so I fixed those
Double check these once or twice though

#### Fixed
- `find_last` — tail loop and `#else` path called `predicate(_data[i-1])` instead of `_data[i-1] == value`; corrected to value comparison (code bug)
- `dedupe_consecutive_` — return type in signature was `array<T, _safety_level>` (value) instead of `array<T, _safety_level>&` (reference); corrected; `@return` doc updated to match
- `shrink_by` — was calling `_SP_APPLY_UNROLLED_(_size, destroy(...))` which destroyed elements from index 0 instead of from `_size - amount`; corrected to `_SP_EXPLICIT_UNROLLED_(i, _size - amount, _size, destroy(...))`; `@brief` updated to clarify it removes from the back

---

### Documentation & Changes — `include/Spiralis/math/hashes.hpp`

I added a few new hashes that seemed sensible to me and wrote the Doxygen docs for them, but I'm
still pretty new to ts and claude was used quite a bit so double check the validity of these rq.

#### Added
- `detail::bit_cast_hash` — Doxygen explaining the type-pun helper and why it avoids UB
- `basic_hash` class — expanded class-level doc listing all dispatched types and their strategies
- `basic_hash::operator()` — `@tparam`, `@param`, `@return` tags
- `basic_hash::____private_string_hash` — `@param` / `@return` tags
- `identity_hash` — class doc with `@warning` about clustered-key risk; `@param` / `@return` on `operator()`
- `fnv1a_hash` — class doc; separate `@return` on the byte-buffer overload; dispatch note on the generic overload
- `murmur3_hash` — class doc; `fmix64` private helper doc; `@return` on both `operator()` overloads
- `wyhash` — class doc; docs on all four private helpers (`_wymix`, `_wyr8`, `_wyr4`, `_wyr3`); `///<` inline docs on the four secret constants; `@return` on both `operator()` overloads

#### Fixed
- `basic_hash::operator()` — was missing `const`; added
- `basic_hash` — `bool`, `char`/`signed char`/`unsigned char`, `short`/`unsigned short` were falling through to `value=0`; now each gets a Fibonacci multiplicative hash
- `basic_hash` — `long`/`unsigned long` branch added (distinct from `long long` on MSVC)
- `basic_hash` — `float`/`double` were multiplied as floating-point values (wrong type, NaN/±0 issues); now bit-reinterpreted via `detail::bit_cast_hash` before hashing; `-0.0` canonicalised to `0`
- `basic_hash` — pointer types were falling through to `value=0`; now strip 3 low alignment bits then Fibonacci-multiply
- Added four new hash functors: `identity_hash`, `fnv1a_hash`, `murmur3_hash`, `wyhash`

---

### Documentation — `include/Spiralis/containers/array.hpp`

The `array.hpp` file had a shit ton of ai slop and blatantly wrong info that I had to go through and fix up the accuracy. 
Full disclosure I still got claude to write most of this but I did double check it so it should be fine.

#### Added
- `is_aligned` — added missing Doxygen comment documenting that it returns true when the allocator provides cache-line alignment
- `access_and_prefetch` (both overloads) — added missing Doxygen comments explaining that the prefetch is a no-op when the allocator does not provide alignment
- `D()` / `D() const` — added missing Doxygen comments identifying them as shorthand aliases for `data()`
- `operator bool` — added missing `@return` tag (`true` if non-empty, `false` otherwise)
- `sort_by` — added missing `@return` tag and `@throws` tag

#### Fixed
- `reserve_extra` — corrected vague `@brief` ("Reserve capacity for the array") to accurately describe that it extends capacity by an exact additional amount with no rounding
- `reserve_extra_rounded` — was identical to `reserve_extra`; corrected `@brief` and `@param` to describe that the additional amount is rounded up to the next power of 2
- `const begin()` / `const end()` overloads — replaced stale `// in array.hpp` inline comment with proper Doxygen `@brief` and `@return` tags
- `push_front` — `@brief` was copy-pasted from `emplace_front`; corrected to describe pushing a value by copy/move
- `clear_and_resize` — `@brief` omitted the destructive clear behaviour; corrected to describe that all existing elements are destroyed before new ones are constructed, and added `@note` about reallocation to `next_pow2(new_size)`
- `equals` (non-safety-template overload near `operator==`) — improved `@brief` to "element-wise equal" and added `@note` documenting the `memcmp` fast path for trivially-copyable types
- `erase_swap` — `@brief` was identical to `erase`; corrected to describe the swap-with-last O(1) semantics and that order is not preserved; added `@throws` tag
- `swap_elements` — `@param a` was described as "index of the mid element" (copy-paste error); corrected to "index of the first element"; added `@throws` tag
- `reverse` — `@return` contained only the return type (`array<T, _safety_level>`), not a description; corrected to "new array containing the elements of this array in reverse order"
- `reverse_` — `@brief` was identical to `reverse`; corrected to state the operation is performed in-place
- `find` — `@brief` said "Find the mid occurrence" (copy-paste error); corrected to "Find the first occurrence"
- `find_if` — `@brief` said "Find the mid element" (copy-paste error); corrected to "Find the first element that satisfies the predicate"
- `for_each` — `@warning` contained a typo ("refrenced"); corrected to "reference"
- `remove_mid` — `@brief` said "Remove the mid occurrence" (copy-paste error); corrected to "Remove the first occurrence" to match the `find()` call in the implementation
- `find_nth` — `@param n` was documented as 0-based; corrected to 1-based to match the implementation (`count` starts at 1 and matches when `count == n`)
- `find_nth_if` — same 0-based/1-based mismatch as `find_nth`; corrected to 1-based
- `stable_partition` — `@brief` contained a typo ("pivolt"); corrected to "pivot"; added `@note` describing ordering semantics and `@throws` tag

---

### Added & Documented — `include/Spiralis/core/pointer.hpp`

Also, I added the weak_ptr stuff we were talking about but mostly used claude for this so again, double check the validity
It might have made some ai slop.

#### Added
- `weak_ptr<T, Alloc>` — non-owning observer of a `shared_ptr`-managed object
  - Increments/decrements `control_block::weak_count` on construction/destruction
  - `lock()` — returns a valid `shared_ptr` if the object is still alive, otherwise null
  - `expired()` — returns `true` if the strong count has already hit zero
  - `use_count()` — returns the current strong reference count
  - `reset()` — releases the weak reference; frees the control block if both counts are zero
  - Full copy/move constructors and assignment operators
  - `operator bool`, `operator==`/`!=` against `nullptr`, `swap()`
- `make_weak(const shared_ptr<T,Alloc>&)` — convenience factory mirroring `make_shared`
- `friend class weak_ptr<T, Alloc>` added inside `shared_ptr` so `lock()` can call the private `shared_ptr(control_block*)` constructor
- Forward declaration of `weak_ptr` added alongside the existing `shared_ptr` forward declaration

#### Documentation
- `ptr<T, Alloc>` — class-level doc, all public members, both `make_ptr` overloads
- `shared_ptr<T, Alloc>` — class-level doc, `control_block` struct and its members (`strong_count`, `weak_count`, `alloc`, `object()`), all public members, both `make_shared` overloads
- `weak_ptr<T, Alloc>` — class-level doc, all public members, `make_weak`
