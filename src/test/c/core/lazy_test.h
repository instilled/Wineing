
#ifndef _LAZY_TEST_H
#define _LAZY_TEST_H

#include "core/lazy.h"

struct my_data {
  char data;
  size_t size;
};

static inline void t_to_g (const void *t, void *g)
{
  const my_data *local = (const my_data*)t;
  my_data *shared = (my_data*)g;

  shared->size = local->size;
  shared->data = local->data;
}

static inline void g_to_t(void *t, const void *g)
{
  my_data *local = (my_data*)t;
  const my_data *shared = (const my_data*)g;

  local->size = shared->size;
  local->data = shared->data;
}



#endif /* _LAZY_TEST_H */
