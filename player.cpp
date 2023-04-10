#include "utility.h"
#include "potato.h"
#include <algorithm>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Syntax: player <machine_name> <port_num>" << std::endl;
        return EXIT_FAILURE;
    }

    const char *machine_name = argv[1];
    const char *port_num = argv[2];

    int ringmaster_fd = startClient(machine_name, port_num);

    int my_id;
    int num_players;
    ssize_t n = recv(ringmaster_fd, &my_id, sizeof(my_id), MSG_WAITALL);
    if (n == -1) {
        std::cerr << "Error: failed to read player_id from ringmaster" << std::endl;
        return EXIT_FAILURE;
    }

    n = recv(ringmaster_fd, &num_players, sizeof(num_players), MSG_WAITALL);
    if (n == -1) {
        std::cerr << "Error: failed to read num_players from ringmaster" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Connected as player " << my_id << " out of " << num_players << " total players" << std::endl;

    int my_fd = startServer("0");
    int my_port = getServerPort(my_fd);

    n = send(ringmaster_fd, &my_port, sizeof(my_port), 0);
    if (n == -1) {
        std::cerr << "Error: failed to send port num to ringmaster" << std::endl;
        return EXIT_FAILURE;
    }

    char right_ip[256];
    memset(&right_ip, 0, sizeof(right_ip));
    n = recv(ringmaster_fd, &right_ip, sizeof(right_ip), MSG_WAITALL);
    if (n == -1) {
        std::cerr << "Error: failed to read neighbor ip from ringmaster" << std::endl;
        return EXIT_FAILURE;
    }

    int right_port;
    n = recv(ringmaster_fd, &right_port, sizeof(right_port), MSG_WAITALL);
    if (n == -1) {
        std::cerr << "Error: failed to read neighbor port from ringmaster" << std::endl;
        return EXIT_FAILURE;
    }

    int right_fd = startClient(right_ip, std::to_string(right_port).c_str());

    struct sockaddr_storage left_addr;
    socklen_t left_addr_len = sizeof(left_addr);
    int left_fd = accept(my_fd, (struct sockaddr *) &left_addr, &left_addr_len);
    if (left_fd == -1) {
        std::cerr << "Error: cannot accept connection on socket" << std::endl;
        return EXIT_FAILURE;
    }

    fd_set readfds;
    std::vector<int> known_fds;
    known_fds.push_back(ringmaster_fd);
    known_fds.push_back(left_fd);
    known_fds.push_back(right_fd);
    int nfds = *std::max_element(known_fds.begin(), known_fds.end()) + 1;

    srand((unsigned int) time(NULL) + 2);

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(ringmaster_fd, &readfds);
        FD_SET(left_fd, &readfds);
        FD_SET(right_fd, &readfds);

        int status = select(nfds, &readfds, NULL, NULL, NULL);
        if (status < 0) {
            std::cerr << "Error: cannot accept connection over select" << std::endl;
            return EXIT_FAILURE;
        } else if (status == 0) {
            continue;
        } else {
            Potato potato;
            for (int i = 0; i < 3; i++) {
                if (FD_ISSET(known_fds[i], &readfds)) {
                    n = recv(known_fds[i], &potato, sizeof(potato), MSG_WAITALL);
                    if (n == -1) {
                        std::cerr << "Error: failed to receive a potato" << std::endl;
                        return EXIT_FAILURE;
                    }
                    break;
                }
            }
            if (potato.getNumHops() == 0 || potato.getNumHops() > 512) {
                break;
            } else if (potato.getNumHops() == 1) {
                potato.decrNumHops();
                potato.addToTrace(my_id);
                potato.incrSize();
                n = send(ringmaster_fd, &potato, sizeof(potato), 0);
                if (n == -1) {
                    std::cerr << "Error: failed to send potato to ringmaster" << std::endl;
                    return EXIT_FAILURE;
                }
                std::cout << "I'm it" << std::endl;
                break;
            } else {
                int random = rand() % 2;
                int neighbor_id;
                int neighbor_fd;
                if (random == 1) {
                    neighbor_id = (my_id + 1) % num_players;
                    neighbor_fd = right_fd;
                } else {
                    neighbor_id = (my_id - 1 + num_players) % num_players;
                    neighbor_fd = left_fd;
                }
                potato.decrNumHops();
                potato.addToTrace(my_id);
                potato.incrSize();
                n = send(neighbor_fd, &potato, sizeof(potato), 0);
                if (n == -1) {
                    std::cerr << "Error: failed to send potato to neighbor" << std::endl;
                    return EXIT_FAILURE;
                }
                std::cout << "Sending potato to " << neighbor_id << std::endl;
            }
        }
    }

    close(left_fd);
    close(right_fd);
    close(ringmaster_fd);

    return EXIT_SUCCESS;
}
