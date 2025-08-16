

all: none
	echo "use cmake to build"


lint:
	clang-format -i -style=google ./include/*h
	clang-format -i -style=google ./src/*cc
	clang-format -i -style=google ./tools/*cc
	clang-format -i -style=google ./test/*cc


.PHONY: build


build:
	cd lib/isobmff/ && make && cd ../../
	LD_LIBRARY_PATH=$${LD_LIBRARY_PATH}:./lib/isobmff/Build/Debug/Products/x86_64/ g++ -c -o src/liblcvm.o -g -O0 src/liblcvm.cc -I./lib/isobmff/ISOBMFF/include/
	LD_LIBRARY_PATH=$${LD_LIBRARY_PATH}:./lib/isobmff/Build/Debug/Products/x86_64/ g++ -c -o tools/lcvm.o -g -O0 tools/lcvm.cc -I./lib/isobmff/ISOBMFF/include/ -I./include
	LD_LIBRARY_PATH=$${LD_LIBRARY_PATH}:./lib/isobmff/Build/Debug/Products/x86_64/ g++ -o tools/lcvm tools/lcvm.o src/liblcvm.o -g -O0 -L./lib/isobmff/Build/Debug/Products/x86_64/ -lISOBMFF


ifeq ($(TESTDIR),)
TESTDIR := /tmp
endif

test:
	LD_LIBRARY_PATH=$${LD_LIBRARY_PATH}:./build/lib/isobmff/src/ ./build/lcvm ./lib/isobmff/media/*MOV -o /tmp/full.csv


build-cmake:
	\rm -rf build
	mkdir build
	cd build && CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_C_FLAGS_DEBUG="-g -O0 -Wall -Wextra -Wno-unused-parameter -Wshadow -Werror" -DCMAKE_CXX_FLAGS_DEBUG="-g -O0 -Wall -Wextra -Wno-unused-parameter -Wshadow -Werror" ..
	cd build && make -j 8

