include VERSION
OSRLS := $(shell head -n 1 /etc/os-release|grep NAME|cut -d"\"" -f2)
INCLUDE=-I../include
ifeq ($(OSRLS), Ubuntu) 
  INCLUDE += -I/usr/include/hdf5/serial -I/usr/include/jsoncpp/
endif
OBJS=dzfile-api.o compress-zlib.o compress-zstd.o  compress-fpga.o srai_accel_utils.o dz-common.o dz-sqlite-ifce.o dz-mq.o dzfile-intern.o sparsehash-map.o
DOBJS=
SHOBJS=libdzfile.so
PROGS=dzfile-cat dzfile-get dzfile-put dzfile-ls dzfile-info dzfile-walker dzfile-convert qztest dzfile
TESTS=ziptest
LIBS=-lz -lpthread -lzmq -lsqlite3 -lzstd -lhdf5
ifeq ($(OSRLS), Ubuntu)
  LIBS += -L/usr/local/hdf5/lib/
endif
DLIBS=-ljsoncpp
CC = gcc 
CXX = g++
DBFLG=-DUSE_SQLITE
CFLAGS=-g -O0 -fPIC -D_LARGEFILE64_SOURCE ${DBFLG} ${INCLUDE}
CXXFLAGS =-g -fPIC ${INCLUDE}
DFLAGS=-DVERSION=\"$(VERSION)\"  -DRELEASE=\"$(RELEASE)\"
SHLIBLDFLAGS = -shared
all: ${SHOBJS} ${PROGS} ${TESTS}
dzfile-cat: ${OBJS} dzfile-cat.o
	${CXX} -o $@ $^ ${LIBS}
dzfile-get: ${OBJS} dzfile-get.o
	${CXX} -o $@ $^ ${LIBS}
dzfile-put: ${OBJS} dzfile-put.o
	${CXX} -o $@ $^ ${LIBS}
dzfile-ls: ${OBJS} dzfile-ls.o
	${CXX} -o $@ $^ ${LIBS}
dzfile-convert: ${OBJS} dzfile-convert.o
	${CXX} -o $@ $^ ${LIBS}
dzfile-walker: ${OBJS} ${DOBJS} dzfile-walker.o
	${CXX} -o $@ $^ ${LIBS} ${DLIBS} 
dzfile-info: ${OBJS} dzfile-info.o
	${CXX} -o $@ $^ ${LIBS}
dzfile: dzfile.c
	${CXX} -DVERSION=\"$(VERSION)\"  -DRELEASE=\"$(RELEASE)\" -o $@ $^
ziptest: ${OBJS} ziptest.o
	${CXX} -o $@ $^ ${LIBS}
qztest: ${OBJS} qztest.o
	${CXX} -o $@ $^ ${LIBS}

libdzfile.so: ${OBJS}
	${CXX} -O2 -shared -o $@ $^
clean:
	rm -rf *.o *.so ${OBJS} ${SHOBJS} ${PROGS}
