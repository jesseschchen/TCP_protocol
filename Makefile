CXX=g++
CXXOPTIMIZE= -O2
CXXFLAGS= -g -Wall -pthread -std=c++11 $(CXXOPTIMIZE)
USERID=204578044
CLASSES=


default: server.cpp client.cpp
	$(CXX) -o server  $(CXXFLAGS) server.cpp packetMaker.cpp
	$(CXX) -o client  $(CXXFLAGS) client.cpp packetMaker.cpp

all: server client

server: server.cpp server.h packetMaker.cpp
	$(CXX) -o server  $(CXXFLAGS) server.cpp packetMaker.cpp

client: client.cpp client.h packetMaker.cpp
	$(CXX) -o client  $(CXXFLAGS) client.cpp packetMaker.cpp

packet: packet.h
	$(CXX) $(CXXFLAGS) packet.h

clean:
	rm -rf *.o *~ *.gch *.swp *.dSYM server client *.tar.gz

dist: tarball
tarball: clean
	tar -cvf /tmp/$(USERID).tar.gz --exclude=./.vagrant . && mv /tmp/$(USERID).tar.gz .
