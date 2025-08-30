#include <iostream>
#include "tcp.h"
#include "http.h"

int main() {
    TCP server("127.0.0.1", 8888);
    server.bindAndListen();

    Http handler;
    server.serve(handler);

    return 0;
}
