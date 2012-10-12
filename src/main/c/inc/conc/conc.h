#ifndef _LAZY_H
#define _LAZY_H

#include <stdio.h>
#include <pthread.h>

/*
  Optimized functions for accessing data shared among threads. The
  optimization lies in comparing a cache-aligned variable [1] - the
  *global version* - against a thread local variable - the *thread
  local version*. The *global version* requires to be cache-aligned to
  avoid false-sharing thereof. Only if versions mismatch will a lock
  be aquired and the thread local data synchronized with the global
  data. This pattern reduces the locking overhead significantly and is
  similar to DCL (Double-Checked Locking) [2].

  The implementation is generally applicaple as long as the following
  conditions are met:

  1) the memory pointed by *g_data (see function parameter of the same
     name in the function below) is declared <tt>volatile</tt> so that
     it is always read from main memory, see [3], [4] and [5].

  2) data is always copied either from shared to local data or vice
     versa - if the memory pointed by is not copied the data could
     suddenly be invalid due to modification by either side

  4) the cache-line size determined for the target architecture [6].
     This implementation expects a compile macro to be set, i.e.
     CACHE_LINE_SIZE. For example set it to the output of
     '/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size'

  Background information on the topic:
  [1] http://stackoverflow.com/questions/12592342/lock-free-check-for-modification-of-a-global-shared-state-in-c-using-cache-line
  [2] http://en.wikipedia.org/wiki/Double-checked_locking
  [3] http://www.drdobbs.com/parallel/volatile-vs-volatile/212701484
  [4] http://stackoverflow.com/questions/2044565/volatile-struct-semantics
  [5] http://stackoverflow.com/questions/7872175/c-volatile-variables-and-cache-memory
  [6] http://stackoverflow.com/questions/794632/programmatically-get-the-cache-line-size
*/

#if !defined(CACHE_LINE_SIZE)
#error CACHE_LINE_SIZE not set. Invoke gcc with -DCACHE_LINE_SIZE=xx
// Set a default CACHE_LINE_SIZE to avoid error printed by __aligned__
// keyword below. The compiler will nonetheless generate an error
#define CACHE_LINE_SIZE  8
#endif

/**
 * Memory aligned instances are only warranted if allocated with
 * *aligned_malloc* or similar. If statically allocated the compiler
 * alignes the data correctly.
 */
struct cache_line_aligned {
  int version;
  // char padding[60];
} __attribute__ ((aligned (CACHE_LINE_SIZE)));

/**
 * Initializes the global version to a user defined value. Defaults to
 * to <tt>0</tt>.
 */
void lazy_init(int);

/**
 * Frees resources.
 */
void lazy_destroy();

/**
 * Update the shared state. Does so by updating the global state only
 * if both, the shared and the thread local versions - <tt>t_shared</tt>
 * and <tt>g_shared</tt> - match.
 *
 * \code{.c}
 * if(t_version == g_version) {
 *   lock();
 *   // increment t_version by one
 *   // assign t_version to g_version
 *   // write global state
 *   unlock();
 *   return t_version; // incremented
 * } else {
 *   return g_version;
 * }
 * \endcode
 *
 * \param t_version  Thread local version. Compared against <tt>g_version<*
 * \param *t_data    Thread local data store
 * \param *g_data    Shared data store
 * \param *fn        Function invoked to copy thread local to globally
 *                   shared memory
 */
int lazy_update_global_if_owner(int t_version,
                                const void *t_data,
                                void *g_data,
                                void (*fn)(const void*, void*));

/**
 * A CAS like operation for updating thread local from a global
 * state. Compares thread local version <tt>t_version< against a global
 * version variable <tt>g_version<* (not exposed). If they mismatch a lock
 * is aquired and the thread local data *t_data* updated. Finally the
 * lock is released.
 *
 * <b>Intrinsics</b> Comparing the global version <tt>g_version</tt>
 * against the thread local version <tt>t_version</tt> is
 * lock-free. This reduces the locking overhead significantly if the
 * data shared among threads rarely changes. A lock is only aquired if
 * the versions mismatch and the thread local data <tt>t_data</tt> is
 * outdated and requires update from <tt>g_data</tt>. To satisfy the
 * correctness of the lock-free comparison the global version variable
 * spawns exactly one cache-line. This is assured by memory-aligning
 * and padding the version variable to the size of one
 * cache-line. This has the advantage of a) avoiding the false-sharing
 * usage pattern thus improving run-time performance, and b) avoiding
 * the sharing of invalid data (version) between multiple threads
 * because a cache-line can only be worked on by one cpu at a time.
 *
 * \param t_version  Thread local version. Compared against <tt>g_version</tt>
 * \param *t_data    Thread local data store
 * \param *g_data    Shared data store
 * \param *fn        Function invoked to copy global to thread local
 *                   memory
 *
 * \sa
 * http://stackoverflow.com/questions/12592342/lock-free-check-for-modification-of-a-global-shared-state-in-c-using-cache-line
 */
int lazy_update_local_if_changed(int t_version,
                                 void *t_data,
                                 const void *g_data,
                                 void (*fn)(void*, const void*));

#endif /* _LAZY_H */
