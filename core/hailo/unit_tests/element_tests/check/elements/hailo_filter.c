#include <gst/check/gstcheck.h>
#include <gst/check/check.h>
#include <stdio.h>

// Test definition
GST_START_TEST(test_hello_world)
{
    printf("TESTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT");
}

END_TEST;

// Suite definition to allow to run a group of test and allow for further control
// Of what test to run
static Suite *
hello_suite (void)
{
    Suite *s = suite_create ("hello");
    TCase *tc_chain = tcase_create ("world");

    suite_add_tcase (s, tc_chain);
    tcase_add_test (tc_chain, test_hello_world);

    return s;
}

// Defines what suite to run as part of the normal calling
GST_CHECK_MAIN (hello);