#define MIN_NUMBER 0
#define MAX_NUMBER 100
#define BUFFER_SIZE 64

#include "common.hpp"
#include <iostream>

void play_game(int sock_fd, bool is_automatic) {
    char buffer[BUFFER_SIZE];
    int low = MIN_NUMBER, high = MAX_NUMBER;

    while (true) {
        int guess;
        std::string message;

        if (is_automatic) {
            guess = low + (high - low) / 2;
            message = std::to_string(guess);
            std::cout << "Trying: " << guess << std::endl;
        }
        else {
            std::cout << "Enter your guess (" << low << "-" << high << "): ";
            std::cin >> guess;
            message = std::to_string(guess);
        }

        check(send(sock_fd, message.c_str(), message.size() + 1, 0));

        check(recv(sock_fd, buffer, sizeof(buffer), 0));

        std::string response(buffer);
        std::cout << response << std::endl;

        if (response == "correct") {
            std::cout << "Congratulations! The number was " << guess << std::endl;
            break;
        }
        if (response == "higher") {
            low = guess + 1;
        }
        else if (response == "lower") {
            high = guess - 1;
        }

        if (low > high) {
            std::cout << "Error: no possible numbers left!" << std::endl;
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <mode: 0-auto, 1-interactive>" << std::endl;
        return 1;
    }

    int mode = atoi(argv[1]);
    if (mode != 0 && mode != 1) {
        std::cerr << "Invalid mode. Use 0 for automatic or 1 for interactive" << std::endl;
        return 1;
    }

    auto dest_addr = local_addr(SERVER_PORT);
    int sock_fd = check(make_socket(SOCK_STREAM));
    check(connect(sock_fd, (sockaddr*)&dest_addr, sizeof(dest_addr)));

    play_game(sock_fd, mode == 0);

    close(sock_fd);

    return 0;
}