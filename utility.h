#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <sys/select.h>
#include <vector>
#include <arpa/inet.h>

int startServer(const char *port_num) {
    int status;
    int server_fd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    const char *machine_name = NULL;

    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags = AI_PASSIVE;

    status = getaddrinfo(machine_name, port_num, &host_info, &host_info_list);
    if (status != 0) {
        std::cerr << "Error: cannot get address info for host" << std::endl;
        std::cerr << "  (" << machine_name << "," << port_num << ")" << std::endl;
        exit(EXIT_FAILURE);
    }

    server_fd = socket(host_info_list->ai_family,
                       host_info_list->ai_socktype,
                       host_info_list->ai_protocol);
    if (server_fd == -1) {
        std::cerr << "Error: cannot create socket" << std::endl;
        std::cerr << "  (" << machine_name << "," << port_num << ")" << std::endl;
        exit(EXIT_FAILURE);
    }

    int yes = 1;
    status = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(server_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        std::cerr << "Error: cannot bind socket" << std::endl;
        std::cerr << "  (" << machine_name << "," << port_num << ")" << std::endl;
        exit(EXIT_FAILURE);
    }

    status = listen(server_fd, 100);
    if (status == -1) {
        std::cerr << "Error: cannot listen on socket" << std::endl;
        std::cerr << "  (" << machine_name << "," << port_num << ")" << std::endl;
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(host_info_list);

    return server_fd;
}

int startClient(const char *machine_name, const char *port_num) {
    int status;
    int server_fd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;

    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(machine_name, port_num, &host_info, &host_info_list);
    if (status != 0) {
        std::cerr << "Error: cannot get address info for host" << std::endl;
        std::cerr << "  (" << machine_name << "," << port_num << ")" << std::endl;
        exit(EXIT_FAILURE);
    }

    server_fd = socket(host_info_list->ai_family,
                       host_info_list->ai_socktype,
                       host_info_list->ai_protocol);
    if (server_fd == -1) {
        std::cerr << "Error: cannot create socket" << std::endl;
        std::cerr << "  (" << machine_name << "," << port_num << ")" << std::endl;
        exit(EXIT_FAILURE);
    }

    status = connect(server_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        std::cerr << "Error: cannot connect to socket" << std::endl;
        std::cerr << "  (" << machine_name << "," << port_num << ")" << std::endl;
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(host_info_list);

    return server_fd;
}

int getServerPort(int server_fd) {
    struct sockaddr_in server_addr;
    socklen_t server_addr_len = sizeof(server_addr);
    int status = getsockname(server_fd, (struct sockaddr *) &server_addr, &server_addr_len);
    if (status == -1) {
        std::cerr << "Error: server port not found for server fd: " << server_fd << std::endl;
        exit(EXIT_FAILURE);
    }
    return ntohs(server_addr.sin_port);
}
