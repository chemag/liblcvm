

# Target
TARGET := antlr.main

all: $(TARGET)

# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O0 -g -I/usr/include/antlr4-runtime -Igen/
CXXFLAGS += -Wno-overloaded-virtual
LDFLAGS := -lantlr4-runtime -lprotobuf


# Protobuf compiler
PROTOC := protoc
PROTO_SRC := rules.proto
PROTO_GEN_SRCS := gen/rules.pb.cc gen/rules.pb.h

# Generate protobuf sources
$(PROTO_GEN_SRCS): $(PROTO_SRC)
	$(PROTOC) --cpp_out=gen/ $(PROTO_SRC)


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


OBJS := $(SRCS:.cpp=.o)

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

realclean: clean
	rm -f gen/*

format:
	clang-format -i -style=google antlr.main.cpp antlr.protovisitor.cpp antlr.protovisitor.h
