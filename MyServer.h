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
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <set>
#include <map>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "CustomTypes.h"
#include "MySubscription.h"
#include "MyTopic.h"

#ifndef MY_SERVER_H
#define MY_SERVER_H

// class for the object server
class MyServer {
private:
    int portNumber, tcpSockFd, udpSockFd, lenFdSet;
    sockaddr_in tcpAddr, udpAddr;
    bool canRunServer;
    fd_set fdSet, auxFdSet;
    char myBuffer[CLIIDLEN]; 

    // Data structures used in order to achieve the most efficient solution
    std::unordered_map<std::string, MyTopicType> knownTopics;
    std::unordered_map<std::string, MySubType> subscribersData; 
    std::unordered_map<int, MySubType *> cliFds;
    std::list<MessageQuCountType> qudMessages;

    // Function used to bind the TCP connection 
    bool bindTcpSocket(const int &sockId, sockaddr_in &addr) {
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(portNumber);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (sockId < 0) {
            write(1, "Socket not opened ... maybe try again.\n", 40);
            return false;
        }

        int aux = 1;
        // Using TCP_NODELAY to disable Nagle's Algorithm
		if (setsockopt(sockId, IPPROTO_TCP, TCP_NODELAY, &aux, sizeof(int)) < 0) {
            write(1, "Couldn't disable Nagle's Algorithm for TCP connection.\n", 56);
            return false;
		}
        
        // binding
        if (bind(sockId, (sockaddr *)&addr, sizeof(addr)) < 0) {
            write(1, "Couldn't bind the socket to the port...try again later.\n", 57);
            return false;
        }
        return true;
    }

    // Function used to bind the UDP connection 
    bool bindUdpSocket(const int &sockId, sockaddr_in &addr) {
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(portNumber);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (sockId < 0) {
            write(1, "Socket not opened...maybe try again.\n", 38);
            return false;
        }
        
        // binding
        if (bind(sockId, (sockaddr *)&addr, sizeof(addr)) < 0) {
            write(1, "Couldn't bind the socket to the port...try again later.\n", 57);
            return false;
        }
        return true;
    }

    // This function gets called to inform a client that has an already in use ID
    bool sendReject(const int &whereTo, const std::string &clientId) {
        std::string strMsg;
        strMsg = "A client with the same ID (" + clientId +
            ") is already connected to the server...Sorry you' out.\n";

        TcpMsg msg;
        memset(&msg, 0, sizeof(TcpMsg));
        memcpy(&msg.content, strMsg.data(), strMsg.size());
        msg.type = 66;
        if (send(whereTo, &msg, sizeof(TcpMsg), 0) < 0) {
            return false;
        }
        return true;
    }

    // This function gets called to inform a client that has an already in use ID
    bool sendCloseSig(const int &whereTo) {
        std::string strMsg;
        strMsg = "The server is turning off\n";

        TcpMsg msg;
        memset(&msg, 0, sizeof(TcpMsg));
        memcpy(&msg.content, strMsg.data(), strMsg.size());
        msg.type = 66;
        if (send(whereTo, &msg, sizeof(TcpMsg), 0) < 0) {
            return false;
        }
        return true;
    }

public:
    // The main constructor of the class
    MyServer(const int &portNumber) {
        this->portNumber = portNumber;
        this->canRunServer = true;
        
        // Creating the sockets of the app 
        tcpSockFd = socket(PF_INET, SOCK_STREAM, 0);
        udpSockFd = socket(PF_INET, SOCK_DGRAM, 0);

        // Establishing the main connections and the listening port
        if (bindTcpSocket(tcpSockFd, tcpAddr) == false) {
            write(1, "Error on tcp connection.\n", 26);
            canRunServer = false;
            return;
        }

        if (bindUdpSocket(udpSockFd, udpAddr) == false) {
            write(1, "Error on udp connection.\n", 26);
            canRunServer = false;
            return;
        }

        // The application can hold up to 50 maximum connection requests
        if (listen(tcpSockFd, MAX_CLIENTS_QUEUE) < 0) {
            write(1, "Can't listen ... maybe next time\n", 26);
            canRunServer = false;
            return;
        }

        // Compute the FD_SET
        FD_ZERO(&fdSet);
        FD_SET(udpSockFd, &fdSet);
        FD_SET(tcpSockFd, &fdSet);
        FD_SET(0, &fdSet);

        lenFdSet = std::max(tcpSockFd, udpSockFd);
    }

    // The main destructor for may app
    ~MyServer() {
        // Safely closing the FDs not before telling the clients
        // to cut out the connection
        for (const auto &iter : cliFds) {
            sendCloseSig(iter.first);
            shutdown(iter.first, SHUT_RDWR);
            close(iter.first);
        }

        shutdown(tcpSockFd, SHUT_RDWR);
        close(tcpSockFd);

        shutdown(udpSockFd, SHUT_RDWR);
        close(udpSockFd);

        FD_ZERO(&fdSet);
    }

    // The main component of the server app
    void runServer() {
        if (canRunServer == false) {
            write(1, "Can't run server.\n", 19);
            exit(1);
        }

        sockaddr_in incomingAddr;
        socklen_t sockLen = sizeof(incomingAddr);
        int newSockFd;
        std::string topicName;

        // The main loop of the program
        while (canRunServer) {
            auxFdSet = fdSet;

            // geting the FD that is showing activity
            if (select(lenFdSet + 1, &auxFdSet, NULL, NULL, NULL) < 0) {
                write(1, "Can't perform select.\n", 23);
                return;
            }

            // case where the app is getting input from std
            if (FD_ISSET(0, &auxFdSet)) {
                memset(myBuffer, 0, CLIIDLEN);
                read(0, myBuffer, CLIIDLEN);
                if (strcmp(myBuffer, "exit\n") == 0 || strcmp(myBuffer, "quit\n") == 0) {
                    canRunServer = false;
                    break;
                } else {
                    write(1, "Invalid command\n", 26);
                }

            // case where the app is getting a connection request from a client
            } else if (FD_ISSET(tcpSockFd, &auxFdSet)) {
                sockLen = sizeof(incomingAddr);
                memset(&incomingAddr, 0, sockLen);

                // Accepting the request
                newSockFd = accept(tcpSockFd,
                    (sockaddr *)&incomingAddr,
                    &sockLen);

                if (newSockFd < 0) {
                    write(1, "Can't connect to client.\n", 26);
                    canRunServer = false;
                    break;
                }

                int aux = 1;
                // Using TCP_NODELAY to disable Nagle's Algorithm
                if (setsockopt(newSockFd, IPPROTO_TCP, TCP_NODELAY, &aux, sizeof(int)) < 0) {
                    write(1, "Couldn't disable Nagle's Algorithm for newcomer.\n", 50);
                    canRunServer = false;
                    break;
                }

                // Receiving the ID of the client (part of my protocol)
                memset(myBuffer, 0, CLIIDLEN);
                if (recv(newSockFd, myBuffer, CLIIDLEN, 0) < 0) {
                    write(1, "Didn't received client ID.\n", 28);
                    canRunServer = false;
                    break;
                }

                std::string strRecv = myBuffer;

                // Checking if the just connected client has any historic
                MySubType *pointToClient;
                const auto &sub = subscribersData.find(strRecv);

                // In case if there is no relevant historic a new object is created
                if (sub == subscribersData.end()) {
                    pointToClient =
                        &subscribersData.insert({strRecv, MySubType(strRecv, newSockFd)}).first->second;
                    cliFds.insert({newSockFd, pointToClient});

                } else {
                    // When a client with the same id is already connect the request is dropped
                    if (sub->second.isConnected()) {
                        if (sendReject(newSockFd, strRecv) == false) {
                            canRunServer = false;
                            break;
                        }
                        close(newSockFd);
                        continue;
                    } else {
                        sub->second.swichConnected();
                        sub->second.setClientFd(newSockFd);
                        pointToClient = &sub->second;
                        cliFds.insert({newSockFd, pointToClient});

                        // In case a client reconnects and has messages in it's queue
                        while (!sub->second.hasQu()) {
                            if (!sub->second.updateReconnection(qudMessages)) {
                                canRunServer = false;
                                write(1, "Failed when updating from personal queue.\n", 43);
                                break;
                            }
                        }
                    }
                }

                // Updating the set    
                FD_SET(newSockFd, &fdSet);
                if (newSockFd > lenFdSet) {
                    lenFdSet = newSockFd;
                }

                // Message displayed when a connection with
                // the client is established 
                std::cout << "New connection coming from "
                    << inet_ntoa(incomingAddr.sin_addr)
                    << ", port "
                    << ntohs(incomingAddr.sin_port)
                    << ", client " + strRecv + " on FD "
                    << newSockFd
                    << '\n';

            // Case when the server gets a message from udp client
            } else if (FD_ISSET(udpSockFd, &auxFdSet)) {
                sockLen = sizeof(incomingAddr);
                memset(&incomingAddr, 0, sockLen);

                UdpMsgStr udpMsg;
                memset(&udpMsg, 0, sizeof(UdpMsgStr));

                // Receiving the message and the information
                // about the port and Ip of the UDP client
                if (recvfrom(udpSockFd,
                    &udpMsg,
                    sizeof(UdpMsgStr),
                    0,
                    (struct sockaddr *)&incomingAddr,
                    &sockLen) < 0) {
                    write(1, "Failed to receive message from UDP cl.\n", 40);
                    continue;
                }
                // Updating the protocol for further transmission
                // And checking if there is any relevant historic regarding this topic
                topicName = udpMsg.topic;
                ++udpMsg.type;
                const auto &currentTopic = knownTopics.find(topicName);

                if (currentTopic != knownTopics.end()) {
                    TcpMsg msg;
                    memset(&msg, 0, sizeof(msg));

                    memcpy(msg.ip,
                        inet_ntoa(incomingAddr.sin_addr),
                        strlen(inet_ntoa(incomingAddr.sin_addr)));

                    // In case there ar subscribers for this topic
                    // Those that are online will get updated immediately
                    memcpy(msg.content, udpMsg.content, CONTENTLEN);
                    memcpy(msg.topic, udpMsg.topic, TOPICLEN);
                    msg.type = udpMsg.type;
                    msg.port = incomingAddr.sin_port;

                    // The Update function from topic class will return a list of those
                    // subscribers that need to be updated when they reconnect
                    std::list<MySubType *> auxList =
                        currentTopic->second.updateSubscribers(msg, canRunServer);

                    if (!auxList.empty() && canRunServer == true) {
                        // In case there are, the message is put to the general queue
                        // and the client gets added a pointer(itterator) in the list
                        // within his object
                        std::list<MessageQuCountType>::iterator qudMsg;
                        qudMessages.push_back(MessageQuCountType(msg, auxList.size()));
                        qudMsg = qudMessages.end();
                        qudMsg--;

                        for (const auto &it : auxList) {
                            it->addToPersonalQu(qudMsg);
                        }
                    }
                }
            // Case where a client sent a command to the server
            } else {
                for (const auto &iter : cliFds) {
                    // Having a map of FDs with pointer to a client and
                    // finding out which FDs has activity
                    if (FD_ISSET(iter.first, &auxFdSet)) {
                        ClientSubMsg msg;
                        memset(&msg, 0, sizeof(msg));

                        // Checking what commnad is coming
                        if (recv(iter.first, &msg, sizeof(msg), 0) < 0) {
                            write(1, "Didn't received client ID.\n", 28);
                            canRunServer = false;
                            break;
                        }
                        MySubType *clientData = iter.second;

                        // In case of an empty message that means there was
                        // a mallfunction
                        // on this FD and the connection is suspend
                        if (msg.type == 0) {
                            std::cout << "Client using FD "
                                    << iter.first
                                    << " disconnected unexpectedly.\n";
                            clientData->swichConnected();
                            shutdown(iter.first, SHUT_RDWR);
                            close(iter.first);
						    FD_CLR(iter.first, &fdSet);
                            cliFds.erase(iter.first);

                            break;
                        }

                        // The client sent this message to inform the server
                        // that he is loging out
                        if (msg.type == 255) {
                            std::cout << "Client "
                                    << clientData->getClientId()
                                    << " disconnected.\n";
                            shutdown(iter.first, SHUT_RDWR);
                            close(iter.first);
                            clientData->swichConnected();
                            // In case the client has 
						    FD_CLR(iter.first, &fdSet);
                            cliFds.erase(iter.first);
                            // If the client that just logged out has no
                            // subscriptions in any place that means the
                            // server doesn't need to retain his data anymore
                            if (clientData->safeDelete()) {
                                subscribersData.erase(clientData->getClientId());
                            }
                            break;
                        }

                        std::string topic = msg.topic;
                        std::string cliId = clientData->getClientId();

                        // Finding the topic on which client issued a command
                        MyTopicType *topicData = NULL;
                        auto const &topicAux = knownTopics.find(topic);

                        // Case for subscribe and unsubscribe
                        if (msg.type == 2) {
                            // In case the topic of interess had no subscribers
                            // till this point then a new object is created
                            if (topicAux == knownTopics.end()) {
                                topicData =
                                    &knownTopics.insert({topic, MyTopicType(topic)}).first->second;
                            } else {
                                topicData = &topicAux->second;
                            }

                            clientData->addSubscritions(topic, msg.sfOrNot);
                            topicData->addSubscriber(cliId, clientData);

                        } else if (topicAux != knownTopics.end()) {
                            topicData = &topicAux->second;
                            clientData->removeSubscritions(topic);
                            
                            // In case there are no more subscribers for
                            // this topic the it can be erased from the
                            // program memory 
                            if (topicData->removeSubscriber(cliId)) {
                                knownTopics.erase(topicAux);
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
};

#endif