#ifndef minunit_h_INCLUDED
#define minunit_h_INCLUDED

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT_TRUE(message, test) do { if (!(test)) return message; } while (0)
#define ASSERT_EQ(first, second) do { if (first != second) { \
                                          const int message_cap = 128; \
                                          char *message = malloc(message_cap); \
                                          const char format[] = "%s:%d %s != %s"; \
                                          int chars_needed = snprintf(message, message_cap, format, __FILE__, __LINE__, #first, #second); \
                                          if (chars_needed >= message_cap) { \
                                              free(message); \
                                              message = malloc(chars_needed + 1); \
                                              snprintf(message, chars_needed+1, format, __FILE__, __LINE__, #first, #second); \
                                          } \
                                          return message; \
                                      } \
                                    } while (0)
#define ASSERT_STR_EQ(first, second) do { ASSERT_EQ(strcmp(first, second), 0); } while (0)

static inline _Bool run_all_tests(char* (*test_functions[]) ()) {
    char *result;
    size_t tests_run = 0;
    for (; test_functions[tests_run] != NULL; tests_run++) {
        result = (*test_functions[tests_run])();
        if (result) {
            break;
        }
    }
    if (result != NULL) {
        printf("%s\n", result);
    } else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %ld\n", tests_run);

    return result == NULL;
}

#endif // minunit_h_INCLUDED
