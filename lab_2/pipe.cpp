#define NUM_ROUNDS 10
#define TIME_DELAY 3

#include <algorithm>
#include <iostream>
#include <random>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "check.hpp"

pid_t pid, parent_p;

int generate_random_number(int max_value) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> distr(1, max_value);
    return distr(gen);
}

void play_as_guesser(int pipe_for_write, int pipe_for_read) {
    int count = 0;
    int max_value;
    ssize_t read_size = check(read(pipe_for_read, &max_value, sizeof(max_value)));
    if (read_size == 0)
        exit(0);

    bool guessed = false;
    for (int value = max_value; value > 0; --value) {
        check(write(pipe_for_write, &value, sizeof(value)));
        read_size = check(read(pipe_for_read, &guessed, sizeof(guessed)));
        if (read_size == 0)
            exit(0);
        if (!guessed) {
            std::cout << "PID [" << getpid() << "]" << " value = " << value << std::endl;
            count += 1;
        }
        else {
            std::cout << "PID [" << getpid() << "]" << " need " << count << " attempts to win" << std::endl;
            break;
        }
    }
}

void play_as_hoster(int round, int pipe_for_write, int pipe_for_read, int max_value) {
    const int number = generate_random_number(max_value);
    check(write(pipe_for_write, &max_value, sizeof(max_value)));

    std::cout << "\nRound " << round << std::endl;
    std::cout << "PID [" << getpid() << "]" << " wish a number = " << number << std::endl;

    bool guessed = false;
    while (!guessed) {
        int value;
        ssize_t read_size = check(read(pipe_for_read, &value, sizeof(value)));
        if (read_size == 0)
            exit(0);
        if (value == number) {
            std::cout << "YEEEES. YOU DID IT!!!" << std::endl;
            guessed = true;
            check(write(pipe_for_write, &guessed, sizeof(guessed)));
            break;
        }
        std::cout << "You not guess." << std::endl;
        check(write(pipe_for_write, &guessed, sizeof(guessed)));
    }
}

void play_round(int round, int pipe_for_write, int pipe_for_read, int max_value) {
    if (round % 2 == 0) {
        (pid == 0) ? play_as_hoster(round, pipe_for_write, pipe_for_read, max_value) : play_as_guesser(pipe_for_write, pipe_for_read);
    }
    else {
        (pid > 0) ? play_as_hoster(round, pipe_for_write, pipe_for_read, max_value) : play_as_guesser(pipe_for_write, pipe_for_read);
    }
}

int main() {
    int max_value = 10;
    int fd_1[2], fd_2[2];

    check(pipe(fd_1));
    check(pipe(fd_2));

    parent_p = getpid();
    pid = check(fork());

    int pipe_for_write, pipe_for_read;

    if (pid == 0) {
        pipe_for_read = fd_2[0];
        pipe_for_write = fd_1[1];
        close(fd_1[0]);
        close(fd_2[1]);
    } else {
        pipe_for_read = fd_1[0];
        pipe_for_write = fd_2[1];
        close(fd_1[1]);
        close(fd_2[0]);
    }

    for (int round = 1; round <= NUM_ROUNDS; round++) {
        sleep(TIME_DELAY);
        play_round(round, pipe_for_write, pipe_for_read, max_value);
    }

    close(pipe_for_read);
    close(pipe_for_write);

    return 0;
}
