There are several conventions to follow:

* Always use Hammer C wrappers around native types: hm_nint instead of size_t, hm_uint8 instead of uint8_t etc.
  This allows to have a future-proof abstraction layer for platform-specific data types.
* Always sort struct fields from larger to smaller values: for example, first pointers (32 or 64 bit), then
  integers, then booleans etc. This can help the compiler to better pack values in memory without "holes" due to
  misalignment.
* Never assume an allocating strategy and instead allow a user select an appropriate allocator at runtime when
  instantiating an object.
* Where implementations may differ, use interfaces: a structure which lists function pointers for methods (where
  the first parameter is always "this") and a `data` struct field to hold object-specific data.
* Always validate arguments which are accepted from outside, but do not check arguments which belong to the
  object itself, for example "this" pointer in interface implementations or the `data` field (a tradeoff between
  speed and correctness).
* Cover everything with tests.
* Never use global state: it should be possible to create as many runtimes per process as one wishes.
* Always place a copyright header at the top of every file.
* For interfaces, implement easy-to-use wrappers (which deal with selecting the function pointer and passing "this" to it).
  Always validate arguments in the implementations themselves, not the wrappers. Such pointers should be in the same
  translation unit for possible inlining.
* By default, ownership belongs to whoever created an object. Never try to dispose of objects you do not own.
  If you take a reference to an object you do not own, pay attention to its lifetime to avoid using disposed objects.
* Try disposing of objects in destructors to the maximum, combining various potential errors with hmMergeErrors.
  This way we hopefully reduce memory leaks to the minimum in case of unpredictable errors in object disposal code paths.
* Mark ownership with the following idioms: HM_OWNED, HM_MOVED, HM_UNOWNED (see)
* Static functions should come after public functions.
* All objects are thread-unsafe and move-only by default, unless stated otherwise.
* All integer operations (addition, multiplication, subtraction, division) must be done with safe math functions as
  described in <core/math.h>, to avoid overflows/underflows. An exception to the rule is iteration variables (as they are bounded).
  Sometimes logic can guarantee a value will never practically overflow -- in that case, a comment must be written to
  explain why no safe math function is used.
* Priorities: safety > simplicity > performance.

Ideas:
* Since it's a request-based runtime (request=>response, with the on-demand runtime instances created/destroyed on each response),
  the idea is that to speed up 80% requests we can allocate all user objects with a bump pointer allocator for the first N megabytes,
  and then switch to the slower system allocator once the bump pointer allocator is full. Bump pointer allocator segments
  can be put in a pool and shared by several new runtimes instances (same for class metadata, with copy-on-write).