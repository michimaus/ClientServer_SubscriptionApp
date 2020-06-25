#include <iostream>
#include <cstring>
#include <stdio.h>
#include <fstream>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "MyClient.h"


int main(int argc, char *argv[]) {
	int connectionPort = 0;
	in_addr ipClient;
	std::string clientId;

    // checking if there are the required arguments and their validity
	if (argc < 4) {
		write(1, "Introduce the Id, IP addres and port number as arguments.\n", 59);
		exit(1);
	} else {
		connectionPort = atoi(argv[3]);
		std::string checkPort1, checkPort2(argv[3]);
		checkPort1 = std::to_string(connectionPort);

		if (checkPort1 != checkPort2) {
			write(1, "Third argument is not a valid port number.\n", 44);
			exit(1);
		}
		if (inet_aton(argv[2], &ipClient) < 0) {
			write(1, "Second argument is not a valid port number.\n", 45);
			exit(1);
		}
	}

	// if the client ID is not valid (has more than 10 characters) the it gets shortened.
	clientId = argv[1];
	if (clientId.size() > 10) {
		clientId = clientId.substr(0, 10);
		write(1, "Client ID got shortened to only use the first 10 characters\n", 61);
	}

	// Creating the object for the client and running it
	// see "MyClient.h" for further implementation details
	MyTcpClient myTcpClient(connectionPort, argv[2], clientId.data());
	myTcpClient.runClient();

	return 0;
}
