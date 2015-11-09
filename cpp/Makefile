.PHONY: clean all install
CC = g++


	LDFLAGS +=-lboost_program_options -lboost_filesystem -lboost_system -lboost_iostreams -lpthread -lboost_mpi -lmpi -lmpi_cxx  -L../../../auryn/build/src -lauryn -L${MPI_LIB}
	CXXFLAGS +=  -I/usr/include -I../../../auryn/src/ -I${MPI_INCLUDE} -std=c++11 


all: postprocess_single postprocess_multisim grapher_single

postprocess_single: postprocess_single.cpp utils.h
	$(CC) $(CXXFLAGS) postprocess_single.cpp -o postprocess_single $(LDFLAGS)

postprocess_multisim: postprocess_multisim.cpp utils.h
	$(CC) $(CXXFLAGS) postprocess_multisim.cpp -o postprocess_multisim $(LDFLAGS)

grapher_single: grapher_single.cpp utils.h
	$(CC) $(CXXFLAGS) grapher_single.cpp -o grapher_single $(LDFLAGS)

install: postprocess_single grapher_single
	mkdir -p ../../../bin/
	cp -v postprocess_single ../../../bin/postprocess_single
	cp -v grapher_single ../../../bin/grapher_single
	cp -v postprocess_multisim ../../../bin/postprocess_multisim
clean:
	rm -fv postprocess_single ../../../bin/postprocess_single
	rm -fv postprocess_multisim ../../../bin/postprocess_multisim
	rm -fv grapher_single ../../../bin/grapher_single
