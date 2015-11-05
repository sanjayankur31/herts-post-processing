.PHONY: clean all install
CC = g++


	LDFLAGS +=-lboost_program_options -lboost_filesystem -lboost_system -lboost_iostreams -lpthread -lboost_mpi -lmpi -lmpi_cxx  -L../../../auryn/build/src -lauryn -L${MPI_LIB}
	CXXFLAGS +=  -I/usr/include -I../../../auryn/src/ -I/usr/include/mpich-x86_64 -I${MPI_INCLUDE} -std=c++11 


all: postprocess_single

postprocess_single: main.cpp utils.h
	$(CC) $(CXXFLAGS) main.cpp -o postprocess_single $(LDFLAGS)

install: postprocess_single
	mkdir -p ../../../bin/
	cp -v postprocess_single ../../../bin/
clean:
	rm -fv postprocess_single ../../../bin/postprocess_single
