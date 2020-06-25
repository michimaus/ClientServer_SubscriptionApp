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
#include "MyServer.h"

int main (int argc, char *argv[]) {
    int connectionPort = 0;

    // checking if there are the required arguments and their validity
    if (argc < 2) {
        write(1, "Introduce the port number as argument.\n", 40);
        exit(1);
    } else {
        connectionPort = atoi(argv[1]);
        std::string checkPort1, checkPort2(argv[1]);
        checkPort1 = std::to_string(connectionPort);
        
        if (checkPort1 != checkPort2) {
            write(1, "The introduced argument is not a valid port number.\n", 53);
            exit(1);
        }
    }

    // Creating the object for the server and running it
	// see "MyServer.h" for further implementation details
    MyServer myServer(connectionPort);
    myServer.runServer();

    write(1, "The server turned of.\n", 23);
    return 0;
}