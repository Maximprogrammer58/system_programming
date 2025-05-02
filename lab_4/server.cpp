#define MIN_NUMBER 0
#define MAX_NUMBER 100
#define BUFFER_SIZE 64

#include "common.hpp"
#include <semaphore.h>
#include <sys/wait.h>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>

sem_t* print_sem;

void no_zombie() {
    struct sigaction sa{};
    sa.sa_handler = SIG_IGN;
    sigaction(SIGCHLD, &sa, nullptr);
}

void print_message(const sockaddr_in& client_addr, const std::string& message) {
    sem_wait(print_sem);
    std::cout << client_addr << message << std::endl;
    sem_post(print_sem);
}

void handle_client(int client_fd, sockaddr_in client_addr) {
    srand(time(nullptr) + getpid());
    int secret = rand() % MAX_NUMBER + MIN_NUMBER;

    print_message(client_addr, " connected. Secret number: " + std::to_string(secret));

    char buffer[BUFFER_SIZE];
    while (true) {
        check(recv(client_fd, buffer, sizeof(buffer), 0));

        int guess = atoi(buffer);

        std::string response;
        if (guess < secret) {
            response = "higher";
        } else if (guess > secret) {
            response = "lower";
        } else {
            response = "correct";
            print_message(client_addr, " guessed the number! " + std::to_string(secret));
        }

        print_message(client_addr, " guessed: " + std::to_string(guess) + " - response: " + response);

        check(send(client_fd, response.c_str(), response.size() + 1, 0));

        if (response == "correct") break;
    }

    print_message(client_addr, " disconnected");

    close(client_fd);
    exit(0);
}

int main() {
    print_sem = check(sem_open("/sem", O_CREAT, 0644, 1));

    no_zombie();

    auto server_addr = local_addr(SERVER_PORT);
    int server_fd = check(make_socket(SOCK_STREAM));

    check(bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)));
    check(listen(server_fd, 10));

    std::cout << "Server started on port " << SERVER_PORT << std::endl;

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = check(accept(server_fd, (sockaddr*)&client_addr, &client_len));

        pid_t pid = check(fork());
        if (pid == 0) {
            close(server_fd);
            handle_client(client_fd, client_addr);
        }
        else {
            close(client_fd);
        }
    }

    sem_close(print_sem);
    sem_unlink("/sem");
    close(server_fd);
    return 0;
}