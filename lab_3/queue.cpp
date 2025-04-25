#include <pthread.h>
#include <queue>
#include <iostream>
#include <optional>
#include <unistd.h>
#include <vector>

#include "include/check.hpp"

template <typename T>
class ThreadSafeQueue {
    std::queue<T> queue;
    const size_t max_size;
    mutable pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
    bool done;

public:
    ThreadSafeQueue(size_t max_size) : max_size(max_size), done(false) {
        check_result(pthread_mutex_init(&mutex, nullptr));
        check_result(pthread_cond_init(&not_full, nullptr));
        check_result(pthread_cond_init(&not_empty, nullptr));
    }

    ~ThreadSafeQueue() {
        check_result(pthread_mutex_destroy(&mutex));
        check_result(pthread_cond_destroy(&not_full));
        check_result(pthread_cond_destroy(&not_empty));
    }

    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue(ThreadSafeQueue&&) = delete;

    void enqueue(const T& v) {
        check_result(pthread_mutex_lock(&mutex));
        while (queue.size() >= max_size && !done)
            check_result(pthread_cond_wait(&not_full, &mutex));
        if (done) {
            check_result(pthread_mutex_unlock(&mutex));
            return;
        }
        queue.push(v);
        check_result(pthread_cond_signal(&not_empty));
        check_result(pthread_mutex_unlock(&mutex));
    }

    T dequeue() {
        check_result(pthread_mutex_lock(&mutex));
        while (queue.empty() && !done)
            check_result(pthread_cond_wait(&not_empty, &mutex));
        if (queue.empty() && done) {
            check_result(pthread_mutex_unlock(&mutex));
            return T();
        }
        T val = queue.front();
        queue.pop();
        check_result(pthread_cond_signal(&not_full));
        check_result(pthread_mutex_unlock(&mutex));
        return val;
    }

    std::optional<T> try_dequeue() {
        check_result(pthread_mutex_lock(&mutex));
        if (queue.empty()) {
            check_result(pthread_mutex_unlock(&mutex));
            return std::nullopt;
        }
        T val = queue.front();
        queue.pop();
        check_result(pthread_cond_signal(&not_full));
        check_result(pthread_mutex_unlock(&mutex));
        return val;
    }

    bool try_enqueue(const T& v) {
        check_result(pthread_mutex_lock(&mutex));
        if (queue.size() >= max_size) {
            check_result(pthread_mutex_unlock(&mutex));
            return false;
        }
        queue.push(v);
        check_result(pthread_cond_signal(&not_empty));
        check_result(pthread_mutex_unlock(&mutex));
        return true;
    }

    bool full() const {
        check_result(pthread_mutex_lock(&mutex));
        bool result = queue.size() >= max_size;
        check_result(pthread_mutex_unlock(&mutex));
        return result;
    }

    bool empty() const {
        check_result(pthread_mutex_lock(&mutex));
        bool result = queue.empty();
        check_result(pthread_mutex_unlock(&mutex));
        return result;
    }

    void set_done() {
        check_result(pthread_mutex_lock(&mutex));
        done = true;
        check_result(pthread_cond_broadcast(&not_empty));
        check_result(pthread_cond_broadcast(&not_full));
        check_result(pthread_mutex_unlock(&mutex));
    }
};


pthread_mutex_t print_mutex;

struct ThreadArgs {
    int id;
    int items_to_produce;
    ThreadSafeQueue<int>* queue;
};

void* producer_thread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    int id = args->id;
    int count = args->items_to_produce;
    ThreadSafeQueue<int>& queue = *args->queue;

    for (int i = 0; i < count; ++i) {
        int val = id * 100 + i;
        queue.enqueue(val);

        check_result(pthread_mutex_lock(&print_mutex));
        std::cout << "{Producer} " << id << " enqueued " << val << std::endl;
        check_result(pthread_mutex_unlock(&print_mutex));

        usleep(100000);
    }
    return nullptr;
}

void* consumer_thread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    int id = args->id;
    ThreadSafeQueue<int>& queue = *args->queue;

    while (true) {
        int val = queue.dequeue();
        if (val == 0 && queue.empty()) break;

        check_result(pthread_mutex_lock(&print_mutex));
        std::cout << "{Consumer} " << id << " dequeued " << val << std::endl;
        check_result(pthread_mutex_unlock(&print_mutex));

        usleep(150000);
    }
    return nullptr;
}


int main() {
    check_result(pthread_mutex_init(&print_mutex, nullptr));

    const int PRODUCERS = 10;
    const int CONSUMERS = 2;
    const int ITEM_PER_PRODUCER = 2;
    const int QUEUE_MAX_LEN = 10;

    ThreadSafeQueue<int> queue(QUEUE_MAX_LEN);

    std::vector<pthread_t> producer_threads(PRODUCERS);
    std::vector<pthread_t> consumer_threads(CONSUMERS);
    std::vector<ThreadArgs> producer_args(PRODUCERS);
    std::vector<ThreadArgs> consumer_args(CONSUMERS);

    for (int i = 0; i < CONSUMERS; ++i) {
        consumer_args[i] = {i + 1, 0, &queue};
        check_result(pthread_create(&consumer_threads[i], nullptr, consumer_thread, &consumer_args[i]));
    }

    for (int i = 0; i < PRODUCERS; ++i) {
        producer_args[i] = {i + 1, ITEM_PER_PRODUCER, &queue};
        check_result(pthread_create(&producer_threads[i], nullptr, producer_thread, &producer_args[i]));
    }

    for (int i = 0; i < PRODUCERS; ++i) {
        check_result(pthread_join(producer_threads[i], nullptr));
    }

    queue.set_done();

    for (int i = 0; i < CONSUMERS; ++i) {
        check_result(pthread_join(consumer_threads[i], nullptr));
    }

    check_result(pthread_mutex_lock(&print_mutex));
    std::cout << "Все потоки завершены. Очередь пуста: " << (queue.empty() ? "Да" : "Нет");
    check_result(pthread_mutex_unlock(&print_mutex));
    check_result(pthread_mutex_destroy(&print_mutex));

    return 0;
}
