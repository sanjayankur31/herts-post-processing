.PHONY: clean all install
CC = g++


	LDFLAGS=-lboost_program_options -lboost_filesystem -lboost_system -lboost_iostreams -lpthread
	CXXFLAGS= -g -pipe -Wall -Werror=format-security -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector-strong --param=ssp-buffer-size=4 -grecord-gcc-switches -m64 -mtune=generic -std=c++11 -O3 -ffast-math -pedantic -I/usr/include

all: postprocess

postprocess: main.cpp utils.h
	$(CC) $(CXXFLAGS) main.cpp -o postprocess $(LDFLAGS)

install: postprocess
	mkdir -p ../../../bin/
	cp -v postprocess ../../../bin/
clean:
	rm -fv postprocess ../../../bin/postprocess
