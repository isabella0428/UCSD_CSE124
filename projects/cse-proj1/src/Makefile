
CXX=g++
CXXFLAGS=-std=c++11 -ggdb -Wall -Wextra -pedantic -Werror -Wnon-virtual-dtor -I../dependencies/include
SERVEROBJS= server-main.o logger.o HttpdServer.o
CLIENTOBJS= client-main.o client.o

default: httpd client

%.o: %.c
	$(CXX) $(CXXFLAGS) -c -o $@ $<

httpd: $(SERVEROBJS)
	$(CXX) $(CXXFLAGS) -o httpd $(SERVEROBJS) -L../dependencies/lib -pthread

client: $(CLIENTOBJS)
	$(CXX) $(CXXFLAGS) -o client $(CLIENTOBJS)

.c.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f httpd *.o
