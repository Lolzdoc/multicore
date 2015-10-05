#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <pthread.h>
#include "dataflow.h"
#include "error.h"
#include "list.h"
#include "set.h"

typedef struct vertex_t	vertex_t;
typedef struct task_t	task_t;
static pthread_mutex_t worklist_mutex;
static list_t*	worklist;

/* cfg_t: a control flow graph. */
struct cfg_t {
	size_t			nvertex;	/* number of vertices		*/
	size_t			nsymbol;	/* width of bitvectors		*/
	vertex_t*		vertex;		/* array of vertex		*/
};


/* vertex_t: a control flow graph vertex. */
struct vertex_t {
	size_t			index;		/* can be used for debugging	*/
	set_t*			set[NSETS];	/* live in from this vertex	*/
	set_t*			prev;		/* alternating with set[IN]	*/
	size_t			nsucc;		/* number of successor vertices */
	vertex_t**		succ;		/* successor vertices 		*/
	list_t*			pred;		/* predecessor vertices		*/
	bool			listed;		/* on worklist			*/
	pthread_mutex_t mymutex;
};

static void clean_vertex(vertex_t* v);
static void init_vertex(vertex_t* v, size_t index, size_t nsymbol, size_t max_succ);

void unlock_mutex(pthread_mutex_t* mutex,char s[]) {
	printf("Unlocked a mutex at %s\n",s);
	int a = pthread_mutex_unlock(mutex);
	if (a != 0 ){
		printf("ERROR no %d in Unlock\n",a );
		abort();
	}
}

void lock_mutex(pthread_mutex_t* mutex,char s[]) {
	printf("Locked a mutex at %s\n",s );
	int a = pthread_mutex_lock(mutex);
	if (a != 0 ){
		printf("ERROR no %d in Lock\n", a);
		abort();
	}
}

cfg_t* new_cfg(size_t nvertex, size_t nsymbol, size_t max_succ)
{
	size_t		i;
	cfg_t*		cfg;

	cfg = calloc(1, sizeof(cfg_t));
	if (cfg == NULL)
		error("out of memory");

	cfg->nvertex = nvertex;
	cfg->nsymbol = nsymbol;

	cfg->vertex = calloc(nvertex, sizeof(vertex_t));
	if (cfg->vertex == NULL)
		error("out of memory");

	for (i = 0; i < nvertex; i += 1)
		init_vertex(&cfg->vertex[i], i, nsymbol, max_succ);

	return cfg;
}

static void clean_vertex(vertex_t* v)
{
	int		i;

	for (i = 0; i < NSETS; i += 1)
		free_set(v->set[i]);
	free_set(v->prev);
	free(v->succ);
	free_list(&v->pred);
}

static void init_vertex(vertex_t* v, size_t index, size_t nsymbol, size_t max_succ)
{
	int		i;

	v->index	= index;
	v->succ		= calloc(max_succ, sizeof(vertex_t*));

	if (v->succ == NULL)
		error("out of memory");
	
	for (i = 0; i < NSETS; i += 1)
		v->set[i] = new_set(nsymbol);

	v->prev = new_set(nsymbol);
}

void free_cfg(cfg_t* cfg)
{
	size_t		i;

	for (i = 0; i < cfg->nvertex; i += 1)
		clean_vertex(&cfg->vertex[i]);
	free(cfg->vertex);
	free(cfg);
}

void connect(cfg_t* cfg, size_t pred, size_t succ)
{
	vertex_t*	u;
	vertex_t*	v;

	u = &cfg->vertex[pred];
	v = &cfg->vertex[succ];

	u->succ[u->nsucc++ ] = v;
	insert_last(&v->pred, u);
}

bool testbit(cfg_t* cfg, size_t v, set_type_t type, size_t index)
{
	return test(cfg->vertex[v].set[type], index);
}

void setbit(cfg_t* cfg, size_t v, set_type_t type, size_t index)
{
	set(cfg->vertex[v].set[type], index);
}
void* compute_in(void *list) {
	vertex_t*	u;
	vertex_t*	v;
	list_t*		p;
	list_t*		h;
	size_t		j;
	set_t*		prev;


	lock_mutex(&worklist_mutex,"initial worklist mutex1"); // Lock Worklist

	u = remove_first(&worklist);		

	lock_mutex(&(u->mymutex),"urrent Vertex2"); // Lock current Vertex
	u->listed = false;
	unlock_mutex(&worklist_mutex, "initial worklist mutex3");  // Unlock Worklist
	while (u != NULL) {

		reset(u->set[OUT]);
		for (j = 0; j < u->nsucc; ++j) {
			lock_mutex(&(u->succ[j]->mymutex),"at succ4"); // Lock succ[j] Vertex 
			or(u->set[OUT], u->set[OUT], u->succ[j]->set[IN]);
			unlock_mutex(&(u->succ[j]->mymutex),"at scc5"); // Unlock succ[j] Vertex 
		}
		prev = u->prev;
		u->prev = u->set[IN];
		u->set[IN] = prev;

		/* in our case liveness information... */
		propagate(u->set[IN], u->set[OUT], u->set[DEF], u->set[USE]);

		if (u->pred != NULL && !equal(u->prev, u->set[IN])) {
			p = h = u->pred;
			do {
				v = p->data;
				lock_mutex(&(v->mymutex),"at pred vertex6"); // Lock pred vertex;
				if (!v->listed) {
					lock_mutex(&worklist_mutex, "worklist7"); // Lock worklist
					v->listed = true;
					insert_last(&worklist, v);
					
					unlock_mutex(&worklist_mutex, "pred vertex worklist10"); // Unlock Worklist
				}
				unlock_mutex(&(v->mymutex), "at pred vertex v9"); // Unlock pred vertex;
				p = p->succ;

			} while (p != h);
			printf("end of Do While12\n");
		}
		unlock_mutex(&(u->mymutex),"Current Vertex13"); // Unlock current vertex



		lock_mutex(&worklist_mutex, "final worklist14");
		 // Lock worklist
		u = remove_first(&worklist);
		lock_mutex(&(u->mymutex), "new Vertex u15"); // Lock new Current vertex
		u->listed = false;
		
		unlock_mutex(&worklist_mutex, "final worklist16"); // Unlock worklist

	}
	return 0;
}


void liveness(cfg_t* cfg)
{
	vertex_t* u;
	
	
	worklist = NULL;

	pthread_mutexattr_t attrmutex;

	/* Initialise attribute to mutex. */
	if (pthread_mutexattr_init(&attrmutex) != 0) {
		printf("LOLOL\n");
		abort();
	}
	pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);

	/* Allocate memory to pmutex here. */

	/* Initialise mutex. */
	

	//pthread_mutex_init(&(list->worklist_mutex),NULL);
	int a = pthread_mutex_init(worklist, &attrmutex);
	if (a != 0) {
		printf("ERROR no %dmutex create\n",a );
	}
	int nthread = 4;

	for (size_t i = 0; i < cfg->nvertex; ++i) {
		u = &cfg->vertex[i];

		insert_last(&(worklist), u);
		u->listed = true;
		pthread_mutex_init(&(u->mymutex),&attrmutex);
	}
	// Threading starts after this

	pthread_t threads[nthread];
	for (int k = 0; k < nthread; k++) {
		printf("started Thread %d\n", k+1);
		pthread_create(&(threads[k]),NULL,&compute_in,NULL);
	}

	for (int k = 0; k < nthread; k++) {
		pthread_join(threads[k],NULL);
		printf("Ended thread\n");
	}

	pthread_mutex_destroy(worklist);
	pthread_mutexattr_destroy(&attrmutex); 
}



void print_sets(cfg_t* cfg, FILE *fp)
{
	size_t		i;
	vertex_t*	u;

	for (i = 0; i < cfg->nvertex; ++i) {
		u = &cfg->vertex[i]; 
		fprintf(fp, "use[%zu] = ", u->index);
		print_set(u->set[USE], fp);
		fprintf(fp, "def[%zu] = ", u->index);
		print_set(u->set[DEF], fp);
		fputc('\n', fp);
		fprintf(fp, "in[%zu] = ", u->index);
		print_set(u->set[IN], fp);
		fprintf(fp, "out[%zu] = ", u->index);
		print_set(u->set[OUT], fp);
		fputc('\n', fp);
	}
}