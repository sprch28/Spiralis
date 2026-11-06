# Getting familiar
* **`setup/init.hpp`** is the most important file. It would be equivalent to the `__config` file used by the STL. It has all the important global definitions. The macros have an underscore hierarchy:
    * 4 underscores is for file definitions (`#ifndef`, `#define`)
    * 3 underscores is for system detection
    * 2 underscores are macros that can be configured by the user either using #define before including Spiralis or by using -D in the build command. They dictate global behavior.
    * 1 underscore is for internal helpers.
    * 0 underscores are mostly used as aliases, and the section is pretty importnat to get familiar with. I've added comments explaining all the clang attributes I have macros for.
* Observing the files in the `core` folder is recommended:
* **`core/type_traits`** is big and not yet complete, but understanding it is simple.
    * They fall in a separate namespace: `spt`.
    * It provides all the type traits the standard library provides, as well as Spiralis-specific ones located around line 1150.
    * Claude provided the outline, which should make navigation a little easier due to the comments.
    * When working in other files and using type traits, try to use spt::`...` ideally. If not yet implemented, you can use std::`...` temporarily.
* **`core/allocators`** is important, but you only really need to look at `allocator_traits` at the top, and maybe the class `allocator` right below it.
* One common pattern in Spiralis class definitions is implementing allocator support with `template <template <typename> typename Alloc>` instead of just `template <typename alloc>`, as this allows for a cleaner public API
    * Example: the user can type `sp::array<int, 1, sp::allocator>` instead of `sp::array<int, 1, sp::allocator<int>>`, which not only eliminates some code bloat but also ensures they don't accidentally use the wrong type in the allocator.
* **`math/bit_manip.hpp`** may be worth visiting, as it has lots of bit manipulation algorithms and builtin cpu instructions wrapped for you.
* **`containers/hba.hpp`** is good to look at if you have a shit ton of free time. It's that new data structure but how it works internally is a bit difficult. `get_idx()` is a function i'm always trying to optimize, but I think it's pretty close to optimal.
---
# Patenting Idea

- Basically we focus our algorithms in Spiralis towards embedding LLMs or smaller versions like SLMs into the hardware.
- We can't patent the idea of a microcontroller or a microprocessor that comes prepackaged with an AI model, since that technically counts as prior art, we have to create our own specific algorithm and patent that. 
- (See similar patents at https://patents.google.com/patent/US20190340491A1/en and https://patents.google.com/patent/US11983630B2/en)
- Basically if we can secure something like that we could start selling our own chips for consumer use since most of the currently existing patents are owned by huge companies like Apple and Tesla.
- We can get around other consumer patent laws by dual licensing our software like we discussed before.
---
- If you have any other ideas on how we can make this more efficient technical wise, put them below this line. The idea is still kinda rough since it came to me on the toilet lmao.
---
# Tasks
### Order of their completion doesn't matter, and you can give me any tasks you don't feel comfortable with.
---
* Doxygen-style documentation comments:
* Double check the ai slop documentation of `containers/array.hpp` and fix if necessary (I'm sure dumbass copilot messed up further down in the file).
* `hash_map` documentation
* `pair` documentation (takes like 2 minutes)
* `string` documentation
* `tensor` documentation if possible
    * May be a bit more tricky because macros are used to define math functions quicker.
---
* Any optimizations/new allocators are welcome in **`core/allocators.hpp`**
    * Add public function `static constexpr size_type get_alignment()` to any new allocators if you make them. Return 0 if it gurantees no special alignment (such as 64-byte)
    * All allocators inherit from `_spiral_alloc_traits`, as it provides special conditions such as no need for construction when the data type is trivially copyable (bypassing calling `construct()` at all), and more will be added later.
* In **`core/pointer.hpp`**, we still need to implement `weak_ptr` if you're familiar with it.
    * My pointer is very similar to the STL, with minor internal tweaks to the data layouts (`shared_ptr` is only 8 bytes instead of 16 and my pointers are compatible with custom allocators)
    * The `control_block` struct is defined in the public section of `shared_ptr`.
    * I don't belive it will be necessary, but you can modify `shared_ptr` if it's behaving weird with `weak_ptr`.
    * `weak_ptr` will need to be capable of destroying the control block, as `shared_ptr` will leave the block alone if it has `weak_count > 0`
    * You can look at `reset` in the public section of `shared_ptr` to understand how freeing the control block should work using Spiralis allocator traits.
* In **`math/hashes.hpp`**, you can see how I implemented `basic_hash` to make it compatible with lots of types. Adding more compatibility is welcome if you see any gaps, but also take note of the use of `SP_HAS_METHOD()` with `c_str()` and `size()`. If you know of any good hashes, adding new hashes would be appreciated.
    * `SP_HAS_METHOD()` is useful everywhere for compatibility with unknown containers, such as STL containers or even custom-made ones. As long as the target container has the method, Spiralis can work with it.
    * If you want to check for a new method and the compiler is throwing, you need to go to line ~1234 of **`core/type_traits.hpp`** and define a new method checker using the macro `SP_DEFINE_METHOD_CHECKER(method_name)`
    * If making new hashes, you can just define them before `basic_hash` because of the big comment I have right now.