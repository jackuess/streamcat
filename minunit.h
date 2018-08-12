#ifndef minunit_h_INCLUDED
#define minunit_h_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT_TRUE(message, test) do { if (!(test)) return message; } while (0)
#define ASSERT_STR_EQ(first, second) do { char *message = assert_str_eq(first, second); \
                                          ASSERT_TRUE(message, message == NULL); } while (0)
#define RUN_TEST(test) do { char *message = test(); tests_run++; \
                            if (message) return message; } while (0)
int tests_run;

char *all_tests();

char *assert_str_eq(char *first, char *second)
{
    const char* template = "\"%s\" != \"%s\"";
    char *err = malloc(sizeof err * (strlen(first) + strlen(second) + strlen(template)));
    sprintf(err, template, first, second);
    return strcmp(first, second) == 0 ? NULL : err;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    char *result = all_tests();
    if (result != NULL) {
        printf("%s\n", result);
    }
    else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);

    return result != NULL;
}

#endif // minunit_h_INCLUDED

