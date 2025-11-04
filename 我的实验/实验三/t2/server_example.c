#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
//server

// 字符串逆序处理函数
void reverse_string(char *str) {
    int len = strlen(str);
    int i, j;
    char temp;
    for (i = 0, j = len - 1; i < j; i++, j--) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
    }
}

// 处理Ctrl+C信号
void handle_sigint(int sig) {
    printf("\nServer is shutting down...\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    int server_sock_listen, server_sock_data;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    char recv_msg[255];

    signal(SIGINT, handle_sigint);

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_address> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* 创建socket */
    server_sock_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock_listen == -1) {
        perror("socket");
        return 1;
    }

    /* 指定服务器地址 */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(server_sock_listen);
        return 1;
    }

    /* 绑定socket与地址 */
    if (bind(server_sock_listen, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_sock_listen);
        return 1;
    }

    /* 监听socket */
    if (listen(server_sock_listen, 5) == -1) {
        perror("listen");
        close(server_sock_listen);
        return 1;
    }

    printf("Server started, listening on %s:%s...\n", argv[1], argv[2]);
    printf("Press Ctrl+C to stop the server\n");

    /* 迭代处理客户端 */
    while (1) {
        client_addr_len = sizeof(client_addr);
        
        /* 接受连接 */
        server_sock_data = accept(server_sock_listen, (struct sockaddr *)&client_addr, &client_addr_len);
        if (server_sock_data == -1) {
            perror("accept");
            continue;
        }

        printf("New client connected: %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));

        /* 循环接收消息 */
        do {
            memset(recv_msg, 0, sizeof(recv_msg));
            ssize_t recv_len = recv(server_sock_data, recv_msg, sizeof(recv_msg) - 1, 0);
            if (recv_len == -1) {
                perror("recv");
                break;
            } else if (recv_len == 0) {
                printf("Client disconnected unexpectedly\n");
                break;
            }

            // 检查退出指令
            if (strcmp(recv_msg, "bye\n") == 0 || strcmp(recv_msg, "bye") == 0) {
                printf("Client requested to exit\n");
                break;
            }

            printf("Recv: %s", recv_msg);

            // 修复换行问题：先剔除输入中的换行符
            int len = strlen(recv_msg);
            if (len > 0 && recv_msg[len - 1] == '\n') {
                recv_msg[len - 1] = '\0';  // 去掉末尾换行
                len--;
            }

            // 逆序处理
            reverse_string(recv_msg);

            // 手动添加正确的换行符
            if (len > 0) {
                recv_msg[len] = '\n';      // 末尾加换行
                recv_msg[len + 1] = '\0';  // 确保字符串结束
            }

            // 回送逆序后的消息
            if (send(server_sock_data, recv_msg, strlen(recv_msg), 0) == -1) {
                perror("send");
                break;
            }
            printf("Sent reversed: %s", recv_msg);
            
        } while (1);

        /* 关闭数据socket */
        if (close(server_sock_data) == -1) {
            perror("close");
        }
        printf("Connection with client closed. Waiting for new connections...\n");
    }

    close(server_sock_listen);
    return 0;
}