#ifndef MATRIX_H
#define MATRIX_H

#include <vector>
#include <string>
#include <stdexcept>

class Matrix {
  std::vector<double> _data;
  size_t _size;

  struct ThreadData {
    const Matrix* mat1;
    const Matrix* mat2;
    Matrix* result;
    size_t start_row;
    size_t end_row;
  };

public:
  Matrix(size_t n);
  Matrix(size_t n, double min_value, double max_value);
  explicit Matrix(const std::vector<double>& matrix);

  void fill_random(double min_val = 0.0, double max_val = 1.0);
  void write_to_file(const std::string& filename) const;
  static Matrix read_from_file(const std::string& filename);
  void print() const;
  size_t size() const;

  double operator()(size_t i, size_t j) const;
  double& operator()(size_t i, size_t j);

  Matrix multiply(const Matrix& other) const;
  Matrix parallel_multiply(const Matrix& other, size_t num_threads = 4) const;

private:
  static void* multiply_partial(void* arg);
};

#endif