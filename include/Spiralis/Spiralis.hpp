#include "setup/init.hpp"

#include "core/allocators.hpp"
#include "core/exceptions.hpp"
#include "core/iterator.hpp"
#include "core/type_traits.hpp"

#include "io/console.hpp"
#include "io/IO.hpp"

#include "math/algorithm.hpp"
#include "math/bit_manip.hpp"
#include "math/hashes.hpp"
#include "math/int128.hpp"
#include "math/math.hpp"
#include "math/random.hpp"

#include "containers/array.hpp"
#include "containers/bitset.hpp"
#include "containers/hash_map.hpp"
#include "containers/hba.hpp"
#include "containers/pair.hpp"
#include "containers/string.hpp"
#if defined( __MACH__) && defined (__APPLE__)
    #include "containers/timer.hpp"
#endif

#if defined(_POSIX_THREADS) && (_POSIX_THREADS > 0) // SIMD uses thread, so both must require posix
    #include "parallel/SIMD.hpp"
    #include "parallel/thread.hpp"
#endif

#include "ml/tokenizer.hpp"