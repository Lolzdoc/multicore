#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/times.h>
#include <sys/time.h>
#include <unistd.h>




int threads_started = 0; // Borde vara atomic annars data race
const nr_threads = 4;

static double sec(void)
{
	struct timespec tp;
	int i = clock_gettime(CLOCK_REALTIME,&tp);
	return (double)tp.tv_nsec;
}

struct data {
	void* base;	// Array to sort.
	size_t n;	// Number of elements in base.
	size_t s;	// Size of each element.
	int (*cmp)(const void*, const void*);
};






void my_par_sort(void *parameter) {


	if (threads_started >= nr_threads) {
		qsort((*parameter).base,(*parameter).n,(*parameter).s,(*cmp)(const void*, const void*))
	} else {

	struct data left_data; 
	struct data right_data;

	left_data.base = (*parameter).base;
	left_data.n = (*parameter).n/2;

	right_data.n = (*parameter).n - (*parameter).n/2;
	right_data.base = ((*parameter).base + (*parameter).n/2);

	left_data.s = right_data.s = (*parameter).s;
	//left_data.(*cmp)(const void*, const void*) = (*parameter).(*cmp)(const void*, const void*);
	//right_data.(*cmp)(const void*, const void*) = (*parameter).(*cmp)(const void*, const void*);

	pthread_t left;
	pthread_t right;

	pthread_create(&left, NULL, my_par_sort, &left_data);
	pthread_create(&right, NULL, my_par_sort, &right_data);

	threads_started += 2;

	 pthread_join(left, NULL);
	 pthread_join(right, NULL);

	}

}


void par_sort(
	void* base,	// Array to sort.
	size_t n,	// Number of elements in base.
	size_t s,	// Size of each element.
	int (*cmp)(const void*, const void*)) { // Behaves like strcmp
	
	struct data d;
	d.base = base;
	d.n = n;
	d.s = s;
	//d.(*cmp)(const void*, const void*) = (*cmp)(const void*, const void*);

	my_par_sort(&d);
}


static int cmp(const void* ap, const void* bp)
{	
	double a = *ap;
	double b = *bp;
	double diff = a - b;
	if (diff < 0){
		return -1;
	} else if (diff == 0) {
		return 0;
	} else {
		return 1;
	}
}

int main(int ac, char** av)
{
	int		n = 2000000;
	int		i;
	double*		a;
	double		start, end;

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
