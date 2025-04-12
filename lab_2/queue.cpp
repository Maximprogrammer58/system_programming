#define MIN_VALUE 1
#define MAX_VALUE 10
#define NUM_ROUNDS 10
#define WAIT_TIME 2

#include <algorithm>
#include <fcntl.h>
#include <iostream>
#include <mqueue.h>
#include <random>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#include "check.hpp"

const int SIG_WIN = -1;
const int SIG_FAIL = -2;

bool process_exists(pid_t p) {
    if (kill(p, 0) == -1 && errno == ESRCH)
        return false;
    return true;
}

void no_zombie() {
    struct sigaction s {};
    s.sa_handler = SIG_IGN;
    check(sigaction(SIGCHLD, &s, NULL));
}

void safe_send(mqd_t queue, const int* value, pid_t opponent) {
    timespec timeout{};
    while (true) {
        check(clock_gettime(CLOCK_REALTIME, &timeout));
        timeout.tv_sec += WAIT_TIME;
        if (check_except(mq_timedsend(queue, (char*)value, sizeof(int), 0, &timeout), ETIMEDOUT) == 0) {
            break;
        }
        if (!process_exists(opponent)) {
            exit(EXIT_SUCCESS);
        }
    }
}

void safe_receive(mqd_t queue, int* value, pid_t opponent) {
    timespec timeout{};
    while (true) {
        check(clock_gettime(CLOCK_REALTIME, &timeout));
        timeout.tv_sec += WAIT_TIME;
        if (check_except(mq_timedreceive(queue, (char*)value, sizeof(int), nullptr, &timeout),ETIMEDOUT) >= 0) {
            break;
        }
        if (!process_exists(opponent)) {
            exit(EXIT_SUCCESS);
        }
    }
}

int generate_random_number() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> distr(MIN_VALUE, MAX_VALUE);
    return distr(gen);
}

void play_as_guesser(mqd_t mq_write, mqd_t mq_read, pid_t opponent) {
    int count = 0;
    int buffer = 0;

    for (int value = MAX_VALUE; value > 0; --value) {
        safe_receive(mq_read, &buffer, opponent);

        if (buffer == SIG_FAIL) {
            std::cout << "PID [" << getpid() << "] value = " << value << std::endl;
            count++;
            buffer = value;
            safe_send(mq_write, &buffer, opponent);
        }
        else if (buffer == SIG_WIN) {
            std::cout << "PID [" << getpid() << "] need " << count << " attempts to win" << std::endl;
            break;
        }
    }
}

void play_as_hoster(mqd_t mq_write, mqd_t mq_read, int round, pid_t opponent) {
    std::cout << "\nRound " << round << std::endl;

    const int number = generate_random_number();

    std::cout << "PID [" << getpid() << "] wish a number = " << number << std::endl;

    int buffer = SIG_FAIL;
    safe_send(mq_write, &buffer, opponent);

    while (true) {
        safe_receive(mq_read, &buffer, opponent);

        if (buffer == number) {
            buffer = SIG_WIN;
            safe_send(mq_write, &buffer, opponent);
            std::cout << "YEEEES. YOU DID IT!!!" << std::endl;
            break;
        }
        std::cout << "PID [" << getpid() << "] did not guess." << std::endl;
        buffer = SIG_FAIL;
        safe_send(mq_write, &buffer, opponent);
    }
}

void play_round(mqd_t mq_p_to_c, mqd_t mq_c_to_p, int round, pid_t parent_p, pid_t child_p) {
    if (child_p != 0) {
        (round % 2 != 0) ? play_as_hoster(mq_p_to_c, mq_c_to_p, round, child_p)
                          : play_as_guesser(mq_p_to_c, mq_c_to_p, child_p);
    }
    else {
        (round % 2 != 0) ? play_as_guesser(mq_c_to_p, mq_p_to_c, parent_p)
                          : play_as_hoster(mq_c_to_p, mq_p_to_c, round, parent_p);
    }
}

int main() {
    no_zombie();

    const char QUEUE_PARENT_TO_CHILD[] = "/mq_p_to_c";
    const char QUEUE_CHILD_TO_PARENT[] = "/mq_c_to_p";

    mq_unlink(QUEUE_PARENT_TO_CHILD);
    mq_unlink(QUEUE_CHILD_TO_PARENT);

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 1;
    attr.mq_msgsize = sizeof(int);

    mqd_t mq_p_to_c = check(mq_open(QUEUE_PARENT_TO_CHILD, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, &attr));
    mqd_t mq_c_to_p = check(mq_open(QUEUE_CHILD_TO_PARENT, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, &attr));

    pid_t parent_p = getpid();
    pid_t child_p = check(fork());

    for (int round = 1; round <= NUM_ROUNDS; round++) {
        sleep(WAIT_TIME);
        play_round(mq_p_to_c, mq_c_to_p, round, parent_p, child_p);
    }

    if (child_p > 0) {
        check(mq_close(mq_p_to_c));
        check(mq_close(mq_c_to_p));
        check(mq_unlink(QUEUE_PARENT_TO_CHILD));
        check(mq_unlink(QUEUE_CHILD_TO_PARENT));
    }

    return 0;
}