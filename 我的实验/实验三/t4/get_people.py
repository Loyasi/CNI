import socket

def get_people_daily():
    # 目标网站信息
    host = "www.people.com.cn"  # 主机名
    port = 80                   # HTTP 默认端口
    path = "/"                  # 请求的路径（根目录）

    # 创建 TCP socket
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            # 连接到服务器
            s.connect((host, port))
            print(f"成功连接到 {host}:{port}")

            # 构造 HTTP GET 请求
            # 注意：HTTP 协议要求每行结尾必须是 \r\n，最后需空一行表示请求结束
            request = (
                f"GET {path} HTTP/1.1\r\n"
                f"Host: {host}\r\n"
                "Connection: close\r\n"  # 告诉服务器处理完请求后关闭连接
                "\r\n"                  # 空行表示请求头结束
            )

            # 发送请求（需转换为字节流）
            s.sendall(request.encode("utf-8"))
            print("已发送 HTTP GET 请求")

            # 接收响应（分块接收，避免内容过长）
            response = b""
            while True:
                data = s.recv(4096)  # 每次接收 4KB 数据
                if not data:        # 数据接收完毕
                    break
                response += data

            # 转换响应为字符串（忽略无法解码的字符，避免报错）
            response_str = response.decode("utf-8", errors="ignore")

            # 保存到文件
            with open("people_daily.txt", "w", encoding="utf-8") as f:
                f.write(response_str)
            print("网页内容已保存到 people_daily.txt")

        except Exception as e:
            print(f"发生错误：{e}")

if __name__ == "__main__":
    get_people_daily()