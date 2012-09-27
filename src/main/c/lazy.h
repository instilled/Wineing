
#ifndef _LAZY_H
#define _LAZY_H

/*
  ======================================================================

  Optimized functions for accessing data shared among threads. The
  optimization lies in checking a cache-aligned variable - the *global
  version* - against a thread local variable - the *thread local
  version*. The *global version* requires to be cache-aligned to avoid
  false-sharing thereof. Only both versions are not equal will a lock
  be aquired and the thread local data synchronized with the global
  data. This pattern reduces the locking overhead significantly and is
  very similar to DCL (Double-Checked Locking).

  Thi implementation is specific to Wineing but can easily be
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

  ======================================================================
*/

#define CACHE_LINE_SIZE  64

/**
 * See below for comments on what these functions exactly do.
 */
#ifndef CACHE_LINE_SIZE
#error CACHE_LINE_SIZE not set
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
 * \struct
 *
 * Used for signalling and passing data from one thread to
 * another. This requires special synchronization to avoid
 * false-sharing and other nasty multi-threading issues.
 */
typedef struct
{
  int cmd;
  char *data;
  size_t size;
} w_shared;

/**
 * Initialize version with 0. Because it's statically allocated it is
 * aligned.
 */
static cache_line_aligned g_version = {0};

static pthread_mutex_t    g_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * Update the shared state. Does so by either updating the global
 * state only if both the shared and the thread local versions match,
 * - *force == 0* - or by strictly overwriting the shared state -
 * *force == 1*. Before any write operation a lock is aquired. Later
 * the memory pointed by the global state pointer is freed and
 * re-allocated, the new global state written, and finally the lock
 * released.
 */
inline int update_global_if_owner_or_forced(int t_version,
                                            w_shared *t_data,
                                            w_shared *g_data,
                                            int force)
{
  if(force == 0) {
      // Comparing against g_version without aquiring lock is only
      // safe because of it being cache-aligned thus avoiding sharing
      // of the partial data (false-sharing)
      if(t_version == g_version.version) {
        // Enter critical section
        pthread_mutex_lock(&g_lock);

        // It is required to re-allocate a new struct to assure a new
        // memory location so that the cpus have to refetch it from
        // the main memory.
        delete g_data;
        g_data = new w_shared;

        // It is important to update g_version within the critical
        // section. If not another thread might aquire the lock while
        // the operation is ongoing and lead to inconsistency.
        g_version.version = t_version;

        // Copy the data
        g_data->cmd = t_data->cmd;
        g_data->size = t_data->size;
        if(0 < g_data->size) {
          memcpy(g_data->data, t_data->data, g_data->size);
        }

        // Leave critical section
        pthread_mutex_unlock(&g_lock);

        // Return the version that must be the same as
        return t_version;

      } else {
        return g_version.version;

      }
    } else {
      // See comments above. The only difference is that no matter
      // what we overwrite the global state.
      pthread_mutex_lock(&g_lock);

      g_version.version = t_version;

      delete g_data;
      g_data = new w_shared;
      g_data->cmd = t_data->cmd;
      g_data->size = t_data->size;
      memcpy(g_data->data, t_data->data, t_data->size);

      pthread_mutex_unlock(&g_lock);
      return t_version;
    }

    return 0;
}

/**
 * A CAS like operation for updating thread local from a global
 * state. Compares thread local version *t_version* against a global
 * version variable. A mismatch indicates that the thread local state
 * is outdated and requires update. The function will then aquire a
 * lock and update the thread local data.
 *
 * Intrinsics
 * Requires a statically allocated, cache-line aligned (to avoid
 * false-sharing) variable (struct) to check for modification of a
 * state shared among multiple threads. If versions mismatch, that
 * is the thread local version variable doesn't match the global
 * version, a lock will be aquired and the global state copied to the
 * thread local state.
 *
 * \param t_version  Thread local version. This is compared against
                     g_version.version
 * \param *t         Thread local data store
 * \param *g         Shared data store
 *
 * \sa
 * http://stackoverflow.com/questions/12592342/lock-free-check-for-modification-of-a-global-shared-state-in-c-using-cache-line
 */

inline int lazy_update_local_if_changed(int t_version,
                                        w_shared *t_data,
                                        w_shared *g_data)
{
  // False-sharing of g_msg_version can only be avoided if the
  // variable spawns full cache-lines, in this case exactly one and
  // has thus to be cache-line aligned. See stackoverflow article
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

      t_data->cmd = g_data->cmd;
      t_data->size = g_data->size;

      // Copy only if there's something to copy :)
      if(0 < g_data->size) {
        memcpy(t_data->data, g_data->data, g_data->size);
      }
    }

    // Leave critical section
    pthread_mutex_unlock(&g_lock);
    return t_version;
  }
}

#endif /* _LAZY_H */
