

all: none
	echo "use cmake to build"


format:
	clang-format -i -style=file ./include/*h
	clang-format -i -style=file ./src/*cc
	clang-format -i -style=file ./tools/*cc
	clang-format -i -style=file ./test/*cc


.PHONY: build


build-manually:
	cd lib/isobmff/ && make && cd ../../
	LD_LIBRARY_PATH=$${LD_LIBRARY_PATH}:./lib/isobmff/Build/Debug/Products/x86_64/ g++ -c -o src/liblcvm.o -g -O0 src/liblcvm.cc -I./lib/isobmff/ISOBMFF/include/
	LD_LIBRARY_PATH=$${LD_LIBRARY_PATH}:./lib/isobmff/Build/Debug/Products/x86_64/ g++ -c -o tools/lcvm.o -g -O0 tools/lcvm.cc -I./lib/isobmff/ISOBMFF/include/ -I./include
	LD_LIBRARY_PATH=$${LD_LIBRARY_PATH}:./lib/isobmff/Build/Debug/Products/x86_64/ g++ -o tools/lcvm tools/lcvm.o src/liblcvm.o -g -O0 -L./lib/isobmff/Build/Debug/Products/x86_64/ -lISOBMFF


build-policy:
	\rm -rf build
	mkdir build
	cd build && cmake -DCMAKE_BUILD_TYPE=DEBUG -DADD_POLICY=ON ..
	cd build && make -j


ifeq ($(TESTDIR),)
TESTDIR := /tmp
endif

test:
	LD_LIBRARY_PATH=$${LD_LIBRARY_PATH}:./build/lib/isobmff/src/ ./build/lcvm ./lib/isobmff/media/*MOV -o /tmp/full.csv


build-clang:
	\rm -rf build
	mkdir build
	cd build && CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=DEBUG ..
	cd build && make -j 8

build-gcc:
	\rm -rf build
	mkdir build
	cd build && CC=gcc CXX=g++ cmake -DCMAKE_BUILD_TYPE=DEBUG ..
	cd build && make -j 8


# Strict Debug builds (CMakeLists.txt warning flags plus -Werror) with gcc and
# clang, followed by the tests. JOBS sets the build parallelism.
JOBS ?= 4

check-debug:
	for cc in gcc:g++ clang:clang++; do \
	  dir=build/debug-$${cc%%:*}; \
	  \rm -rf $$dir; \
	  cmake -S . -B $$dir -DCMAKE_BUILD_TYPE=Debug \
	    -DCMAKE_C_COMPILER=$${cc%%:*} -DCMAKE_CXX_COMPILER=$${cc##*:} \
	    -DADD_C_INTERFACE=ON && \
	  cmake --build $$dir --parallel $(JOBS) && \
	  ctest --test-dir $$dir --output-on-failure || exit 1; \
	done
