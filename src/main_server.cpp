#include <iostream>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cerrno>

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

    if (listen(listen_fd, 10) < 0){
        perror("listen");
        return 1;
    }
    
    // 创建 epll 实例
    
    int epfd = epoll_create1(0); // 0 : 没有特殊标志
    
    // 往 epoll 加入一个fd
    struct epoll_event ev{};
    
    ev.events = EPOLLIN; // 对 fd 的可读感兴趣
    ev.data.fd = listen_fd; // 把 listen_fd 存入，方便等会知道是谁 ready
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);
    
    // 等待事件
    struct epoll_event ready[10]; // 最多一次处理 10 个事件
    while (true){

        int n = epoll_wait(epfd, ready, 10, -1); // 10 ：最多返回 10 个, -1:无限等待(阻塞), 0就是不等待， 1000就是等待1000毫秒
        
        for (int i = 0; i < n; i++){
            int fd = ready[i].data.fd;
            
            if (fd == listen_fd){
                // 有新的连接
                int client_fd = accept(listen_fd, nullptr, nullptr);
                ev.events = EPOLLIN;
                ev.data.fd = client_fd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
            }
            else{
                // 已有的客户端发来的要转发的数据
                char buffer[1024];
                ssize_t bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);
                if (bytes <= 0){
                    // 客户端断了
                    close(fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr); // 从 epoll 移除
                }
                else{
                    send(fd, buffer, bytes, 0);
                }
            }
        }
    }

    // 关闭
    close(epfd);
    close(listen_fd);
    return 0;
}