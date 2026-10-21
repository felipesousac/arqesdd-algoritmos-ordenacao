EMCC ?= emcc
CFLAGS = -O2 -std=c11 -Wall -Wextra
EXPORTS = "['_malloc','_free','_sort_run','_sort_values','_sort_metrics','_sort_operations','_sort_free']"

.PHONY: all wasm serve clean native
all: wasm

# `make` remains useful without Emscripten: the UI has a JS demonstration mode.
wasm:
	@if ! command -v $(EMCC) >/dev/null 2>&1; then \
		echo "Emscripten (emcc) não encontrado. A interface pode rodar em modo demonstração; veja README.md para instalar o SDK."; \
		exit 0; \
	fi
	$(MAKE) web/sort_engine.js EMCC='$(EMCC)'

serve:
	python3 -m http.server 8000

web/sort_engine.js: src/sort_engine.c
	$(EMCC) $(CFLAGS) $< -o $@ -s MODULARIZE=1 -s EXPORT_NAME=SortEngine -s ENVIRONMENT=web -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS=$(EXPORTS) -s EXPORTED_RUNTIME_METHODS='[]'

native: build/sort_engine

build/sort_engine: src/sort_engine.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f web/sort_engine.js web/sort_engine.wasm build/sort_engine
