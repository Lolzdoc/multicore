#include <stdio.h>
#include <string.h>

#define N (1024)

double a[N][N];
double b[N][N];
double c[N][N];

int main(void)
{
	size_t	i, j, k;
	#pragma omp parallel for
	for (i = 0; i < N; i += 1) {
		for (k = 0; k < N; k += 1)
			for (j = 0; j < N; j += 1) {
				a[i][j] = 0;
				a[i][j] += b[i][k] * c[k][j];
		}
	}

	return 0;
}
