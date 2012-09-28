#ifndef _LAZY_H
#define _LAZY_H

#include <string.h>

/*
  Optimized functions for accessing data shared among threads. The
  optimization lies in checking a cache-aligned variable - the *global
  version* - against a thread local variable - the *thread local
  version*. The *global version* requires to be cache-aligned to avoid
  false-sharing thereof. Only if versions mismatch will a lock be
  aquired and the thread local data synchronized with the global
  data. This pattern reduces the locking overhead significantly and is
  similar to DCL (Double-Checked Locking).

  The implementation is specific to Wineing but can easily be
  generalized as long as the following conditions are met:

  1) the memory pointed by *g (see function parameter of the same name
     in the function below) is freed and re-allocated on every write
     operation (memory fencing)

  2) data is always copied either from shared to local data or vice
     versa - if the memory pointed by is not copied the data could
     suddenly be invalid due to modification by either side

  3) the instance of the shared data is global

  4) the cache-line size is provided before compiling, e.g. by setting
     CACHE_LINE_SIZE to e.g. the output of
     '/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size'

  Background information on the topic:
  - http://stackoverflow.com/questions/794632/programmatically-get-the-cache-line-size
  -
  http://stackoverflow.com/questions/12592342/lock-free-check-for-modification-of-a-global-shared-state-in-c-using-cache-line
*/

/**
 * See below for comments on what these functions exactly do.
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
 * will make sure the data is correctly alligned.
 */
struct cache_line_aligned {
  int version;
  // char padding[60];
} __attribute__ ((aligned (CACHE_LINE_SIZE)));

/**
 * Initialize version with 0. A statically allocated instance of a
 * struct is always going to be aligned.
 */
static cache_line_aligned g_version = {0};

/**
 * Lock aquired to either, copy from shared to local memory, or local
 * to shared memory.
 */
static pthread_mutex_t    g_lock;

/**
 * Initializes the global version to a user defined value. Defaults to
 * to <tt>0</tt>.
 */
void lazy_init(int initial_version = 0)
{
  g_version.version = initial_version;
  pthread_mutex_init(&g_lock, NULL);
}

/**
 * Frees resources.
 */
void lazy_destroy()
{
  pthread_mutex_destroy(&g_lock);
}

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
inline int lazy_update_global_if_owner(int t_version,
                                       const void *t_data,
                                       void *g_data,
                                       void (*fn)(const void*, void*))
{
  // Comparing against g_version without aquiring lock is only
  // safe because it is cache-aligned thus avoiding sharing of
  // partial data (false-sharing)
  if(t_version == g_version.version) {
    // Enter critical section
    pthread_mutex_lock(&g_lock);

    // It is required to update g_version within the critical section
    // because otherwise another thread might aquire the lock while
    // assigning the value, thus leading to an inconsistency in the
    // data.
    g_version.version = ++t_version;

    // Copy the data
    fn(t_data, g_data);

    // Leave critical section
    pthread_mutex_unlock(&g_lock);

    // Return the new version
    return t_version;

  } else {
    return g_version.version;

  }
}

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

inline int lazy_update_local_if_changed(int t_version,
                                        void *t_data,
                                        const void *g_data,
                                        void (*fn)(void*, const void*))
{
  // False-sharing of g_msg_version can only be avoided if the
  // variable spawns full cache-lines, in this case exactly
  // one. Satisfied by memory-aligning and padding the struct to
  // cache-line size. See stackoverflow article
  // http://stackoverflow.com/questions/12592342/lock-free-check-for-modification-of-a-global-shared-state-in-c-using-cache-line
  if(t_version == g_version.version) {
    // Nothing to do if versions match
    return t_version;

  } else {
    // Enter critical section
    pthread_mutex_lock(&g_lock);
    {
      // Local version requires update within the locked area so that
      // it is update with a possibly illegal value. The value would
      // be illegal if another thread updates the variable before this
      // thread enters the critical section.
      t_version = g_version.version;

      // Copy the data
      fn(t_data, g_data);
    }

    // Leave critical section
    pthread_mutex_unlock(&g_lock);
    return t_version;
  }
}

#endif /* _LAZY_H */
