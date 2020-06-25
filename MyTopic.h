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
#include <list>
#include <unordered_set>
#include <string>
#include "CustomTypes.h"

#ifndef MY_TOPIC_H
#define MY_TOPIC_H


// Helper class that creates objects regarding the topics associated with subscribers
// Maps up the subscriber's name with a pointer to subscriber object
class MyTopicType {
private:
    std::string topicName;
    std::unordered_map<std::string, MySubType *> subscribers;

public:
    MyTopicType(const std::string &name) {
        this->topicName = name;
    }

    std::string getTopicName() {
        return topicName;
    }

    void addSubscriber(std::string &sub, MySubType *client) {
        if (subscribers.find(sub) == subscribers.end()) {
            this->subscribers.insert({sub, client});
        }
    }

    bool removeSubscriber(std::string &sub) {
        subscribers.erase(sub);
        if (subscribers.empty()) {
            return true;
        }
        return false;
    }

    // This function lists out those subscribers that requested SF on
    // on this topic and are currently offline
    std::list<MySubType *> updateSubscribers(TcpMsg &msg, bool &canRunServer) {
        std::list<MySubType *> unconnectedAndQud;
        for (const auto &iter : subscribers) {
            if (iter.second->performUpdate(msg, canRunServer)) {
                unconnectedAndQud.push_back(iter.second);
                if (!canRunServer) {
                    break;
                }
            }
        }
        return unconnectedAndQud;
    }
};

#endif
