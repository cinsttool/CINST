#pragma once
#include<functional>
class ASMClient {
    int pid;
    int sockfd;
    public:
    ASMClient();
    ~ASMClient();
    int send(const void* buf, int len);
    int recv(void* buf, int len);
    int send_str(const char* src);
    int send_with_len(const void* buf, int len);
    void recv_int(int* val);
    void send_int(int val);
    void send_header(int command);

    void send_bytecode(const char* name, const unsigned char* class_data, int class_data_len);
    void recv_bytecode(unsigned char** new_class_data, int* new_class_data_len, const std::function<void(size_t, unsigned char**)>& allocator);
    void send_hello();
};