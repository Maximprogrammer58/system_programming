#define MIN_NUMBER 0
#define MAX_NUMBER 100

#include "common.hpp"
#include <iostream>

void play_game(int sock_fd, bool is_automatic) {
    int low = MIN_NUMBER, high = MAX_NUMBER;

    while (true) {
        int guess;
        if (is_automatic) {
            guess = low + (high - low) / 2;
            std::cout << "Trying: " << guess << std::endl;
        } else {
            std::cout << "Enter your guess (" << low << "-" << high << "): ";
            std::cin >> guess;
        }

        check(send(sock_fd, &guess, sizeof(guess), 0));

        Response response;
        check(recv(sock_fd, &response, sizeof(response), 0));

        switch (response) {
            case Response::CORRECT:
                std::cout << "Congratulations! The number was " << guess << std::endl;
                return;
            case Response::HIGHER:
                std::cout << "Higher!" << std::endl;
                low = guess + 1;
                break;
            case Response::LOWER:
                std::cout << "Lower!" << std::endl;
                high = guess - 1;
                break;
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