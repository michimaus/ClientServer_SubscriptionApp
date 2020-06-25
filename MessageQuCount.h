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
#include <string>
#include <queue>
#include "CustomTypes.h"

#ifndef MSG_QU_TYPE_H
#define MSG_QU_TYPE_H


// Helper class that creates objects meant to be stored in the
// general queue
// Links up the content regarding the message and the number
// of clients that are want this message when they reconnect
class MessageQuCountType {
private:
    TcpMsg msg;
    int subsWaiting;
public:
    MessageQuCountType(const TcpMsg &msg, const int &subsWaiting) {
        memcpy(&this->msg, &msg, sizeof(msg));
        this->subsWaiting = subsWaiting;
    }

    int getSubsWaiting() {
        return this->subsWaiting;
    }

    TcpMsg getMsg() {
        return this->msg;
    }

    void decClinetCount() {
        --this->subsWaiting;
    }

};

#endif
