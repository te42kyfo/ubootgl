#pragma once
#include <algorithm>
#include <iostream>

class FrameTimes {
public:
  void add(double t) {

    if (values.size() < 300) {
      values.push_back(t);
    }

    if (pointer >= values.size()) {
      pointer = 0;
    }
    values[pointer] = t;
    pointer++;
  }
  double avg() {
    double sum = 0.0;
    for (int i = 0; i < values.size(); i++) {
      sum += values[i];
    }
    return sum / values.size();
  };

  double low1pct() {
    if (values.empty())
      return 0.0;
    auto vcopy = std::vector<double>(values);
    auto nth_iter = begin(values) + static_cast<int>(values.size() * 0.05);
    std::nth_element(begin(values), nth_iter, end(values));
    return *nth_iter;
  }
  double high1pct() {
    if (values.empty())
      return 0.0;
    auto vcopy = std::vector<double>(values);
    auto nth_iter = begin(values) + static_cast<int>(values.size() * 0.95);
    std::nth_element(begin(values), nth_iter, end(values));
    return *nth_iter;
  }
  double lowest() {
    if (values.empty())
      return 0.0;
    return *std::min_element(begin(values), end(values));
  };
  double largest() {
    if (values.empty())
      return 0.0;
    return *std::max_element(begin(values), end(values));
  };

private:
  int pointer = 0;
  std::vector<double> values;
};
