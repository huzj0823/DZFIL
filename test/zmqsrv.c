#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>

int main () {
    void* context = zmq_ctx_new();
    //void* responder = zmq_socket(context, ZMQ_REP);
    void* responder = zmq_socket(context, ZMQ_PULL);
    int rc = zmq_bind(responder, "tcp://*:5555");
    if (rc != 0) {
        printf("Error binding socket: %s\n", zmq_strerror(errno));
        return 1;
    }

    while (1) {
        char buffer[1024];
        zmq_recv(responder, buffer, 1024, 0);
        printf("Received message: %s\n", buffer);
        zmq_send(responder, "OK", 2, 0);
    }

    zmq_close(responder);
    zmq_ctx_destroy(context);
    return 0;
}
