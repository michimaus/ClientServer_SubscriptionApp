DUICAN MIHNEA - IONUȚ
324CA

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ README - TEMA 2 PC ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The main two components of my applications are found in subscriber.cpp and server.cpp. First of all,
those are checking the correctness of the introduced arguments. Afterwards there will be instantiated
the server object in server.cpp and the client object in subscriber.cpp. The main core implementation
of my solutions is associated with the logic of the constructors, methods, and destructors of those
classes.

Class for my CLIENT:
    The parameters used by this class capture the basic data of the client(ID, port, Ip and so on).
    Also this class has a buffer structure for processing data coming from server. These are the
    main steps:

    - connection is established within the main constructor, after the request is accepted by
    the server the client sends on his ID in order to let the server know his authenticity.

    - the main runner of the client is performing select() on the FD_SET in order to know which
    FD is showing activity in this program. There are two main cases: getting input from std::in
    or receiving a message from server.

    - when reading input from std::in the client is processing the message and executes a command
    only in case of a valid stream

    - when receiving a message from server, first of all is checked it's type and after that our
    client makes a decision. It can be commanded to shut down itself or he can close up on its own
    in case of a malfunction. On the other hand he can receive data that has to be displayed on
    std:out.

    -shut down is performed by destructor

Class for my SERVER:
    This class is holding up multiple data structures in order to know the evidence of all clients
    and the topics they are subscribed to. I created some custom classes and types, such as the
    the class for subscribers where it's mapped the name of a topic with it's subscription type,
    a personal queue that holds up pointers/iterators to the general queue-elements of the server,
    making the access in constant time when it comes to deleting a message in order to preserve
    the app from consuming to much memory. Moreover the clients are mapped by their ID's (whereas it
    is actually instantiated an subscriber-object) and by their personal FD, this time with pointers
    to the objects of the previously mentioned data structure. The topic-objects also retains a map,
    by the clients' IDs with pointes to the client-objects. In addition to that a client-object
    is deleted when he gets offline and it has no active subscriptions (stays in if it has at least
    one subscription - no matter it is type 1 or 0), and also a topic object will be deleted if
    there are no known subscribers (idem - no matter 1 or 0). Working up together, al those data
    structures assure increased efficiency in accessing any object and preserve the memory leaks and
    posable memory overflows, by using std::*stuff* of C++. **Worked hard to figure out this system:)

    * A possible improvement would be when accessing a topic's subscribers map (from here could have
    retained 2 distinct lists for type 1 and type 0 subscribers, but i'm looking in the subscriber's
    object to figure out it's type instead. In theory this would mean worst case O(logn)^2, but in a 
    real world situation a topic would have hundreds of thousands of subscribers, but when it comes
    to an ordinary subscriber he would only be interested in only ~few hundred of topics, so I
    consider that my solution would perform better in practice).

    The steps followed by the main runners of the server object are quite similar with the ones of 
    the client:

    - the only valid command coming from std::in is exit/quit.

    - the server checks if there is any message coming from the UDP client and the current
    online subscribers (for this topic) are updated and in case there are type 1 offline
    subscribers the message is stored in the general list and those clients get pointer - 
    iterators to the specific message and update their personal queue.

    - the server rejects a client if he is trying to connect with an ID that is already in use.

    - when a client logs out his FD is removed from the set (along with the pointer to his object)

    - when the server is about to shut down he sends a suspend signal to all the clients (destructor)

DISCLAIMER!!
    Yes yes...I know I am one day off deadline but I had some technical issues with my laptop, and lost
    some time to recoverer my stuff...What matters is that I had backup to my work and only lost a few
    things and I assume the fact that I will lose points on that. Have a good day! :)