/*
  Invokes preprocessor only:
  gcc -I ../../../main/c/ -E *.cc

  Use to compile:
  gcc -std=c++11 -lstdc++ -Wall -I ../../../main/c/ *.cc

  BY NO MEANS A COMPLETE NOR SAFE testing framework.
*/

#include <assert.h>
#include <pthread.h>
#include <stdio.h>

#define CACHE_LINE_SIZE    64

#include "core/lazy.h"

// struct test {
//   char *name;
//   bool passed;
//   void (*testFn)(void);
// };

// struct test_suite {
//   char *name;
//   test tests[];
//   bool passed_all;
// };

bool passed_cur = false;
bool passed_all = false;


#define ASSERT_EQUALSC(expected, actual) {                              \
    if(expected != actual) {                                            \
      passed_cur = false;                                                   \
      printf("\n    Assertion error. Expected <%c> but was <%c> (%s:%d)\n", \
             expected,                                            \
             actual,                                              \
             __FILE__,                                                  \
             __LINE__                                                   \
             );                                                         \
    } }

#define ASSERT_EQUALSI(expected, actual) {                              \
    if(expected != actual) {                                            \
      passed_cur = false;                                                   \
      printf("\n    Assertion error. Expected <%ld> but was <%ld> (%s:%d)\n", \
             (long)expected,                                            \
             (long)actual,                                              \
             __FILE__,                                                  \
             __LINE__                                                   \
             );                                                         \
    } }

#define ASSERT_TRUE(val) {                                              \
    if(!val) {                                                          \
      passed_cur = false;                                               \
      printf("\n    Assertion error. Expected TURE but was FALSE (%s:%d)\n", \
             __FILE__,                                                  \
             __LINE__                                                   \
             );                                                         \
    } }

#define ASSERT_FALSE(val) {                                             \
    if(val) {                                                           \
      passed_cur = false;                                               \
      printf("\n    Assertion error. Expected FALSE but was TRUE (%s:%d)\n", \
             __FILE__,                                                  \
             __LINE__                                                   \
             );                                                         \
    } }


#define TEST(fn) {                                         \
    passed_cur = true;                                     \
    printf(" > %s",                                        \
           # fn);                                          \
    fn();                                                  \
    printf(" %*s\n",                                       \
           (int)(60-strlen(# fn)),                         \
           passed_cur ? "PASSED" : "");                    \
    passed_all |= passed_cur;                              \
  }

// #define TEST_SUITE(name, __VA_ARGS__) {                        \
//     printf("%s", # name);                              \
//                                                        \
  }
// #define TEST(test_name,                             \
//              body_fn) {                             \
//   printf(" > %s\n", test_name);                     \
//   auto fn = body_fn;                                \
//   fn();                                             \
// }
  // TEST("testStructIsCacheLineAligned",
  //      [] () -> void
  //      {
  //        static cache_line_aligned a;
  //        ASSERT_EQUALSI(CACHE_LINE_SIZE,
  //                       sizeof(a));
  //        ASSERT_EQUALSI(2,
  //                       sizeof(a));
  //      });

void test_StructIsCacheLineAligned()
{
  static cache_line_aligned a;

  ASSERT_EQUALSI(CACHE_LINE_SIZE,
                 sizeof(a));
}

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

void test_UpdateGlobalFailsIfNotOwner()
{
  static my_data g_data = {
    'a',
    1
  };

  static my_data t_data = {
    'b',
    0
  };

  int t_version;
  int actual;

  lazy_init(2);
  {
    // This should fail and return with the actual version
    t_version = -1;
    actual = lazy_update_global_if_owner(t_version,
                                         &t_data,
                                         &g_data,
                                         t_to_g);
    ASSERT_FALSE(t_version + 1 == actual);
    ASSERT_EQUALSC('a', g_data.data);
    ASSERT_TRUE(2 == actual);
  }
  lazy_destroy();
}

void test_UpdateGlobalIfOwnerWorks()
{
  static my_data g_data = {
    'a',
    0
  };

  static my_data t_data = {
    'b',
    0
  };

  int t_version;
  int actual;

  lazy_init(2);
  {
    // The update should succeed.
    t_version = 2;
    actual = lazy_update_global_if_owner(t_version,
                                         &t_data,
                                         &g_data,
                                         t_to_g);
    ASSERT_EQUALSI(3, actual);
    ASSERT_EQUALSC('b', g_data.data);
  }
  lazy_destroy();
}

void test_UpdateGlobalIfOwnerWorks()
{
  static my_data g_data = {
    'a',
    0
  };

  static my_data t_data = {
    'b',
    0
  };

  int t_version;
  int actual;

  lazy_init(2);
  {
    // The update should succeed.
    t_version = 2;
    actual = lazy_update_global_if_owner(t_version,
                                         &t_data,
                                         &g_data,
                                         t_to_g);
    ASSERT_EQUALSI(3, actual);
    ASSERT_EQUALSC('b', g_data.data);
  }
  lazy_destroy();
}

int main(int argc, char *argv[])
{
  printf("Running tests...\n");


  //TEST_SUITE("lazy.h tests",
  TEST(test_StructIsCacheLineAligned);
  TEST(test_UpdateGlobalFailsIfNotOwner);
  TEST(test_UpdateGlobalIfOwnerWorks);

  return passed_all;
}
