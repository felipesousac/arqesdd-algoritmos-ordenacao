CFLAGS = -O2 -std=c11 -D_GNU_SOURCE -Wall -Wextra
CPPFLAGS = -Isrc -Ivendor
LDLIBS ?=

.PHONY: all serve clean native
all: native

native: build/ordena-server

build/ordena-server: src/server.c src/sort_engine.c src/sort_engine.h src/tad/headerFiles/listaSequencial.h src/tad/sourceFiles/listaSequencial.c vendor/mongoose.c vendor/mongoose.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ src/server.c src/sort_engine.c src/tad/sourceFiles/listaSequencial.c vendor/mongoose.c $(LDLIBS)

serve: build/ordena-server
	./build/ordena-server

clean:
	rm -f build/ordena-server
