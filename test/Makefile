PROG=check zmqcli zmqsrv sparsehash
all: ${PROG}
check: check.c
	gcc -g -o $@ $^ 
zmqsrv: zmqsrv.c
	gcc -g -o $@ $^ -lzmq 
zmqcli: zmqcli.c
	gcc -g -o $@ $^ -lzmq 
sparsehash: sparsehash.cc
	g++ -g -o $@ $^ 
clean:
	rm -rf *.o ${PROG}
