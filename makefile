all:
	g++ -g --std=c++11 -Wall -Werror -pedantic -o bool main.cpp

test:
	g++ -g --std=c++11 -Wall -Werror -pedantic -o bool-test test.cpp
	./bool-test 1000

clean:
	rm -f bool-test bool
