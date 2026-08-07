#ifndef ____SPIRALIS_CPP____
#define ____SPIRALIS_CPP____

// Setup & Environment
#include "setup/init.hpp"

// Core Primitives
#include "core/allocators.hpp"
#include "core/exceptions.hpp"
#include "core/IO.hpp"
#include "core/pair.hpp"
#include "core/type_traits.hpp"

// Math & Low-Level Numeric
#include "numeric/bit_manip.hpp"
#include "numeric/bitset.hpp"
#include "numeric/hashes.hpp"
#include "numeric/math.hpp"
#include "numeric/SIMD.hpp"
#include "numeric/int128.hpp"

// Algorithms & Iteration
#include "algorithms/algorithm.hpp"
#include "algorithms/iterator.hpp"
#include "algorithms/random.hpp"

// Collections (Data Structures)
#include "collections/array.hpp"
#include "collections/hash_map.hpp"
#include "collections/hba.hpp"
#include "collections/string.hpp"

// OS & System Abstractions
#include "system/console.hpp"
#include "system/highrestimer.hpp"
#include "system/thread.hpp"

#include "asm/sp_asm.hpp"

#endif // ____SPIRALIS_CPP____