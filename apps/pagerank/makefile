#SOURCES = $(wildcard *.cpp)
#OBJECTS := $(SOURCES:.cpp=.o)

CXXFLAGS = -ggdb -Wall -pipe -fPIC -O0 -std=c++11
LDFLAGS = -lpthread -lprotobuf -lglog -lgflags #-lprofiler

all : pagerank

pagerank : $(OBJECTS)
	clang++ -o pr pagerank.cpp $(CXXFLAGS) $(LDFLAGS)

#%.o :  %.cpp
#	mpic++ -c -o $@ $^ $(CXXFLAGS)

clean :
	rm -f pr *.o
