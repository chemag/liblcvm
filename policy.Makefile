
# TODO(marko): This Makefile should be integrated into the root (and autogen?) CMakeLists.txt file, after fixing all paths

# Target
TARGET := antlr.main

all: $(TARGET)

# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O0 -g -I/usr/include/antlr4-runtime -Igen/ -I/opt/homebrew/include $(shell pkg-config --cflags protobuf) -I/usr/local/include/antlr4-runtime
CXXFLAGS += -Wno-overloaded-virtual
LDFLAGS := $(shell pkg-config --libs protobuf) -lantlr4-runtime


# Protobuf compiler
PROTOC := protoc
PROTO_SRC := rules.proto
PROTO_GEN_SRCS := gen/rules.pb.cc gen/rules.pb.h

# Generate protobuf sources
$(PROTO_GEN_SRCS): $(PROTO_SRC)
	$(PROTOC) --cpp_out=gen/ $(PROTO_SRC)

gen/rules.pb.o: gen/rules.pb.cc gen/rules.pb.h

antlr.main.o: gen/rules.pb.h

# Source files
SRCS := antlr.main.cpp \
        antlr.protovisitor.cpp \
        gen/rules.pb.cc \
        gen/rulesLexer.cpp \
        gen/rulesParser.cpp \
        gen/rulesBaseListener.cpp \
        gen/rulesListener.cpp \
        gen/rulesBaseVisitor.cpp \
        gen/rulesVisitor.cpp


OBJS := $(patsubst %.cpp,%.o,$(filter %.cpp,$(SRCS))) \
        $(patsubst %.cc,%.o,$(filter %.cc,$(SRCS)))


gen/rulesLexer.interp: rules.g4
	java -Xmx500M -cp antlr-4.13.1-complete.jar org.antlr.v4.Tool -Dlanguage=Cpp -visitor -o gen $<



$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(ANTLR4_FLAGS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(ANTLR4_FLAGS)

gen/%.o: gen/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(ANTLR4_FLAGS)


clean:
	rm -f $(TARGET) $(OBJS)
	rm -f $(PROTO_GEN_SRCS)

realclean: clean
	rm -f gen/*

format:
	clang-format -i -style=google antlr.main.cpp antlr.protovisitor.cpp antlr.protovisitor.h
