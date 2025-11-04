#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#define BUFFER_SIZE 255
//client

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_address> <server_port>\n", argv[0]);
        return 1;
    }

    int client_sock;
    struct sockaddr_in server_addr;
    char send_msg[BUFFER_SIZE];
    char recv_msg[BUFFER_SIZE];  // 新增：用于接收服务器返回的逆序消息

    /* 创建socket */
    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock == -1) {
        perror("socket_error");
        return 1;
    }

    /* 指定服务器地址 */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    if (inet_aton(argv[1], &server_addr.sin_addr) == 0) {
        perror("inet_aton_error");
        close(client_sock);
        return 1;
    }
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

    /* 连接服务器 */
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect_error");
        close(client_sock);
        return 1;
    }

    /* 循环发送消息并接收逆序结果 */
    while (1) {
        printf("Enter message ('bye' to exit): ");
        fgets(send_msg, BUFFER_SIZE, stdin);

        printf("Send: %s", send_msg);
        if (send(client_sock, send_msg, strlen(send_msg), 0) == -1) {
            perror("send_error");
            close(client_sock);
            return 1;
        }

        // 检查是否退出
        if (strncmp(send_msg, "bye", 3) == 0) {
            break;
        }

        // 新增：接收服务器返回的逆序消息
        memset(recv_msg, 0, sizeof(recv_msg));
        ssize_t recv_len = recv(client_sock, recv_msg, BUFFER_SIZE - 1, 0);
        if (recv_len == -1) {
            perror("recv_error");
            close(client_sock);
            return 1;
        } else if (recv_len == 0) {
            printf("Server disconnected\n");
            break;
        }

        // 新增：打印服务器返回的逆序消息
        printf("Recv reversed: %s", recv_msg);
        
    }

    /* 关闭socket */
    if (close(client_sock) == -1) {
        perror("close_error");
        return 1;
    }

    return 0;
}