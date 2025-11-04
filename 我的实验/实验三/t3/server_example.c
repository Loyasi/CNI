#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

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

// 子进程处理客户端连接的函数
void handle_client(int client_sock, struct sockaddr_in client_addr) {
    char recv_msg[255];
    ssize_t recv_len;

    printf("Child process handling client: %s:%d\n", 
           inet_ntoa(client_addr.sin_addr), 
           ntohs(client_addr.sin_port));

    while (1) {
        memset(recv_msg, 0, sizeof(recv_msg));
        recv_len = recv(client_sock, recv_msg, sizeof(recv_msg) - 1, 0);
        if (recv_len == -1) {
            perror("recv");
            break;
        } else if (recv_len == 0) {
            printf("Client %s:%d disconnected unexpectedly\n", 
                   inet_ntoa(client_addr.sin_addr), 
                   ntohs(client_addr.sin_port));
            break;
        }

        if (strcmp(recv_msg, "bye\n") == 0 || strcmp(recv_msg, "bye") == 0) {
            printf("Client %s:%d requested to exit\n", 
                   inet_ntoa(client_addr.sin_addr), 
                   ntohs(client_addr.sin_port));
            send(client_sock, "bye\n", 4, 0);
            break;
        }

        printf("Recv from %s:%d: %s", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port), 
               recv_msg);

        // 处理换行符并逆序
        int len = strlen(recv_msg);
        if (len > 0 && recv_msg[len - 1] == '\n') {
            recv_msg[len - 1] = '\0';
            len--;
        }
        reverse_string(recv_msg);
        if (len > 0) {
            recv_msg[len] = '\n';
            recv_msg[len + 1] = '\0';
        }

        if (send(client_sock, recv_msg, strlen(recv_msg), 0) == -1) {
            perror("send");
            break;
        }
        printf("Sent reversed to %s:%d: %s", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port), 
               recv_msg);
    }

    close(client_sock);
    printf("Connection with %s:%d closed. Child process exiting.\n", 
           inet_ntoa(client_addr.sin_addr), 
           ntohs(client_addr.sin_port));
    exit(0);
}

int main(int argc, char *argv[])
{
    int server_sock_listen, server_sock_data;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    pid_t pid;

    // 忽略SIGCHLD信号，避免僵尸进程
    signal(SIGCHLD, SIG_IGN);
    // 处理Ctrl+C信号
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

    /* 父进程循环接受客户端连接，创建子进程处理 */
    while (1) {
        client_addr_len = sizeof(client_addr);
        server_sock_data = accept(server_sock_listen, (struct sockaddr *)&client_addr, &client_addr_len);
        if (server_sock_data == -1) {
            perror("accept");
            continue;
        }

        printf("New client connected: %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));

        // 创建子进程处理该客户端
        pid = fork();
        if (pid == -1) {
            perror("fork");
            close(server_sock_data);
            continue;
        } else if (pid == 0) {
            // 子进程：关闭监听socket（子进程不需要）
            close(server_sock_listen);
            // 处理客户端连接
            handle_client(server_sock_data, client_addr);
        } else {
            // 父进程：关闭数据socket（父进程不需要）
            close(server_sock_data);
        }
    }

    close(server_sock_listen);
    return 0;
}