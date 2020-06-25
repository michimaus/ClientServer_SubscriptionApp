build: server subscriber

subscriber:
	g++ -Wall -Wextra -std=c++11 subscriber.cpp -o subscriber

server:
	g++ -Wall -Wextra -std=c++11 server.cpp -o server

clean:
	rm -rf server subscriber
