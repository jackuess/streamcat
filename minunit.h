#ifndef minunit_h_INCLUDED
#define minunit_h_INCLUDED

#include <stdio.h>

#define mu_assert(message, test) do { if (!(test)) return message; } while (0)
#define mu_run_test(test) do { char *message = test(); tests_run++; \
                               if (message) return message; } while (0)
int tests_run;

char *all_tests();

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    char *result = all_tests();
    if (result != 0) {
        printf("%s\n", result);
    }
    else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);

    return result != 0;
}

#endif // minunit_h_INCLUDED

