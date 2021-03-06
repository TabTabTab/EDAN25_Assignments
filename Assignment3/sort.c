#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/times.h>
#include <sys/time.h>
#include <unistd.h>


#define NSAMPLES (100)
#define NSPLITS (3)
#define NTHREADS (1 << NSPLITS)

int* split_points;

static int cmp(const void* ap, const void* bp);
void inner_part_array(double* a, int l_start, int length, int depth, int i);
int bin_split(double* a, int length);

struct t_args {
    size_t size;
    void* a;
    int length;
};
typedef struct t_args t_args;

void* sort(void* targ)
{
    t_args* args = (t_args*)targ;
    qsort(args->a, args->length, args->size, cmp);
}

static double sec(void)
{
    struct timeval t;
    gettimeofday(&t, NULL);
    double t_us = (double)t.tv_usec;
    double t_s = (double)t.tv_sec;
    double val = t_s + t_us / 1000000;
    return val;
}

double find_that_pivot(double* a, int length)
{
    int index = 0;
    int nsamp = length < NSAMPLES ? length : NSAMPLES;
    qsort(a, nsamp, sizeof(double), cmp);
    index = nsamp / 2 ;
    return a[index];
}

int partition_array(double* a, int length)
{
    split_points[NTHREADS] = length;

    inner_part_array(a, 0, length, 0, 0);
}

void inner_part_array(double* a, int l_start, int length, int depth, int i)
{
    if(depth == NSPLITS){
        split_points[i] = l_start;
        return;
    }
    depth ++;

    int spl = NSPLITS - depth;
    int i_r = i + (1 << spl);
    int r_start = bin_split(&a[l_start], length);
    r_start += l_start;
    int r_length = length - (r_start - l_start);
    int l_length = r_start - l_start;

    inner_part_array(a, l_start, l_length, depth, i);
    inner_part_array(a, r_start, r_length, depth, i_r);
}
/**
 * Returns an int determining the start of p2 while p1 starts at i=0
**/
int bin_split(double* a, int length)
{
    double pivot = find_that_pivot(a, length);
    //first we find the n of elements below or equal to pivot
    int n_low=0;
    for(int i = 0;i<length; ++i){
        if(a[i] <= pivot){
            n_low++;
        }
    }
    double percentage = ((double)n_low) / length;
    //printf(" pivot percent: %f \n\n", percentage);
    int temp;
    int next_high_index = n_low;
    for(int i=0; i<n_low ;i++){
        if(a[i]>pivot){
            while(a[next_high_index] > pivot){
                next_high_index++;
            }
            //now we swap!
            temp = a[i];
            a[i] = a[next_high_index];
            a[next_high_index] = temp;
        }
    }
    return n_low;
}

void par_sort(
    void*        base,    // Array to sort.
    size_t        n,    // Number of elements in base.
    size_t        s,    // Size of each element.
    int        (*cmp)(const void*, const void*)) // Behaves like strcmp
{

    partition_array(base, n);
    pthread_t threads[NTHREADS];
    t_args as[NTHREADS];

    for(int i=0;i<NTHREADS; ++i){
        int start = split_points[i];
        int len = split_points[i+1] - start;
        as[i].a = base + start*s;
        as[i].length= len;
        as[i].size = s;
        pthread_create(&threads[i], NULL, sort, &as[i]);
    }


    //wait for threads
    for(int i=0; i<NTHREADS; ++i){
        if(pthread_join(threads[i], NULL)) {
            fprintf(stderr, "Error joining thread\n");
        }
    }
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

static void print_array(const double* a, int length)
{
    printf("\nWow, thanks for that beautiful array:\n");
    for( int i = 0; i < length; ++i )
        printf("\n%f ", a[i] );
    printf("\n");
}

static int status_is_ok(double* a, int length)
{
    double last = 0;
    for(int i=0; i<length; ++i){
        if(last > a[i]){
            printf("\n\nIt seems you have not sorted correctly\n");
            return 0;
        }
        last = a[i];
    }
    //printf("\n\nIts all sorted!, you are like a God\n");
    return 1;
}

void do_seq(int n, int seed)
{
    int        i;
    double*        a;
    double        start, end;

    srand(seed);

    a = malloc(n * sizeof a[0]);
    for (i = 0; i < n; i++){
        a[i] = rand();
        //printf("%f\n", a[i]);
    }
    start = sec();

    qsort(a, n, sizeof a[0], cmp);


    end = sec();

    printf("%1.4f s\n", end - start);
    //print_array(a, n);
    status_is_ok(a, n);;
    free(a);

}


void do_par(int n, int seed)
{
    int        i;
    double*        a;
    double        start, end;


    srand(seed);

    a = malloc(n * sizeof a[0]);
    for (i = 0; i < n; i++){
        a[i] = rand();
    }
    start = sec();


    split_points = malloc(sizeof(int)*(NTHREADS+1));
    par_sort(a, n, sizeof a[0], cmp);


    end = sec();

    printf("%1.4f s\n", end - start);
    //print_array(a, n);
    status_is_ok(a, n);;
    free(a);
    free(split_points);

}

int main(int ac, char** av)
{
    int        n = 20000;

    if (ac > 1)
        sscanf(av[1], "%d", &n);
    int seed = getpid();
    printf("======================================\n");
    printf("doing par\n");
    do_par(n, seed);
    printf("doing seq:\n");
    do_seq(n, seed);
    printf("======================================\n");
    return 0;
    /*
    srand(getpid());

    a = malloc(n * sizeof a[0]);
    for (i = 0; i < n; i++){
        a[i] = rand();
        //printf("%f\n", a[i]);
    }
    start = sec();

#ifdef PARALLEL
    split_points = malloc(sizeof(int)*(NTHREADS+1));
    par_sort(a, n, sizeof a[0], cmp);
#else
    qsort(a, n, sizeof a[0], cmp);
#endif

    end = sec();

    printf("%1.4f s\n", end - start);

    //print_array(a, n);
    status_is_ok(a, n);;
    free(a);

    return 0;
    */
}
