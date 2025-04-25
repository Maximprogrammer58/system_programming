#include <iostream>
#include <cstdlib>
#include <cmath>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <chrono>
#include <stdexcept>

#include "../include/Matrix.h"
#include "../include/check.hpp"

Matrix::Matrix(size_t n) : _size(n), _data(n * n, 0.0) {}

Matrix::Matrix(size_t n, double min_value, double max_value) : _size(n), _data(n * n) {
    if (min_value > max_value) {
        throw std::invalid_argument("min_value must be less than or equal to max_value");
    }
    fill_random(min_value, max_value);
}

Matrix::Matrix(const std::vector<double>& matrix) {
    double size_d = std::sqrt(matrix.size());
    if (std::floor(size_d) != size_d) {
        throw std::invalid_argument("Input vector length must be a perfect square");
    }
    _size = static_cast<size_t>(size_d);
    _data = matrix;
}

void Matrix::fill_random(double min_val, double max_val) {
    if (min_val > max_val) {
        throw std::invalid_argument("min_val must be less than or equal to max_val");
    }

    double range = max_val - min_val;
    for (size_t i = 0; i < _size * _size; ++i) {
        _data[i] = min_val + (static_cast<double>(std::rand()) / RAND_MAX) * range;
    }
}

void Matrix::write_to_file(const std::string& filename) const {
    int fd = check(open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644));
    if (fd == -1) {
        throw std::runtime_error("Failed to open file for writing");
    }

    ssize_t bytes_written = check(write(fd, _data.data(), _size * _size * sizeof(double)));
    if (bytes_written == -1 || static_cast<size_t>(bytes_written) != _size * _size * sizeof(double)) {
        close(fd);
        throw std::runtime_error("Failed to write matrix to file");
    }
    close(fd);
}

Matrix Matrix::read_from_file(const std::string& filename) {
    int fd = check(open(filename.c_str(), O_RDONLY));
    if (fd == -1) {
        throw std::runtime_error("Failed to open file for reading");
    }

    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) {
        close(fd);
        throw std::runtime_error("Failed to get file size");
    }

    off_t file_size = file_stat.st_size;
    size_t matrix_size = static_cast<size_t>(std::sqrt(file_size / sizeof(double)));

    if (matrix_size * matrix_size * sizeof(double) != static_cast<size_t>(file_size)) {
        close(fd);
        throw std::runtime_error("Invalid file size for square matrix");
    }

    Matrix result(matrix_size);
    ssize_t bytes_read = check(read(fd, result._data.data(), file_size));
    if (bytes_read == -1 || static_cast<size_t>(bytes_read) != file_size) {
        close(fd);
        throw std::runtime_error("Failed to read matrix from file");
    }

    close(fd);
    return result;
}

void Matrix::print() const {
    std::cout << "Matrix " << _size << "x" << _size << ":\n";
    for (size_t i = 0; i < _size; ++i) {
        for (size_t j = 0; j < _size; ++j) {
            std::cout << _data[i * _size + j] << "\t";
        }
        std::cout << "\n";
    }
}

size_t Matrix::size() const {
    return _size;
}

double Matrix::operator()(size_t i, size_t j) const {
    if (i >= _size || j >= _size) {
        throw std::out_of_range("Matrix indices out of range");
    }
    return _data[i * _size + j];
}

double& Matrix::operator()(size_t i, size_t j) {
    if (i >= _size || j >= _size) {
        throw std::out_of_range("Matrix indices out of range");
    }
    return _data[i * _size + j];
}

Matrix Matrix::multiply(const Matrix& other) const {
    if (_size != other._size) {
        throw std::invalid_argument("Matrix dimensions must agree for multiplication");
    }

    Matrix result(_size);
    for (size_t i = 0; i < _size; ++i) {
        for (size_t j = 0; j < _size; ++j) {
            double sum = 0.0;
            for (size_t k = 0; k < _size; ++k) {
                sum += (*this)(i, k) * other(k, j);
            }
            result(i, j) = sum;
        }
    }
    return result;
}

void* Matrix::multiply_partial(void* arg) {
    ThreadData* data = static_cast<ThreadData*>(arg);
    const Matrix& mat1 = *(data->mat1);
    const Matrix& mat2 = *(data->mat2);
    Matrix& result = *(data->result);
    size_t size = mat1.size();

    for (size_t i = data->start_row; i < data->end_row; ++i) {
        for (size_t j = 0; j < size; ++j) {
            double sum = 0.0;
            for (size_t k = 0; k < size; ++k) {
                sum += mat1(i, k) * mat2(k, j);
            }
            result(i, j) = sum;
        }
    }

    return nullptr;
}

Matrix Matrix::parallel_multiply(const Matrix& other, size_t num_threads) const {
    if (_size != other._size) {
        throw std::invalid_argument("Matrix dimensions must agree for multiplication");
    }
    if (num_threads == 0) {
        throw std::invalid_argument("Number of threads must be positive");
    }

    Matrix result(_size);
    if (_size == 0) {
        return result;
    }

    num_threads = std::min(num_threads, _size);
    std::vector<pthread_t> threads(num_threads);
    std::vector<ThreadData> thread_data(num_threads);

    size_t rows_per_thread = _size / num_threads;
    size_t remainder = _size % num_threads;
    size_t start_row = 0;

    for (size_t i = 0; i < num_threads; ++i) {
        size_t end_row = start_row + rows_per_thread + (i < remainder ? 1 : 0);

        thread_data[i] = {this, &other, &result, start_row, end_row};

        if (check_result(pthread_create(&threads[i], nullptr, multiply_partial, &thread_data[i]))) {
            for (size_t j = 0; j < i; ++j) {
                check_result(pthread_join(threads[j], nullptr));
            }
            throw std::runtime_error("Failed to create thread");
        }

        start_row = end_row;
    }

    for (auto& thread : threads) {
        if (check_result(pthread_join(thread, nullptr))) {
            throw std::runtime_error("Failed to join thread");
        }
    }

    return result;
}