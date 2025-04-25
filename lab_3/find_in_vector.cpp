#include <iostream>
#include <vector>
#include <pthread.h>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>

#include "include/check.hpp"

struct ThreadData {
    const std::vector<int>& array;
    int target;
    std::vector<int>& results;
    pthread_mutex_t& mutex;
    int& current_index;
};

void* search_element(void* arg) {
    ThreadData* data = (ThreadData*)arg;

    while (true) {
        check_result(pthread_mutex_lock(&data->mutex));

        int index = data->current_index;
        if (index >= data->array.size()) {
            check_result(pthread_mutex_unlock(&data->mutex));
            break;
        }
        data->current_index++;
        check_result(pthread_mutex_unlock(&data->mutex));

        if (data->array[index] == data->target) {
            check_result(pthread_mutex_lock(&data->mutex));
            data->results.push_back(index);
            check_result(pthread_mutex_unlock(&data->mutex));
        }
    }

    return nullptr;
}

std::vector<int> search(const std::vector<int>& array, int target) {
    std::vector<int> results;
    for (int i = 0; i < array.size(); ++i) {
        if (array[i] == target) {
            results.push_back(i);
        }
    }
    return results;
}

std::vector<int> parallel_search(const std::vector<int>& array, int target, int num_threads) {
    std::vector<int> results;
    int current_index = 0;

    pthread_mutex_t mutex;
    check_result(pthread_mutex_init(&mutex, nullptr));

    ThreadData thread_data = {array, target, results, mutex, current_index};

    std::vector<pthread_t> threads(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        check_result(pthread_create(&threads[i], nullptr, search_element, &thread_data));
    }

    for (int i = 0; i < num_threads; ++i) {
        check_result(pthread_join(threads[i], nullptr));
    }

    check_result(pthread_mutex_destroy(&mutex));

    return results;
}

void write_to_file(const std::vector<int>& vec, const char* filename) {
    int fd = check(open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666));
    if (fd == -1) {
        perror("Error opening file");
        return;
    }

    ssize_t bytes_written = check(write(fd, vec.data(), vec.size() * sizeof(int)));
    if (bytes_written == -1) {
        perror("Error writting in file");
    }

    close(fd);
}

void read_to_file(std::vector<int>& vec, const char* filename) {
    int fd = check(open(filename, O_RDONLY));
    if (fd == -1) {
        perror("Error opening file");
        return;
    }
    
    off_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET); 

    size_t count = file_size / sizeof(int);
    vec.resize(count);

    ssize_t bytes_read = check(read(fd, vec.data(), file_size));
    if (bytes_read == -1) {
        perror("Error reading file");
    }

    close(fd);
}

void print_vector(const std::vector<int>& vec) {
    for (int el : vec) {
        std::cout << el << " ";
    }
    std::cout << std::endl;
}

int main() {
    std::srand(std::time(nullptr));

    size_t size = 10000000;
    std::vector<int> array(size);
    for (size_t i = 0; i < size; ++i) {
        array[i] = std::rand() % 100;
    }

    const char* filename = "data.bin";

    write_to_file(array, filename);
    std::vector<int> tmp;
    read_to_file(tmp, filename);

    const int target = 8;
    const int num_threads = 4;

    std::cout << "Размер массива:  " << array.size() << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    std::vector<int> result = search(array, target);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> sequential_time = end - start;
    std::cout << "Sequential search time: " << sequential_time.count() << std::endl;
    std::cout << "Число " << target << " найден " << result.size() << " раз" << std::endl;

    auto p_start = std::chrono::high_resolution_clock::now();
    std::vector<int> result_indices = parallel_search(array, target, num_threads);
    auto p_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> p_sequential_time = p_end - p_start;
    std::cout << "Parallel search time: " << p_sequential_time.count() << std::endl;
    std::cout << "Число " << target << " найден " << result_indices.size() << " раз" << std::endl;

    return 0;
}