#ifndef minunit_h_INCLUDED
#define minunit_h_INCLUDED

#define _GNU_SOURCE

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT_TRUE(message, test) do { if (!(test)) return message; } while (0)
#define ASSERT_EQ(first, second) do { char *message; \
                                      if (first != second) { \
                                          asprintf(&message, "%s != %s", #first, #second); \
                                          return message; \
                                      } \
                                    } while (0)
#define ASSERT_STR_EQ(first, second) do { char *message = assert_str_eq(first, second); \
                                          ASSERT_TRUE(message, message == NULL); } while (0)

static inline char *assert_str_eq(const char *first, const char *second)
{
    const char* template = "\"%s\" != \"%s\"";
    char *err = malloc(sizeof err * (strlen(first) + strlen(second) + strlen(template)));
    sprintf(err, template, first, second);
    return strcmp(first, second) == 0 ? NULL : err;
}

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
    }
    else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %ld\n", tests_run);

    return result == NULL;
}

#endif // minunit_h_INCLUDED
