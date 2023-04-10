#include "utility.h"
#include "potato.h"
#include <algorithm>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Syntax: ringmaster <port_num> <num_players> <num_hops>" << std::endl;
        return EXIT_FAILURE;
    }

    const char *port_num = argv[1];
    const int num_players = std::atoi(argv[2]);
    const int num_hops = std::atoi(argv[3]);

    if (num_players < 2) {
        std::cerr << "Error: number of players must be more than 1" << std::endl;
        return EXIT_FAILURE;
    }
    if (num_hops < 0 || num_hops > 512) {
        std::cerr << "Error: number of hops must be in 0-512 range" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Potato Ringmaster" << std::endl;
    std::cout << "Players = " << num_players << std::endl;
    std::cout << "Hops = " << num_hops << std::endl;

    std::vector<int> player_fds;
    std::vector<int> player_ports;
    std::vector<std::string> player_ips;

    int my_fd = startServer(port_num);

    for (int player_id = 0; player_id < num_players; player_id++) {
        struct sockaddr_storage player_addr;
        socklen_t player_addr_len = sizeof(player_addr);
        int player_fd = accept(my_fd, (struct sockaddr *) &player_addr, &player_addr_len);
        if (player_fd == -1) {
            std::cerr << "Error: cannot accept connection on socket" << std::endl;
            return EXIT_FAILURE;
        }
        player_fds.push_back(player_fd);
        std::string player_ip = inet_ntoa(((struct sockaddr_in *) &player_addr)->sin_addr);
        player_ips.push_back(player_ip);

        ssize_t n = send(player_fd, &player_id, sizeof(player_id), 0);
        if (n == -1) {
            std::cerr << "Error: failed to send player id " << player_id << std::endl;
            return EXIT_FAILURE;
        }
        n = send(player_fd, &num_players, sizeof(num_players), 0);
        if (n == -1) {
            std::cerr << "Error: failed to receive num players to player with id " << player_id << std::endl;
            return EXIT_FAILURE;
        }

        int player_port;
        n = recv(player_fd, &player_port, sizeof(player_port), MSG_WAITALL);
        if (n == -1) {
            std::cerr << "Error: failed to receive player port of player id " << player_id << std::endl;
            return EXIT_FAILURE;
        }
        player_ports.push_back(player_port);
        std::cout << "Player " << player_id << " is ready to play" << std::endl;
    }

    for (int player_id = 0; player_id < num_players; player_id++) {
        int neighbor_id = (player_id + 1) % num_players;

        char neighbor_ip_buffer[256];
        memset(&neighbor_ip_buffer, 0, sizeof(neighbor_ip_buffer));
        std::strcpy(neighbor_ip_buffer, player_ips[neighbor_id].c_str());
        std::string neighbor_ip = player_ips[neighbor_id];
        ssize_t n = send(player_fds[player_id], &neighbor_ip_buffer, sizeof(neighbor_ip_buffer), 0);
        if (n == -1) {
            std::cerr << "Error: failed to send neighbor ip to player id " << player_id << std::endl;
            return EXIT_FAILURE;
        }

        int neighbor_port = player_ports[neighbor_id];
        n = send(player_fds[player_id], &neighbor_port, sizeof(neighbor_port), 0);
        if (n == -1) {
            std::cerr << "Error: failed to send neighbor port to player id " << player_id << std::endl;
            return EXIT_FAILURE;
        }
    }

    if (num_hops != 0) {
        srand((unsigned int) time(NULL) + num_players);
        int random_player_id = rand() % num_players;

        Potato *potato = new Potato();
        potato->setNumHops(num_hops);

        ssize_t n = send(player_fds[random_player_id], potato, sizeof(*potato), 0);
        if (n == -1) {
            std::cerr << "Error: failed to send potato to player id " << random_player_id << std::endl;
            return EXIT_FAILURE;
        }

        std::cout << "Ready to start the game, sending potato to player " << random_player_id << std::endl;

        fd_set readfds;
        FD_ZERO(&readfds);
        for (int player_id = 0; player_id < num_players; player_id++) {
            FD_SET(player_fds[player_id], &readfds);
        }
        int nfds = *std::max_element(player_fds.begin(), player_fds.end()) + 1;
        int status = select(nfds, &readfds, NULL, NULL, NULL);
        if (status == -1) {
            std::cerr << "Error: cannot accept connection over select" << std::endl;
            return EXIT_FAILURE;
        } else {
            for (int player_id = 0; player_id < num_players; player_id++) {
                if (FD_ISSET(player_fds[player_id], &readfds)) {
                    n = recv(player_fds[player_id], potato, sizeof(*potato), MSG_WAITALL);
                    if (n == -1) {
                        std::cerr << "Error: failed to receive a potato" << std::endl;
                        return EXIT_FAILURE;
                    }
                    break;
                }
            }
        }

        std::cout << "Trace of potato:" << std::endl;
        potato->printTrace();
    }

    for (int player_id = 0; player_id < num_players; player_id++) {
        Potato *potato = new Potato();
        potato->setNumHops(513);
        ssize_t n = send(player_fds[player_id], &potato, sizeof(potato), 0);
        if (n == -1) {
            std::cerr << "Error: failed to send shut down to player id " << player_id << std::endl;
            return EXIT_FAILURE;
        }
    }

    for (int player_id = 0; player_id < num_players; player_id++) {
        close(player_fds[player_id]);
    }
    close(my_fd);

    return EXIT_SUCCESS;
}
