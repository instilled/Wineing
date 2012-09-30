
#include <check.h>

#define CACHE_LINE_SIZE 64

#include "lazy_test.h"


START_TEST (test_StructIsCacheLineAligned)
{
  static cache_line_aligned a;

  fail_unless (sizeof(a) == CACHE_LINE_SIZE, NULL);
}
END_TEST

START_TEST (test_UpdateGlobalFailsIfNotOwner)
{
  my_data g_data = {
    'a',
    1
  };

  my_data t_data = {
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

    // If update succeeds t_version woulb be incremented by
    // one, that is -1 + 1 = 0 is != to 2 (because we initialized
    // with 2 lazy_init
    fail_unless (t_version + 1 != actual, NULL);
    fail_unless ('a' == g_data.data);
    fail_unless (2 == actual);
  }
  lazy_destroy();
}
END_TEST

void setup (void)
{
  //five_dollars = money_create (5, "USD");
}

void teardown (void)
{
  //money_free (five_dollars);
}

Suite * lazy_suite (void)
{
  Suite *s = suite_create ("Lazy");

  /* Core test case */
  TCase *tc_core = tcase_create ("core");
  //tcase_add_checked_fixture (tc_core, setup, teardown);
  tcase_add_test (tc_core, test_StructIsCacheLineAligned);
  tcase_add_test (tc_core, test_UpdateGlobalFailsIfNotOwner);
  suite_add_tcase (s, tc_core);

  return s;
}

