.PHONY: all

all:
	rm -rf build
	cmake -B build
	cmake --build build
	cp build/src/mysh .