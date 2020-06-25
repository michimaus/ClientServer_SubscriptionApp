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
#include <unordered_map>
#include <map>
#include <list>
#include <string>
#include <queue>
#include "CustomTypes.h"
#include "MessageQuCount.h"

#ifndef MY_SUBSCRIPTION_H
#define MY_SUBSCRIPTION_H


// This is the class for the subscribers' data
// It is only instantiated on the server part
// It stores information about the topics the client is subscribed to
class MySubType {
private:
    std::string clientId;
    int clientFd;
    bool connected;

    // Data structures used by the client
    std::queue<std::list<MessageQuCountType>::iterator> personalQu;
    std::unordered_map<std::string, char> subsType;

public:
    // The main constructor of the client
    MySubType(const std::string &name, const int &clientFd) {
        this->clientId = name;
        this->connected = true;
        this->clientFd = clientFd;
    }

    std::string getClientId() {
        return this->clientId;
    }

    int getClientFd() {
        return this->clientFd;
    }

    // This function checks if the subscriber is connected
    // in case it is the message is sent
    bool performUpdate(const TcpMsg &msg, bool &canRunServer) {
        if (this->connected) {
            if (send(this->clientFd, &msg, sizeof(msg), 0) < 0) {
                canRunServer = false;
                return false;
            }
            return false;
        } else {
            // In case he is offline then his subscriptions are checked to see
            // if the message is going to be stored by the server or not
            std::string auxStr = msg.topic;
            auto const &sub = subsType.find(auxStr);
            if (sub->second == 1) {
                return true;
            }
            return false;
        }
    }

    // The personal que is made up from iterator of the general
    // queue of the server
    void addToPersonalQu(const std::list<MessageQuCountType>::iterator &qudMsg) {
        personalQu.push(qudMsg);
    }

    bool safeDelete() {
        if (subsType.empty()) {
            return true;
        }
        return false;
    }

    bool isConnected() {
        return this->connected;
    }

    void setClientFd(const int &clientFd) {
        this->clientFd = clientFd;
    }

    void swichConnected() {
        this->connected = !this->connected;
    }

    void safeDisconnectFd() {
        this->clientFd = -1;
    }

    void addSubscritions(std::string &topicName, const char &sfType) {
        auto const &sub = subsType.find(topicName);
        if (sub == subsType.end()) {
            subsType.insert({topicName, sfType});
        } else {
            sub->second = sfType;
        }
    }

    void removeSubscritions(std::string &topicName) {
        subsType.erase(topicName);
    }

    bool hasQu() {
        return personalQu.empty();
    }

    // This function wraps up many mesages from the personal queue
    // and sends them up to the fresh reconnected subscriber
    bool updateReconnection(std::list<MessageQuCountType> &qudMessages) {
        std::list<MessageQuCountType>::iterator quComp;
        TcpMsg msg;
        memset(&msg, 0, sizeof(TcpMsg));
        unsigned int nr = 0;

        msg.type = 10;
        unsigned int dim = 0;
        unsigned int dimHere = 0;

        if (personalQu.size() > 20) {
            dim = htonl(20);
            dimHere = 20;
        } else {
            dim = htonl(personalQu.size());
            dimHere = personalQu.size();
        }

        memcpy(msg.content, &dim, 4);

        if (send(this->clientFd, &msg, sizeof(msg), 0) < 0) {
            write(1, "Failed to send the reconnection update request", 47);
            return false;
        }

        TcpMsg *msgArr = (TcpMsg *)malloc(sizeof(TcpMsg) * dimHere);

        // Computing the chunk of messages
        while (nr < dimHere) {
            quComp = personalQu.front();
            personalQu.pop();
            msg = quComp->getMsg();

            memcpy(&msgArr[nr], &msg, sizeof(TcpMsg));
            ++nr;

            // If the current message that is stored in the general queue
            // has no more subscribers that are waiting for it then it
            // can safely be erased (in a constant time O(1))
            quComp->decClinetCount();
            if (!quComp->getSubsWaiting()) {
                qudMessages.erase(quComp);
            }
        }

        if (send(this->clientFd, msgArr, sizeof(TcpMsg) * dimHere, 0) < 0) {
            write(1, "Failed to send the reconnection queued messages", 48);
            free(msgArr);
            return false;
        }

        free(msgArr);
        return true;
    }
};

#endif
