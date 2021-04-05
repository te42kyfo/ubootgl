#include "nn_terrain_generator.hpp"
#include <iomanip>
#include <random>

using namespace std;

namespace NNTerrainGenerator {

default_random_engine gen(1293);
uniform_real_distribution<float> dist(0.0, 1.0);
normal_distribution<float> normalDist(0.0, 0.2);

template <int I, int N> class Layer {
public:
  Layer() {
    for (int n = 0; n < N; n++) {
      for (int i = 0; i < I; i++) {
        weights[n][i] = normalDist(gen);
      }
      bias[n] = normalDist(gen);
    }
  }

  float score1(int n, float inputs[]) {
    float score = 0.0f;
    #pragma omp simd
    for (int i = 0; i < I; i++) {
      score += weights[n][i] * inputs[i];
    }
    score += bias[n];

    if (score < 0.0f)
      score *= 0.001f;

    return score;
  }

  void applyError1(int n, float error, float inputs[], float alpha) {

    float derv = (score1(n, inputs) > 0.0f) ? 1.0f : 0.001f;
    for (int i = 0; i < I; i++) {
      weights[n][i] +=
          alpha * (derv * error * inputs[i] - weights[n][i] * 0.0001f);
    }
    bias[n] += alpha * (derv * error * 1.0f - bias[n] * 0.0001f);
  }

  void train(int n, float inputs[], float expected, float alpha) {
    this->applyError1(n, expected - this->score1(n, inputs), inputs, alpha);
  }

  float inputError(int n, int i, float error, float inputs[]) {
    float derv = (score1(n, inputs) > 0.0f) ? 1.0f : 0.001f;
    return weights[n][i] * derv * error;
  }

  void print() {
    for (int n = 0; n < N; n++) {

      std::cout << bias[n] << " ";
      for (int i = 0; i < I; i++) {
        cout << setprecision(2) << setw(7) << weights[n][i] << " ";
      }
      cout << "\n";
    }
  }
  float weights[N][I];
  float bias[N];
};

const int W = 2;
const int D = 4;
const int IN = (W * 2 + 1) * D;

const int HN = 4;

Layer<HN, 1> outputLayer;
Layer<IN, HN> hiddenLayer;

void learn(const Single2DGrid &flag) {

  float alpha = 0.02;
  for (int iter = 0; iter < 20; iter++) {
    float averageError = 0.0f;
    int sampleCount = 0;
    for (int x = D; x < flag.width; x++) {
      for (int y = 0; y < flag.height; y++) {
        std::vector<float> inputs;

        for (int d = 1; d <= D; d++) {
            inputs.push_back(flag(x-d, y));
            for (int iw = 0; iw < W; iw++) {
                inputs.push_back((y - iw - 1 < 0 ? 0.0 : flag(x - d, y - iw - 1)));
                inputs.push_back(
                    (y + iw + 1 >= flag.height ? 0.0 : flag(x - d, y + iw + 1)));
            }
        }

        float hiddenOutputs[HN];
        for (int hn = 0; hn < HN; hn++) {
          hiddenOutputs[hn] = hiddenLayer.score1(hn, inputs.data());
        }
        float error = (outputLayer.score1(0, hiddenOutputs) - flag(x, y));
        averageError += error * error;
        sampleCount++;
        outputLayer.train(0, hiddenOutputs, flag(x, y), alpha);

        for (int hn = 0; hn < HN; hn++) {
          hiddenLayer.applyError1(
              hn,
              outputLayer.inputError(
                  0, hn, flag(x, y) - outputLayer.score1(0, hiddenOutputs),
                  hiddenOutputs),
              inputs.data(), alpha);
        }
      }
    }
    cout << alpha << " " << sqrt(averageError / sampleCount) << "\n";
    alpha *= 0.975f;
    outputLayer.print();
    cout << "\n";
  }
}

std::vector<float> generateLine(const Single2DGrid &flag) {
  vector<float> newLine(flag.height, 0.0f);

  float density = 0.0;
  for (int y = 0; y < flag.height; y++) {
    density += flag(flag.width - 1, y);
  }
  density /= flag.height;
  for (int y = 0; y < flag.height; y++) {
    std::vector<float> inputs;
    int x = flag.width;

    for (int d = 1; d <= D; d++) {
      inputs.push_back(flag(x-d, y));
      for (int iw = 0; iw < W; iw++) {
        inputs.push_back((y - iw - 1 < 0 ? 0.0 : flag(x - d, y - iw - 1)));
        inputs.push_back(
          (y + iw + 1 >= flag.height ? 0.0 : flag(x - d, y + iw + 1)));
      }
    }

    float hiddenOutputs[HN];
    for (int hn = 0; hn < HN; hn++) {
      hiddenOutputs[hn] = hiddenLayer.score1(hn, inputs.data());
    }
    float score = outputLayer.score1(0, hiddenOutputs);

    if (score + normalDist(gen) * 1.0 > density*0.7) {
      newLine[y] = 1.0;
    } else {
      newLine[y] = 0.0;
    }
  }
  newLine[0] = 0.0f;
  newLine[1] = 0.0f;
  newLine[2] = 0.0f;
  newLine[3] = 0.0f;
  newLine[4] = 0.0f;
  newLine[flag.height - 1] = 0.0f;
  newLine[flag.height - 5] = 0.0f;
  newLine[flag.height - 2] = 0.0f;
  newLine[flag.height - 3] = 0.0f;
  newLine[flag.height - 4] = 0.0f;

  return newLine;
}
} // namespace TerrainGenerator
