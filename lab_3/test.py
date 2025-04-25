import numpy as np


def read_matrix_from_file(filename, size):
    """Чтение матрицы из бинарного файла"""
    with open(filename, 'rb') as f:
        data = np.fromfile(f, dtype=np.float64)
    return data.reshape((size, size))


if __name__ == "__main__":
    matrix_size = 10

    matrix1 = read_matrix_from_file('cmake-build-debug/matrix_1.txt', matrix_size)
    matrix2 = read_matrix_from_file('cmake-build-debug/matrix_2.txt', matrix_size)
    result = read_matrix_from_file('cmake-build-debug/result.bin', matrix_size)

    numpy_result = np.dot(matrix1, matrix2)

    if np.allclose(result, numpy_result, atol=1e-8):
        print("Результаты совпадают с точностью до 1e-8")
    else:
        print("Результаты НЕ совпадают с точностью до 1e-8")