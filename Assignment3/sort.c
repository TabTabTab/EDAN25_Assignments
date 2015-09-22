#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/times.h>
#include <sys/time.h>
#include <unistd.h>

static double sec(void)
{
    struct timeval t;
    gettimeofday(&t, NULL);
    double t_us = (double)t.tv_usec;
    double t_s = (double)t.tv_sec;
    double val = t_s + t_us / 1000000;
    return val;
}

void par_sort(
    void*        base,    // Array to sort.
    size_t        n,    // Number of elements in base.
    size_t        s,    // Size of each element.
    int        (*cmp)(const void*, const void*)) // Behaves like strcmp
{
}

static int cmp(const void* ap, const void* bp)
{
    double *p1, *p2;
    p1 = (double*)ap;
    p2 = (double*)bp;
    double d1 = (double)(*p1);
    double d2 = (double)(*p2);
    return d1 < d2 ? -1 : d1 == d2 ? 0 : 1;
}

int main(int ac, char** av)
{
    int        n = 2000000;
    int        i;
    double*        a;
    double        start, end;

    if (ac > 1)
        sscanf(av[1], "%d", &n);

    srand(getpid());

    a = malloc(n * sizeof a[0]);
    for (i = 0; i < n; i++)
        a[i] = rand();

    start = sec();

#ifdef PARALLEL
    par_sort(a, n, sizeof a[0], cmp);
#else
    qsort(a, n, sizeof a[0], cmp);
#endif

    end = sec();

    printf("%1.2f s\n", end - start);

    free(a);

    return 0;
}
