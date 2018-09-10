#ifndef scut_h_INCLUDED
#define scut_h_INCLUDED

#include <string.h>

#define ASSERT_(tr, success, first, second, func) do { \
                                                      tr->n_assertions++; \
                                                      if (!success) { \
                                                          tr_append_error_message(tr, t_error_message(func, first, second)); \
                                                          return; \
                                                      } \
                                                  } while (0)
#define ASSERT_TRUE(tr, expression) ASSERT_(tr, (expression), #expression, "true", __func__)
#define ASSERT_EQ(tr, first, second) ASSERT_(tr, (first == second), #first, #second, __func__)
#define ASSERT_STR_EQ(tr, first, second) ASSERT_(tr, strcmp(first, second) == 0, #first, #second, __func__)

#define MAX_ERROR_MESSAGES 5
struct TestResult {
    size_t n_tests;
    size_t n_assertions;
    char *error_messages[MAX_ERROR_MESSAGES];
    unsigned int n_error_messages;
};

char *t_error_message (const char *test_name, const char *first, const char *second);
struct TestResult *t_run(size_t n_test_functions, void (**test_functions) (struct TestResult *));
void tr_append_error_message(struct TestResult *tr, char *message);
void tr_print(const struct TestResult *tr);
_Bool tr_success(const struct TestResult *tr);
void tr_free(struct TestResult *tr);

#endif // scut_h_INCLUDED
