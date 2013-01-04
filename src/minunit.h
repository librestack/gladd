/* 
        file: minunit.h 
        from: http://www.jera.com/techinfo/jtns/jtn002.html
*/

#ifndef __MINUNIT_H__
#define __MINUNIT_H__ 1

#define mu_assert(message, test) do { \
        printf("%i: %s ... ", tests_run, message); \
        if ((test)) { \
                printf("OK\n"); \
        } \
        else { \
                printf("FAIL\n"); \
                return message; \
        } \
} while (0)

#define mu_run_test(test) do { \
        char *message = test(); \
        tests_run++; \
        if (message) return message; \
} while (0)

extern int tests_run;

#endif /* __MINUNIT_H__ */
