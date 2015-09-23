#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/times.h>
#include <sys/time.h>
#include <unistd.h>




_Atomic int threads_started = 1; // Borde vara atomic annars data race
const int nr_threads = 1;

static double sec(void)
{
	struct timeval tv; // Change to timeofday?
	gettimeofday(&tv,NULL);
	return (double)(tv.tv_sec + (double)tv.tv_usec/1000000);
}

static int cmp(const void* ap, const void* bp)
{	
		//printf("asd \n");
	const double a = *(double*) ap;
	const double b = *(double*) bp;

	if (a < b){
		return -1;
	} else if (a == b) {
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
	int id;
};

int my_random() {
	int a = rand(); //0 -> 32767 atleast
	a = a % 10; // 0 -> 9
	if (a == 0) {
		a = 1;
	}
	return a;
}

void sort(void *parameter) {
	void* base = ((struct data*)parameter)->base;
	size_t n = ((struct data*)parameter)->n;	// Number of elements in base.
	size_t s = ((struct data*)parameter)->s;
	int (*cmp)(const void*, const void*) = ((struct data*)parameter)->cmp;
	//printf("%p %d %d\n", base, (int) n, (int) s);
	qsort(base,n,s,cmp);
}

void merge(void* left_data,void* right_data) {

	double* base_left = ((struct data*)left_data)->base;
	size_t n_left = ((struct data*)left_data)->n;	// Number of elements in base.
	double* base_right = ((struct data*)right_data)->base;
	size_t n_right = ((struct data*)right_data)->n;

	double* temp = malloc((n_left+n_right)*sizeof(double));
	int index = 0;
	int i = 0;
	int j = 0;

	while(i<n_left && j<n_right){

			if(base_left[i] > base_right[j]){
				temp[index] = base_right[j];
				index++;
				j++;
			} else {
				temp[index] = base_left[i];
				index++;
				i++;
			}
	}
		while(i<n_left){
			temp[index] = base_left[i];
			index++;
			i++;
		}
		while(j<n_right){
			temp[index] = base_right[j];
			index++;
			j++;
		}
		//memcpy((temp+index*sizeof(double)),(base_left+i*sizeof(double)),(n_left-i)*sizeof(double));


	memcpy(base_left,temp,(n_left+n_right)*sizeof(double));
	free(temp);

}

void *my_par_sort(void *parameter) {
	if (threads_started >= nr_threads) {
		//printf("max nr of threads reached %d\n",((struct data*)parameter)->id);
		sort(parameter);
	} else {
	threads_started++; 
	
	struct data left_data; 
	struct data right_data;
	left_data.id = threads_started;
	right_data.id = ((struct data*)parameter)->id;
	void* base = ((struct data*)parameter)->base;	
	size_t n = ((struct data*)parameter)->n;	
	size_t s = ((struct data*)parameter)->s;
	int a = my_random();

	left_data.base = base;
	left_data.n = (int)(n/a);

	right_data.n = n - (int)(n/a);
	right_data.base = (double*)base + (int)n/a;

	left_data.s = right_data.s = s;
	left_data.cmp = ((struct data*)parameter)->cmp;
	right_data.cmp = ((struct data*)parameter)->cmp;

	pthread_t left;

	
	pthread_create(&left, NULL, my_par_sort, &left_data);
	//printf("Started new thread %d\n",left_data.id);
	my_par_sort(&right_data);

	pthread_join(left, NULL);

	merge(&left_data,&right_data);



	}
	return 0;
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
	d.cmp = cmp;
	d.id = threads_started;
	//printf("first id: %d\n", d.id);

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
	par_sort(a, n, sizeof a[0], cmp);
	//qsort(a, n, sizeof a[0], cmp);
#endif

	end = sec();
	
	printf("%1.2f s\n", end - start);

	free(a);

	return 0;
}
