#include "type.h"
#include "util.h"
#include "phase.h"

#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct error {
   int        line;
   const char *snippet;
} error_t;

static const error_t  *error_lines = NULL;
static error_fn_t orig_error_fn = NULL;

static void setup(void)
{
   const char *lib_dir = getenv("LIB_DIR");
   if (lib_dir)
      lib_add_search_path(lib_dir);

   lib_set_work(lib_tmp());
   opt_set_int("bootstrap", 0);
   opt_set_int("unit-test", 1);
   opt_set_int("prefer-explicit", 0);
}

static void teardown(void)
{
   lib_free(lib_work());
}

static void test_error_fn(const char *msg, const loc_t *loc)
{
   fail_if(error_lines == NULL);

   bool unexpected = error_lines->line == -1
      || error_lines->snippet == NULL
      || error_lines->line != loc->first_line
      || strstr(msg, error_lines->snippet) == NULL;

   if (unexpected) {
      orig_error_fn(msg, loc);
      printf("expected line %d '%s'\n",
             error_lines->line, error_lines->snippet);
   }

   fail_if(unexpected);

   error_lines++;
}

static void expect_errors(const error_t *lines)
{
   fail_unless(orig_error_fn == NULL);
   orig_error_fn = set_error_fn(test_error_fn);
   error_lines = lines;
}

START_TEST(test_bounds)
{
   tree_t e, a;

   const error_t expect[] = {
      {  26, "left index 0 violates constraint STD.STANDARD.POSITIVE" },
      {  27, "right index 60 violates constraint FOO" },
      {  31, "array S index -52 out of bounds 1 to 10" },
      {  32, "slice right index 11 out of bounds 1 to 10" },
      {  33, "slice left index 0 out of bounds 1 to 10" },
      {  37, "aggregate index 0 out of bounds 1 to 2147483647" },
      {  46, "actual length 8 does not match formal length 4" },
      {  47, "actual length 8 does not match formal length 4" },
      {  50, "actual length 3 for dimension 2 does not" },
      {  53, "length of value 9 does not match length of target 10" },
      {  54, "length of value 2 does not match length of target 0" },
      {  55, "expected 0 elements in aggregate but have 3" },
      {  60, "length of value 10 does not match length of target 3" },
      {  66, "array S index 11 out of bounds 1 to 10" },
      {  67, "array S index -1 out of bounds 1 to 10" },
      {  74, "expected 6 elements in aggregate but have 3" },
      {  74, "length of value 6 does not match length of target 3" },
      {  83, "value '1' out of target bounds 'a' to 'z'" },
      {  84, "value 0 out of target bounds 1 to 2147483647" },
      {  89, "invalid dimension 5 for type MY_VEC1" },
      {  94, "value -1 out of bounds 0 to 2147483647 for parameter X" },
      { 107, "aggregate index 5 out of bounds 1 to 3" },
      { 116, "length of sub-aggregate 2 does not match expected length 4" },
      { 137, "array index 14 out of bounds 0 to 2" },
      { -1, NULL }
   };
   expect_errors(expect);

   input_from_file(TESTDIR "/bounds/bounds.vhd");

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);
   fail_unless(sem_errors() == 0);

   simplify(a);
   bounds_check(a);

   fail_unless(bounds_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_case)
{
   tree_t e, a;

   const error_t expect[] = {
      {  13, "missing choice C in case statement" },
      {  19, "missing choice B in case statement" },
      {  30, "10 to 19" },
      {  36, "4 to 2147483647" },
      {  44, "2147483647" },
      {  51, "value 50 is already covered" },
      {  53, "range 60 to 64 is already covered" },
      {  59, "value -1 outside STD.STANDARD.NATURAL bounds" },
      {  58, "0 to 2147483647" },
      {  79, "choices cover only 2 of 8 possible values" },
      {  84, "expected 3 elements in aggregate but have 2" },
      {  86, "expected 3 elements in aggregate but have 4" },
      {  88, "expected 3 elements in string literal but have 2" },
      {  90, "expected 3 elements in string literal but have 4" },
      {  95, "choices do not cover all possible values" },
      { 101, "choices cover only 2 of 100 possible values" },
      { -1, NULL }
   };
   expect_errors(expect);

   input_from_file(TESTDIR "/bounds/case.vhd");

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);
   fail_unless(sem_errors() == 0);

   simplify(a);
   bounds_check(a);

   fail_unless(bounds_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_issue36)
{
   tree_t e;

   const error_t expect[] = {
      { -1, NULL }
   };
   expect_errors(expect);

   input_from_file(TESTDIR "/bounds/issue36.vhd");

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);
   fail_unless(sem_errors() == 0);

   simplify(e);
   bounds_check(e);

   fail_unless(bounds_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

START_TEST(test_issue54)
{
   tree_t e, a;

   const error_t expect[] = {
      { 12, "aggregate index 3 out of bounds 7 downto 4" },
      { 12, "aggregate index 0 out of bounds 7 downto 4" },
      { -1, NULL }
   };
   expect_errors(expect);

   input_from_file(TESTDIR "/bounds/issue54.vhd");

   e = parse();
   fail_if(e == NULL);
   fail_unless(tree_kind(e) == T_ENTITY);
   sem_check(e);

   a = parse();
   fail_if(a == NULL);
   fail_unless(tree_kind(a) == T_ARCH);
   sem_check(a);

   fail_unless(parse() == NULL);
   fail_unless(parse_errors() == 0);
   fail_unless(sem_errors() == 0);

   simplify(a);
   bounds_check(a);

   fail_unless(bounds_errors() == (sizeof(expect) / sizeof(error_t)) - 1);
}
END_TEST

int main(void)
{
   register_trace_signal_handlers();

   setenv("NVC_LIBPATH", "../lib/std", 1);

   Suite *s = suite_create("bounds");

   TCase *tc_core = tcase_create("Core");
   tcase_add_unchecked_fixture(tc_core, setup, teardown);
   tcase_add_test(tc_core, test_bounds);
   tcase_add_test(tc_core, test_case);
   tcase_add_test(tc_core, test_issue36);
   tcase_add_test(tc_core, test_issue54);
   suite_add_tcase(s, tc_core);

   SRunner *sr = srunner_create(s);
   srunner_run_all(sr, CK_NORMAL);

   int nfail = srunner_ntests_failed(sr);

   srunner_free(sr);

   return nfail == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
