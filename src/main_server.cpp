#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main(){
    // 创建socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0){ 
        perror("socket");
        return 1;
    }

    // bind 绑定 12345 端口
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        perror("bind");
        return 1;
    }

    // listen
    if (listen(listen_fd, 10) < 0){
        perror("listen");
        return 1;
    }

    std::cout << "Listening on port 12345..." << std::endl;

    // accept 阻塞等待客户端
    int client_fd = accept(listen_fd, nullptr, nullptr);
    if (client_fd < 0){
        perror("accept");
        return 1;
    }
    std::cout << "Client connected!" << std::endl;


    // 接受数据

    char buffer[1024];

    while (true){
        // recv 收数据
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) break;

        std::cout << "Received" << bytes <<" bytes" << std::endl;

        // send 原样发回去
        send(client_fd, buffer, bytes, 0);
    }

    // 关闭
    close(client_fd);
    close(listen_fd);
    return 0;
}