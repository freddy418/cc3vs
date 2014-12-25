PROG = cache_sim
CC = g++
SRCS = utils.cpp store.cpp memmap.cpp tcache.cpp ccache.cpp rgcache.cpp cltcache.cpp cache_sim.cpp
OBJS = ${SRCS:.cpp=.o}
CFLAGS = -g -O3 -fno-omit-frame-pointer -DTARGET_IA32 -DHOST_IA32 -DTARGET_LINUX
CFLAGS+= -DOFFSET=3

.SUFFIXES: .o .cpp

.cpp.o :
	$(CC) $(CFLAGS) -c $? -o $@

all : $(PROG)

$(PROG) : $(OBJS)
	$(CC) $^ -o $@ -lm

clean :
	rm -rf $(PROG) *.o
