LIB_PATH = .

HEADER_PATH = ./api_lib/IBJts/source/cppclient/client

LDFLAGS = -L$(LIB_PATH) -Wl,-rpath,$(LIB_PATH) -ltwsapi -lbid -lncurses

program: clean globals.o table.o terminal.o optionChainManager.o my_wrapper.o main.o 
	g++ -g globals.o table.o terminal.o optionChainManager.o my_wrapper.o main.o -o program $(LDFLAGS)
	rm -f *.o 

main.o: main.cpp
	g++ -c main.cpp -I $(HEADER_PATH)

my_wrapper.o: my_wrapper.cpp
	g++ -c my_wrapper.cpp -I $(HEADER_PATH)

optionChainManager.o: optionChainManager.cpp
	g++ -c optionChainManager.cpp -I $(HEADER_PATH)

globals.o: globals.cpp
	g++ -c globals.cpp -I $(HEADER_PATH)

terminal.o: terminal.cpp
	g++ -c terminal.cpp 

table.o: table.cpp
	g++ -c table.cpp -I $(HEADER_PATH)

clean:
	rm -f program *.o
