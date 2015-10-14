CC		= gcc

CFLAGS		= -Wall -pedantic -std=c99 -m32 -g
CFLAGS		= -m64 -g -Wall -pedantic -Werror -std=c99
CFLAGS		= -O3 -maltivec -Wall -pedantic -std=c99
CFLAGS		= -O3 -Wall -pedantic -std=c99
#CFLAGS      = -O3 -maltivec -Wall -pedantic -std=c11 -m64 -pthread -D_GNU_SOURCE

OBJS		= main.o list.o error.o random.o set.o dataflow.o

OUT		= live

#LDFLAGS		= -lpthread -static
LDFLAGS		= -lpthread 

S=100
V=10
U=4
A=10
T=4
P=1

all: $(OBJS)	
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $(OUT)
	#valgrind --tool=helgrind ./$(OUT) $(S) $(V) $(U) $(A) $(T) $(P) $(L)
	#valgrind --tool=memcheck --track-origins=yes ./$(OUT) $(S) $(V) $(U) $(A) $(T) $(P) $(L)
	./$(OUT) $(S) $(V) $(U) $(A) $(T) $(P)

clean:
	rm -f $(OUT) $(OBJS) cfg.dot
