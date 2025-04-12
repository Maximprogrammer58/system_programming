#define MIN_VALUE 1
#define MAX_VALUE 10
#define NUM_ROUNDS 30
#define TIME_DELAY 1

#include <algorithm>
#include <signal.h>
#include <iostream>
#include <random>
#include <unistd.h>
#include <wait.h>
#include "check.hpp"

volatile sig_atomic_t last_sig;
volatile sig_atomic_t value;

bool process_exists(pid_t p) {
    if (kill(p, 0) == -1 && errno == ESRCH)
        return false;
    return true;
}

void check_process_exists(pid_t p) {
    if (!process_exists(p)) {
        std::cerr << "Opponent process " << p << " does not exist anymore!" << std::endl;
        exit(1);
    }
}

void no_zombie() {
    struct sigaction s{};
    s.sa_handler = SIG_IGN;
    check(sigaction(SIGCHLD, &s, NULL));
}

void guess_handler(int sig, siginfo_t *si, void *ctx) {
    last_sig = sig;
    value = si->si_value.sival_int;
}

void handler(int sig) {
    last_sig = sig;
}

void wait_signal(pid_t opponent, const sigset_t &signal_mask) {
    while (true) {
        alarm(1);
        sigsuspend(&signal_mask);
        alarm(0);

        if (last_sig != SIGALRM)
            break;

        check_process_exists(opponent);
    }
}

void timeout_handler(int sig) { _exit(1); }

int generate_random_number() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> distr(MIN_VALUE, MAX_VALUE);
    return distr(gen);
}

void setup_signal_handlers() {
    struct sigaction hit_action{}, miss_action{}, guess_action{}, timeout_action{};
    hit_action.sa_handler = handler;
    check(sigaction(SIGUSR1, &hit_action, NULL));

    miss_action.sa_handler = handler;
    check(sigaction(SIGUSR2, &miss_action, NULL));

    guess_action.sa_sigaction = guess_handler;
    guess_action.sa_flags = SA_SIGINFO;
    check(sigaction(SIGRTMAX, &guess_action, NULL));

    timeout_action.sa_handler = timeout_handler;
    check(sigaction(SIGALRM, &timeout_action, NULL));
}

void play_as_guesser(pid_t opponent_pid, const sigset_t &signal_mask) {
    wait_signal(opponent_pid, signal_mask);

    int num_tries = 1;
    for (int el = MAX_VALUE; el > 0; --el) {
        check_process_exists(opponent_pid);
        check(sigqueue(opponent_pid, SIGRTMAX, sigval{el}));
        std::cout << "PID [" << getpid() << "] think it's " << el << std::endl;

        wait_signal(opponent_pid, signal_mask);

        if (last_sig == SIGUSR1) {
            std::cout << "That's right, it's " << el << std::endl;
            break;
        }
        ++num_tries;
    }
    std::cout << "Number of attempts: " << num_tries << " tries" << std::endl;
}

void play_as_hoster(pid_t opponent_pid, int round, const sigset_t &signal_mask) {
    const int random_num = generate_random_number();
    std::cout << "\nRound " << round << std::endl;
    std::cout << "[PID " << getpid() << "] I guessed a number from " << MIN_VALUE << " to " << MAX_VALUE
              << ". Try to guess it! " << std::endl;

    check_process_exists(opponent_pid);
    check(kill(opponent_pid, SIGUSR2));

    for (int i = 1; i <= MAX_VALUE; ++i) {
        wait_signal(opponent_pid, signal_mask);
        if (value == random_num) {
            check(kill(opponent_pid, SIGUSR1));
            break;
        }
        check(kill(opponent_pid, SIGUSR2));
    }
}

void play_round(pid_t pid, pid_t parent_pid, int round, const sigset_t &signal_mask) {
    if (round % 2 == 0) {
        (pid == 0) ? play_as_hoster(parent_pid, round, signal_mask) : play_as_guesser(pid, signal_mask);
    } else {
        (pid > 0) ? play_as_hoster(pid, round, signal_mask) : play_as_guesser(parent_pid, signal_mask);
    }
}

int main() {
    no_zombie();
    setup_signal_handlers();

    sigset_t set;
    sigfillset(&set);
    sigprocmask(SIG_BLOCK, &set, &set);
    sigdelset(&set, SIGUSR1);
    sigdelset(&set, SIGUSR2);
    sigdelset(&set, SIGRTMAX);
    sigdelset(&set, SIGALRM);

    pid_t parent_pid = getpid();
    pid_t pid = check(fork());

    for (int round = 1; round <= NUM_ROUNDS; ++round) {
        sleep(TIME_DELAY);
        play_round(pid, parent_pid, round, set);
    }

    return 0;
}
