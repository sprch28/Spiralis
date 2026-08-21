#ifndef ____SPIRALIS_HPP____
#define ____SPIRALIS_HPP____

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
#if defined(_POSIX_THREADS) && (_POSIX_THREADS > 0)
    #include "numeric/SIMD.hpp" // uses thread.hpp
#endif
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
#if defined( __MACH__) && defined (__APPLE__)
    #include "system/highrestimer.hpp"
#endif
#if defined(_POSIX_THREADS) && (_POSIX_THREADS > 0)
    #include "system/thread.hpp"
#endif

#endif // ____SPIRALIS_HPP____