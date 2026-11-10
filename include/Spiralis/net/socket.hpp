#ifndef ____SP_SOCKET____
#define ____SP_SOCKET____
#pragma once
#include "../setup/init.hpp"
#include "../containers/string.hpp"
#if ___SP_USING_PLATFORM_LINUX___ || ___SP_USING_PLATFORM_APPLE___ || ___SP_USING_COMPILER_MINGW___
    #include <sys/types.h>    // Basic system data types (size_t, ssize_t, etc.)
    #include <sys/event.h>    // kqueue and kevent definitions
    #include <sys/time.h>     // struct timeval and struct timespec (often needed for timeouts)
    #include <sys/socket.h>   // Core socket functions and structures (socket, bind, listen, accept)
    #include <netinet/in.h>   // Internet domain address structures (struct sockaddr_in)
    #include <arpa/inet.h>    // IP address conversion functions (inet_pton, inet_ntoa)
    #include <unistd.h>       // Standard symbolic constants and types (close, read, write)
    #include <fcntl.h>        // File control options (fcntl for setting O_NONBLOCK)
    #include <stdio.h>        // Standard input/output (perror, printf)
    #include <stdlib.h>       // Standard library functions (exit, atoi)
    #include <string.h>       // Memory manipulation functions (memset, strerror)
#endif

namespace sp{ namespace net {
enum class ip_type{
    ipv4, ipv6
};

enum class protocol{
    TCP, UDP
};

struct packet{
    size_type size = 0;
    void* data = nullptr;
    ~packet(){
        if(data){
            sp::free(data);
        }
    }
};

class socket{
    friend bool set_socket_buf_size(const socket&, int);
    #if ___SP_USING_PLATFORM_LINUX___ || ___SP_USING_PLATFORM_APPLE___ || ___SP_USING_COMPILER_MINGW___
        int fd = -1;
        ip_type ip;
        protocol p;
        uint16_t port = 0;
        bool has_binded = false;
    public:
        socket(ip_type ip_ = ip_type::ipv4, protocol p_ = protocol::TCP) : ip(ip_), p(p_){
            int domain = (ip == ip_type::ipv4) ? AF_INET : AF_INET6;
            int type = (p == protocol::TCP) ? SOCK_STREAM : SOCK_DGRAM;
            fd = ::socket(domain, type, 0);
        }
        ~socket(){
            if(fd >= 0) ::close(fd);
        }
        socket(const socket&) = delete;
        socket& operator=(const socket&) = delete;

        socket(socket&& other) noexcept : fd(other.fd), ip(other.ip), p(other.p), port(other.port), has_binded(other.has_binded) {
            other.fd = -1;
        }

        socket& operator=(socket&& other) noexcept {
            if(this != &other) {
                if(fd >= 0) ::close(fd);
                fd = other.fd;
                ip = other.ip;
                p = other.p;
                port = other.port;
                has_binded = other.has_binded;
                other.fd = -1;
            }
            return *this;
        }

        SP_FORCEINLINE socket& set_port(uint16_t p){
            port = p;
            return *this;
        }

        bool bind(){
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(port);

            if(::bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) return false;
            has_binded = true;
            return true;
        }

        bool listen(int backlog = SOMAXCONN){
            if(p != protocol::TCP) return false;
            return ::listen(fd, backlog) == 0;
        }

        socket accept() {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            int client_fd = ::accept(fd, (struct sockaddr*)&client_addr, &client_len);
            if(client_fd < 0) throw exceptions::spiralis_exception("accept failed");

            socket client_sock;
            client_sock.fd   = client_fd;
            client_sock.ip   = ip;
            client_sock.p    = p;
            client_sock.has_binded = true;
            return client_sock;
        }

        bool connect(const sp::string& remote_ip, uint16_t remote_port) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(remote_port);
        inet_pton(AF_INET, remote_ip.c_str(), &addr.sin_addr);

        return ::connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0;
    }

    bool send(size_type size, const void* data){
        size_type sent_size = 0;
        while(sent_size < sizeof(size_type)){
            int bytes = ::send(fd, reinterpret_cast<const char*>(&size) + sent_size, sizeof(size_type) - sent_size, 0);
            if(bytes <= 0) return false;
            sent_size += bytes;
        }
        
        size_type total_sent = 0; 
        while(total_sent < size){
            const char* src = static_cast<const char*>(data) + total_sent;
            size_type remaining = size - total_sent;
            int bytes = ::send(fd, src, remaining, 0);
            if(bytes <= 0) return false;
            total_sent += bytes;
        }
        
        return true;
    }

    packet recv() {
        packet p;
        char size_buffer[8]{};
        size_type total_size_read = 0;

        while(total_size_read < sizeof(size_type)){
            int bytes_read = ::recv(fd, size_buffer + total_size_read, sizeof(size_type) - total_size_read, 0);
            if(bytes_read <= 0) {
                throw exceptions::spiralis_exception("Error reading size header in recv().");
            }
            total_size_read += bytes_read;
        }

        p.size = *reinterpret_cast<size_type*>(size_buffer);
        if(p.size == 0) return p;

        p.data = sp::malloc(p.size);
        if(!p.data){
            throw exceptions::spiralis_exception("Failed to allocate memory for packet data.");
        }

        size_type total_payload_read = 0;
        while(total_payload_read < p.size){
            char* destination = static_cast<char*>(p.data) + total_payload_read;
            size_type remaining = p.size - total_payload_read;
            size_type chunk_size = sp::min(static_cast<size_type>(__SP_DEFAULT_SOCKET_BUF_SIZE__), remaining);

            int bytes_read = ::recv(fd, destination, chunk_size, 0);
            if(bytes_read <= 0){
                throw exceptions::spiralis_exception("Error reading payload bytes in recv().");
            }
            total_payload_read += bytes_read;
        }

        return p;
    }

    bool shutdown(int how = SHUT_RDWR) {
        if(fd < 0) return false;
        return ::shutdown(fd, how) == 0;
    }

    void close() {
        if(fd >= 0){
            ::close(fd);
            fd = -1;
        }
    }
    #else

    #endif
};

bool set_socket_buf_size(const socket& s, int buf_size = __SP_DEFAULT_SOCKET_BUF_SIZE__) {
    if(setsockopt(s.fd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size)) < 0){
        return false;
    }

    if(setsockopt(s.fd, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size)) < 0){
        return false;
    }

    return true;
}
}}

#endif