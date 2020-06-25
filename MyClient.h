#include <iostream>
#include <stdlib.h>
#include <cstring>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <cmath>
#include <iomanip>
#include "CustomTypes.h"

#ifndef MY_CLIENT_H
#define MY_CLIENT_H


// class for the object client
class MyTcpClient {
private:
    int portNumber, tcpSockFd, lenTcp;
	in_addr ipClient;
    sockaddr_in tcpAddr;
    bool canRunClient;
    fd_set tcpSet, auxTcpSet;
    char myBuffer[BUFLEN], clientId[CLIIDLEN];

    // The function used to process a message type from server
    void printMessage(const TcpMsg &msg) {
        std::cout << msg.ip
                << ':'
                << ntohs(msg.port)
                << " - "
                << msg.topic
                << " - ";


        // Those are meant to be displayed on std::output
        // Checking the matching type from my protocol
        // and computing the output message fo the just
        // received msg
        if (msg.type == 1) {
            int sig = msg.content[0];
            unsigned int res = 0;
            if (sig) {
                sig = -1;
            } else {
                sig = 1;
            }

            memcpy(&res, msg.content + 1, 4);

            std::cout << "INT - "
                    << (long)ntohl(res) * sig;

        } else if (msg.type == 2) {
            unsigned short int res = 0;

            memcpy(&res, msg.content, 2);

            std::cout << "SHORT_REAL - "
                    << std::fixed
                    << std::setprecision(2)
                    << (float)ntohs(res) / 100.00;

        } else if (msg.type == 3) {
            int sig = msg.content[0];
            unsigned int res = 0;
            int power = msg.content[5];
            if (sig) {
                sig = -1;
            } else {
                sig = 1;
            }

            memcpy(&res, msg.content + 1, 4);

            std::cout << "FLOAT - "
                    << std::fixed
                    << std::setprecision(power)
                    << sig * (double)ntohl(res) / (float)pow(10, power);

        } else if (msg.type == 4) {
            std::cout << "STRING - "
                    << msg.content;
        }
        std::cout << '\n';
    }


public:
    // The main constructor for the client object
    MyTcpClient(const int &portNumber, char *ipAddr, const char *clientId) {
        memset(this->clientId, 0, 11);
        this->portNumber = portNumber;
        this->ipClient = ipClient;
        strcpy(this->clientId, clientId);
        canRunClient = true;

        // Creating the sockets of the app 
        tcpSockFd = socket(PF_INET, SOCK_STREAM, 0);

        memset(&tcpAddr, 0, sizeof(tcpAddr));
        tcpAddr.sin_family = AF_INET;
        tcpAddr.sin_port = htons(portNumber);
        inet_aton(ipAddr, &(tcpAddr.sin_addr));

        if (tcpSockFd < 0) {
            write(1, "Can't open the socket.\n", 24);
            canRunClient = false;
            return;
        }

        int aux = 1;
        // Using TCP_NODELAY to disable Nagle's Algorithm
		if (setsockopt(tcpSockFd, IPPROTO_TCP, TCP_NODELAY, &aux, sizeof(int)) < 0) {
            write(1, "Couldn't disable Nagle's Algorithm.\n", 37);
            canRunClient = false;
            return;
		}

        // Function used to connect the TCP sok to the server of the app 
        if (connect(tcpSockFd, (sockaddr *)&tcpAddr, sizeof(tcpAddr)) < 0) {
            write(1, "Can't establish connection.\n", 29);
            canRunClient = false;
            return;
        }

        // Compute the FD_SET
        FD_ZERO(&tcpSet);
        FD_SET(0, &tcpSet);
        FD_SET(tcpSockFd, &tcpSet);
        lenTcp = tcpSockFd;

        // Sending the client ID to the server
        if (send(tcpSockFd, this->clientId, CLIIDLEN, 0) < 0) {
            write(1, "Failed to send client id.\n", 27);
            canRunClient = false;
            return;
        }

        std::cout << "Your client ID is: "
                << this->clientId << '\n'; 
    }

    // The main destructor for may app
    ~MyTcpClient() {
        // Safely closing the FDs
        shutdown(tcpSockFd, SHUT_RDWR);
        close(tcpSockFd);
        FD_ZERO(&tcpSet);
    }

    // The main component of the client app
    void runClient() {
        if (canRunClient == false) {
            write(1, "Can't run client.\n", 19);
            exit(1);
        }

        // The main loop of the program
        while (canRunClient) {
            auxTcpSet = tcpSet;

            // geting the FD that is showing activity
            if (select(lenTcp + 1, &auxTcpSet, NULL, NULL, NULL) < 0) {
                write(1, "Can't perform select.\n", 23);
                return;
            }

            // case where the app is getting input from std
            if (FD_ISSET(0, &auxTcpSet)) {
                memset(myBuffer, 0, 80);
                read(0, myBuffer, 80);
                ClientSubMsg msg;
                memset(&msg, 0, sizeof(msg));

                // The case when the client wants to log out
                if (strcmp(myBuffer, "exit\n") == 0 || strcmp(myBuffer, "quit\n") == 0) {
                    msg.type = 255;
                    
                    if (send(tcpSockFd, &msg, sizeof(msg), 0) < 0) {
                        write(1, "Failed to send closing commnad.\n", 33);
                        return;
                    }
                    break;
                }

                // processing the commnad from std::input
                std::string strBuffer;
                std::vector<std::string> words;
                bool canSend = true;
                char *pointChar = strtok(myBuffer, " ");

                // Separating the words
                while (pointChar) {
                    strBuffer = pointChar;
                    words.push_back(strBuffer);
                    pointChar = strtok(NULL, " \n\0");
                    if (words.size() >= 4) {
                        break;
                    }
                }

                // The folowing lines are are checking the type
                // of the command (subscribe or unsubscribe)
                if (words.size() == 3 || words.size() == 2) {
                    if (words[1].size() > TOPICLEN) {
                        canSend = false;
                        write(1, "The topic name is not valid\n", 29);
                    } else if (words[0] == "subscribe") {
                        if (words.size() != 3) {
                            canSend = false;
                            write(1, "SF not specified...use 0/1, please.\n", 37);
                        } else {
                            if (words[2][0] == '0' || words[2][0] == '1') {
                                msg.type = 2;
                                msg.sfOrNot = words[2][0] - '0';
                                strcpy(msg.topic, words[1].data());
                            } else {
                                canSend = false;
                                write(1, "Wrong use of SF argument...use 0/1, please.\n", 45);
                            }
                        }
                    } else if (words[0] == "unsubscribe") {
                        msg.type = 1;
                        msg.sfOrNot = 0;
                        strcpy(msg.topic, words[1].data());
                    } else {
                        canSend = false;
                        write(1, "Commnad undefined.\n", 20);
                    }
                } else {
                    canSend = false;
                    write(1, "Wrong number arguments.\n", 25);
                }

                if (canSend == false) {
                    continue;
                }

                // Letting the server know what is the request of the client
                if (send(tcpSockFd, &msg, sizeof(msg), 0) < 0) {
                    write(1, "Failed to send the message.\n", 29);
                    return;
                } else {
                    if (words[0] == "subscribe") {
                        std::cout << "Subscribed to topic: "
                                << words[1]
                                << '\n';
                    } else {
                        std::cout << "Unsubscribed from topic: "
                                << words[1]
                                << '\n';
                    }
                }
            } else {
                // On this case the client is checking eather
                // the server is bringing updates on a specific topic
                // or eather the server is issueing another type of comand
                TcpMsg msg;
                memset(&msg, 0, sizeof(TcpMsg));
                if (recv(tcpSockFd, &msg, sizeof(TcpMsg), 0) < 0) {
                    write(1, "Failed when receiving the message.\n", 36);
                    break;
                }

                // in case of unusual behaviour on the FD
                // the connection is suspended
                // client is forced to shut down 
                if (msg.type == 0) {
                    write(1, "Something might have happened to the server.\nDisconnecting...\n", 63);
                    break;
                }

                // For this type the client is commanded to close
                // as the server eather is shuting down or the
                // client ID is already in use
                if (msg.type == 66) {
                    std::cout << msg.content;
                    break;
                }
                
                if (msg.type == 1 || msg.type == 2 || msg.type == 3 || msg.type == 4) {
                    printMessage(msg);
                    continue;
                }

                // For this case the client is preparing to capture a large
                // amount of messages as he reconnected and is about to
                // receive the content of his personal queue 
                if (msg.type == 10) {
                    unsigned int count = 0;
                    mempcpy(&count, msg.content, 4);
                    count = ntohl(count);

                    TcpMsg *msgArr = (TcpMsg *)malloc(sizeof(TcpMsg) * count);

                    if (recv(tcpSockFd, msgArr, count * sizeof(TcpMsg), 0) < 0) {
                        write(1, "Failed when receiving the SF array.\n", 37);
                        free(msgArr);
                        break;
                    }
                    
                    for (unsigned int i = 0; i < count; ++i) {
                        if (msgArr[i].type == 1 || msgArr[i].type == 2 ||
                            msgArr[i].type == 3 || msgArr[i].type == 4) {
                            printMessage(msgArr[i]);
                        } else {
                            break;
                        }
                    }

                    free(msgArr);
                }
            }
        }
    }
};

#endif
