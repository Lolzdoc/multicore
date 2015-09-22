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
const int nr_threads = 4;

static double sec(void)
{
	//struct timespec tp; // Change to timeofday?
	//int i = clock_gettime(CLOCK_REALTIME,&tp);
	//return (double)tp.tv_nsec;
	printf("Hej Hej\n");
	return 2;
}


static int cmp(const void* ap, const void* bp)
{	
	double a = *((double*)ap);
	double b = *((double*)bp);
	double diff = a - b;
	if (diff < 0){
		return -1;
	} else if (diff == 0) {
		return 0;
	} else {
		return 1;
	}
}




struct data {
	void* base;	// Array to sort.
	size_t n;	// Number of elements in base.
	size_t s;	// Size of each element.
	int (*cmp)(const void*, const void*);
};






void *my_par_sort(void *parameter) {


	if (threads_started >= nr_threads) {
		void* base = ((struct data*)parameter)->base;
		size_t n = ((struct data*)parameter)->n;	// Number of elements in base.
		size_t s = ((struct data*)parameter)->s;
		qsort(&base,n,s,cmp);
	} else {
		printf("Started new thread");
	struct data left_data; 
	struct data right_data;

	left_data.base = ((struct data*)parameter)->base;
	left_data.n = ((struct data*)parameter)->n/2;

	right_data.n = ((struct data*)parameter)->n - ((struct data*)parameter)->n/2;
	right_data.base = (((struct data*)parameter)->base + ((struct data*)parameter)->n/2);

	left_data.s = right_data.s = ((struct data*)parameter)->s;
	//left_data.(*cmp)(const void*, const void*) = (*parameter).(*cmp)(const void*, const void*);
	//right_data.(*cmp)(const void*, const void*) = (*parameter).(*cmp)(const void*, const void*);

	pthread_t left;
	//pthread_t right;
	threads_started++;
	pthread_create(&left, NULL, my_par_sort, &left_data); 
	//pthread_create(&right, NULL, my_par_sort, &right_data); // Start only one thread...
	my_par_sort(&right_data);

	 pthread_join(left, NULL);
	// pthread_join(right, NULL);

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
