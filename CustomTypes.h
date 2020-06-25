#ifndef CUSTOM_TYPES_H
#define CUSTOM_TYPES_H

#define BUFLEN 2048
#define TOPICLEN 50
#define CONTENTLEN 1500
#define MAX_CLIENTS_QUEUE 50
#define CLIIDLEN 11
#define LEN_IP 20

// Used to receive from the udp client
struct UdpMsgStr {
	char topic[TOPICLEN];
	unsigned char type;
	char content[CONTENTLEN];
} __attribute__((__packed__));

// Used by clients to receive content from server
struct TcpMsg {
	unsigned char type;
	char content[CONTENTLEN];
	char topic[TOPICLEN];
	char ip[LEN_IP];
	unsigned short int port;
} __attribute__((__packed__));

// Used by server to receive content from clients
struct ClientSubMsg {
	unsigned char type;
	char topic[TOPICLEN];
	unsigned char sfOrNot;
} __attribute__((__packed__));

// Auxiliary classes/types used in order to manipulate data and store it
// in the most efficient way possible 
class MyTopicType;
class MySubType;
class MessageQuCountType;

#endif
