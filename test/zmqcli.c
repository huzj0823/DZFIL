#include <zmq.h>
#include <stdio.h>
#include <unistd.h>

int main () {
    int rc; 
    // 创建 ZeroMQ 上下文和套接字
    void* context = zmq_ctx_new();
    //void* socket = zmq_socket(context, ZMQ_REQ);
    void* socket = zmq_socket(context, ZMQ_PUSH);
    // 将套接字设置为非阻塞模式
    int timeout = 1;
    //rc = zmq_setsockopt(socket, ZMQ_SNDTIMEO, &timeout, sizeof(uint64_t));
    //rc = zmq_setsockopt(socket, ZMQ_RCVTIMEO, &timeout, sizeof(uint64_t));
    // 设置连接超时时间为1秒
    //rc = zmq_setsockopt(socket, ZMQ_CONNECT_TIMEOUT, &timeout, sizeof(int));
    // 连接服务器
    rc = zmq_connect(socket, "tcp://localhost:5555");
    printf("connect return %d\n", rc);
    if (rc == -1 && zmq_errno() == ETERM) {
        // 上下文已被销毁
        printf("No found server\n");
        return 0;
    } else if (rc == -1 && zmq_errno() == EHOSTUNREACH) {
        // 服务器不可达
        printf("Server is unreachable\n");
        return 0;
    } else if (rc == -1 && zmq_errno() == ECONNREFUSED) {
        // 服务器拒绝连接
        printf("Connection refused by server\n");
        return 0;
    }

    while (1) {
        // 发送消息
        rc = zmq_send(socket, "Hello", 5, 0);
        printf("send return %d\n", rc);
        //if (rc == -1 && zmq_errno() == EAGAIN) {
        if (rc == -1) {
            // 无法发送消息，继续等待
            printf("send failed, %d\n", zmq_errno());
            //sleep(1);
            continue;
        }
        /*
        // 接收响应消息
        char buffer[1024];
        int size = zmq_recv(socket, buffer, 1024, 0);
        if (size == -1 && zmq_errno() == EAGAIN) {
            // 没有收到响应消息，继续等待
            printf("Recevied failed\n");
            continue;
        }
        // 处理响应消息
        printf("Received message: %s\n", buffer);
        */
        // 等待一段时间后重试发送消息
        sleep(1);
    }

    // 关闭套接字和上下文
    rc = zmq_close(socket);
    rc = zmq_ctx_destroy(context);

    return 0;
}
