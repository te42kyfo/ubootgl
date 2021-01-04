#pragma once
#include "components.hpp"
#include "db2dgrid.hpp"
#include "entt/entity/registry.hpp"
#include <functional>
#include <iomanip>
#include <iostream>
#include <random>

template <typename T, int, int> class Matrix2D;

template <typename T, int N, int M>
std::ostream &operator<<(std::ostream &stream, const Matrix2D<T, N, M> &matrix);

template <typename T, int N, int M> class Matrix2D {
public:
  Matrix2D(){};
  Matrix2D(T val) { set(val); };
  Matrix2D(std::initializer_list<T> l) {
    for (int n = 0; n < N; n++) {
      for (int m = 0; m < M; m++) {
        data[n][m] = *(begin(l) + n);
      }
    }
  }

  std::array<T, M> &operator[](int n) { return data[n]; }
  std::array<T, M> operator[](int n) const { return data[n]; }

  template <typename G> void randInitialize(G &gen) {
    for (int n = 0; n < N; n++) {
      for (int m = 0; m < M; m++) {
        data[n][m] = gen();
      }
    }
  }

  Matrix2D &set(T value) {
    for (int n = 0; n < N; n++) {
      for (int m = 0; m < M; m++) {
        data[n][m] = value;
      }
    }
    return *this;
  }

  void clamp(T vmin, T vmax) {
    for (int n = 0; n < N; n++) {
      for (int m = 0; m < M; m++) {
        data[n][m] = std::max(vmin, std::min(data[n][m], vmax));
      }
    }
  }

  Matrix2D operator+(const Matrix2D &other) {
    auto result = *this;
    for (int n = 0; n < N; n++) {
      for (int m = 0; m < M; m++) {
        result[n][m] += other[n][m];
      }
    }
    return result;
  }

  template <int K> Matrix2D<T, N, K> operator*(const Matrix2D<T, M, K> &other) {
    Matrix2D<T, N, K> result(0.0);
    for (int n = 0; n < N; n++) {
      for (int k = 0; k < K; k++) {
        result[n][k] = 0.0;
        for (int m = 0; m < M; m++) {
          result[n][k] += data[n][m] * other[m][k];
        }
      }
    }
    return result;
  }

  Matrix2D<T, N, M> operator*(T val) {
    Matrix2D<T, N, M> result(0.0);
    for (int n = 0; n < N; n++) {
      for (int m = 0; m < M; m++) {
        result[n][m] = data[n][m] * val;
      }
    }
    return result;
  }

  Matrix2D<T, M, N> transpose() {
    Matrix2D<T, M, N> transposed(0.0);
    for (int n = 0; n < N; n++) {
      for (int m = 0; m < M; m++) {
        transposed[m][n] = data[n][m];
      }
    }
    return transposed;
  }

protected:
  std::array<std::array<T, M>, N> data;
};

template <typename T, int N, int M>
std::ostream &operator<<(std::ostream &stream,
                         const Matrix2D<T, N, M> &matrix) {
  for (int n = 0; n < N; n++) {
    for (int m = 0; m < M; m++) {
      std::cout << std::setprecision(2) << std::setw(6) << matrix[n][m] << " ";
    }
    std::cout << "\n";
  }
  return stream;
}

template <int neuronCount, int inputCount> class NeuronLayer {

  Matrix2D<float, neuronCount, inputCount> weights;
  Matrix2D<float, neuronCount, 1> bias;

  Matrix2D<float, neuronCount, inputCount> weightsGrad;
  Matrix2D<float, neuronCount, 1> biasGrad;

  std::vector<Matrix2D<float, 7, 1>> lastInputs;
  std::vector<Matrix2D<float, 7, 1>> lastOutputs;
};

void classicSwarmAI(entt::registry &registry, const Single2DGrid &flag,
                    float *vx, float *vy, int vwidth, float h);
