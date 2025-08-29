#ifndef CONNECTION_H
#define CONNECTION_H

class TCP;

class IConnectionHandler {
public:

    virtual ~IConnectionHandler() = default;
    virtual void handle(int fd, TCP& io) = 0;
    
};

#endif
