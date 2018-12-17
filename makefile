all: 
	g++ -g --std=c++11 -Wall -Werror -pedantic -o form form.cpp
	#g++ -g --std=c++11 -Wall -Werror -pedantic -o bool main.cpp

sets:
	g++ -g --std=c++11 -Wall -Werror -pedantic -o set sets.cpp

test:
	g++ -g --std=c++11 -Wall -Werror -pedantic -o bool-test test.cpp
	./bool-test 1000

form:
	g++ -g --std=c++11 -Wall -Werror -pedantic -o form form.cpp

clean:
	rm -f bool-test bool form
