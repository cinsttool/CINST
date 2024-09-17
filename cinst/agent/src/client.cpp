#include "client.h"
#include "utils.h"
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include "spdlog/spdlog.h"
#include <functional>
#include <filesystem>

ASMClient::ASMClient() {
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        spdlog::error("can not create socket");
        throw nullptr;
    }
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(12345);                                 // server's port
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");                            // server's ip
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) // send request to server for connection
    {
        perror("connect");
        close(sockfd);
        spdlog::error("cannot connect to server");
        throw nullptr;
    }
    pid = getpid();
}
ASMClient::~ASMClient() {
    close(sockfd);
}

int ASMClient::send(const void* buf, int len) {
    return ::send(sockfd, buf, len, 0);
}
int ASMClient::recv(void* buf, int len) {
    int sum = 0;
    while (sum != len) {
        auto res = ::recv(sockfd, ((unsigned char*)buf)+sum, len-sum, 0);
        if (res < 0) {
            return res;
        }
        sum += res;
    }
    return sum;
}
int ASMClient::send_str(const char* src)
{
    int len = strlen(src);
    send_with_len(src, len);
    return len;
}
int ASMClient::send_with_len(const void* buf, int len) {
    auto res = ::send(sockfd, &len, 4, 0);
    assert(res != -1);
    res = ::send(sockfd, buf, len, 0);
    assert(res != -1);
    return 0;
}

void ASMClient::send_int(int val) {
    ::send(sockfd, &val, 4, 0);
}
void ASMClient::recv_int(int* val) {
    recv(val, 4);
}

void ASMClient::send_bytecode(const char* name, const unsigned char* class_data, int class_data_len)
{
    send_str(name);
    send_with_len(class_data, class_data_len);
}
void ASMClient::recv_bytecode(unsigned char** new_class_data, int* new_class_data_len, const std::function<void(size_t, unsigned char**)>& allocator)
{
    recv_int(new_class_data_len);
    if (*new_class_data_len == 0) {
        return;
    }
    allocator(*new_class_data_len, (unsigned char**)new_class_data);
    recv(*new_class_data, *new_class_data_len);
}
void ASMClient::send_header(int command) {
    send_int(pid);
    send_int(command);
}
void ASMClient::send_hello() {
    auto cwd = std::filesystem::current_path();
    send_header(HELLO);
    spdlog::debug("cwd: {}", cwd.c_str());
    send_str(cwd.c_str());
    // send_with_len(cwd.c_str(), cwd.string().size());
    spdlog::debug("after send cwd");
    int status;
    recv_int(&status);
    assert(status == 0);
}

