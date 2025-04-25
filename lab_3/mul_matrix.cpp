#include <iostream>
#include <chrono>
#include <cmath>

#include "include/Matrix.h"

int main() {
    try {
        std::srand(std::time(nullptr));

        Matrix m1(100, 1, 10);
        Matrix m2(100, 1, 10);

        std::cout << "Matrix 1:\n";
        m1.write_to_file("matrix_1.bin");
        //m1.print();
        std::cout << "\nMatrix 2:\n";
        m2.write_to_file("matrix_2.bin");
        //m2.print();

        // Sequential multiplication
        auto start = std::chrono::high_resolution_clock::now();
        Matrix m3 = m1.multiply(m2);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> sequential_time = end - start;
        std::cout << "\nMatrix 1 * Matrix 2 (single-threaded):\n";
        //m3.print();
        std::cout << "Sequential multiplication time: " << sequential_time.count() << "s\n";

        // Parallel multiplication
        start = std::chrono::high_resolution_clock::now();
        Matrix m4 = m1.parallel_multiply(m2);
        end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> parallel_time = end - start;
        std::cout << "\nMatrix 1 * Matrix 2 (multi-threaded):\n";
        //m4.print();
        std::cout << "Parallel multiplication time: " << parallel_time.count() << "s\n";

        m4.write_to_file("result.bin");
        std::cout << "\nResult written to file.\n";

        Matrix m5 = Matrix::read_from_file("result.bin");
        std::cout << "\nMatrix read from file:\n";
        //m5.print();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}