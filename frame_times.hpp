#pragma once
#include <algorithm>
#include <iostream>

class FrameTimes {
public:
  void add(float t) {

    if (t > 10000)
      return;

    if (pointer >= (int)values.size()) {
      pointer = 0;
    }
    values[pointer] = t;
    pointer++;
  }

  float avg() {
    float sum = 0.0;
    for (int i = 0; i < (int)values.size(); i++) {
      sum += values[i];
    }
    return sum / values.size();
  };

  float low1pct() {
    if (values.empty())
      return 0.0;
    auto vcopy = std::vector<float>(values);
    auto nth_iter = begin(vcopy) + static_cast<int>(vcopy.size() * 0.05);
    std::nth_element(begin(vcopy), nth_iter, end(vcopy));
    return *nth_iter;
  }
  float high1pct() {
    if (values.empty())
      return 0.0;
    auto vcopy = std::vector<float>(values);
    auto nth_iter = begin(vcopy) + static_cast<int>(vcopy.size() * 0.95);
    std::nth_element(begin(vcopy), nth_iter, end(vcopy));
    return *nth_iter;
  }
  float lowest() {
    if (values.empty())
      return 0.0;
    auto vcopy = std::vector<float>(values);
    return *std::min_element(begin(vcopy), end(vcopy));
  };
  float largest() {
    if (values.empty())
      return 0.0;
    auto vcopy = std::vector<float>(values);
    return *std::max_element(begin(vcopy), end(vcopy));
  };
  std::vector<float> &data() { return values; }

private:
  int pointer = 0;
  std::vector<float> values = std::vector<float>(300);
};
