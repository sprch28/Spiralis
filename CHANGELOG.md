# Changelog

## [Unreleased]

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
