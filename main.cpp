#include <iostream>
#include "tcp.h"
#include "http.h"

int main() {
    TCP server("0.0.0.0", 8888);
    server.bindAndListen();

    Http handler;
    server.serve(handler);

    return 0;
}
