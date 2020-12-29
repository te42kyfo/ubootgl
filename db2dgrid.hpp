#pragma once

#include <algorithm>
#include <assert.h>
#include <glm/glm.hpp>
#include <immintrin.h>
#include <iomanip>
#include <iostream>
#include <vector>

class Single2DGrid {
public:
  Single2DGrid(int width, int height)
      : width(width), height(height), v(width * height, 0.0f) {}

  Single2DGrid() : width(0), height(0){};

  int idx(int x, int y) const { return y * width + x; }
  float *data() { return v.data(); }

  float &operator()(int x, int y) {
    assert(x < width && x >= 0 && y < height && y >= 0);
    return v[idx(x, y)];
  }
  float operator()(int x, int y) const {
    assert(x < width && x >= 0 && y < height && y >= 0);
    return v[idx(x, y)];
  }

  float &operator()(glm::ivec2 c) { return (*this)(c.x, c.y); }
  float operator()(glm::ivec2 c) const { return (*this)(c.x, c.y); }

  float operator=(float val) {
    std::fill(std::begin(v), std::end(v), val);
    return val;
  }
  int width, height;

  void print() {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        std::cout << std::setprecision(2) << std::setw(2) << (*this)(x, y)
                  << " ";
      }
      std::cout << "\n";
    }
  }

private:
  std::vector<float> v;
};

template <typename GridType>
inline float bilinearSample(const GridType &grid, glm::vec2 c) {

  c = glm::clamp(c, glm::vec2(1.0f, 1.0f),
                 glm::vec2(grid.width - 1.0f, grid.height - 1.0f));

  glm::ivec2 ic = c;
  glm::vec2 st = fract(c);

  float v1 = grid(ic.x, ic.y);
  float v2 = grid(ic.x + 1, ic.y);
  float v3 = grid(ic.x, ic.y + 1);
  float v4 = grid(ic.x + 1, ic.y + 1);
  float vm1 = v1 + (v2 - v1) * st.x;
  float vm2 = v3 + (v4 - v3) * st.x;
  return vm1 + (vm2 - vm1) * st.y;
}

template <typename GridType>
inline __m256 gridGather(const GridType &grid, __m256i x, __m256i y) {
  __m256i idx =
      _mm256_add_epi32(x, _mm256_mullo_epi32(y, _mm256_set1_epi32(grid.width)));

  auto result = _mm256_i32gather_ps(grid.data(), idx, 4);
  return result;
}

template <typename GridType>
inline __m256 bilinearSample(const GridType &grid, __m256 cx, __m256 cy) {
  cx = _mm256_max_ps(_mm256_min_ps(cx, _mm256_set1_ps(grid.width - 1.0f)),
                     _mm256_set1_ps(1.0f));
  cy = _mm256_max_ps(_mm256_min_ps(cy, _mm256_set1_ps(grid.height - 1.0f)),
                     _mm256_set1_ps(1.0f));

  __m256i icx = _mm256_cvttps_epi32(cx);
  __m256i icy = _mm256_cvttps_epi32(cy);

  __m256 stx = _mm256_sub_ps(cx, _mm256_round_ps(cx, _MM_FROUND_TRUNC));
  __m256 sty = _mm256_sub_ps(cy, _mm256_round_ps(cy, _MM_FROUND_TRUNC));

  __m256 v1 = gridGather(grid, icx, icy);
  __m256 v2 =
      gridGather(grid, _mm256_add_epi32(icx, _mm256_set1_epi32(1)), icy);
  __m256 v3 =
      gridGather(grid, icx, _mm256_add_epi32(icy, _mm256_set1_epi32(1)));
  __m256 v4 = gridGather(grid, _mm256_add_epi32(icx, _mm256_set1_epi32(1)),
                         _mm256_add_epi32(icy, _mm256_set1_epi32(1)));

  __m256 vm1 = _mm256_fmadd_ps(_mm256_sub_ps(v2, v1), stx, v1);
  __m256 vm2 = _mm256_fmadd_ps(_mm256_sub_ps(v4, v3), stx, v3);

  return _mm256_fmadd_ps(_mm256_sub_ps(vm2, vm1), sty, vm1);
}

inline __m256 CubicHermite(__m256 t, __m256 A, __m256 B, __m256 C, __m256 D) {
  __m256 a = -A / 2.0f + (3.0f * B) / 2.0f - (3.0f * C) / 2.0f + D / 2.0f;
  __m256 b = A - (5.0f * B) / 2.0f + 2.0f * C - D / 2.0f;
  __m256 c = -A / 2.0f + C / 2.0f;
  __m256 d = B;

  return a * t * t * t + b * t * t + c * t + d;
}

template <typename GridType>
inline __m256 bicubicSample(const GridType &grid, __m256 cx, __m256 cy) {
  cx = _mm256_max_ps(_mm256_min_ps(cx, _mm256_set1_ps(grid.width - 3.0f)),
                     _mm256_set1_ps(3.0f));
  cy = _mm256_max_ps(_mm256_min_ps(cy, _mm256_set1_ps(grid.height - 3.0f)),
                     _mm256_set1_ps(3.0f));

  __m256i icx = _mm256_cvttps_epi32(cx);
  __m256i icy = _mm256_cvttps_epi32(cy);

  __m256 stx = _mm256_sub_ps(cx, _mm256_round_ps(cx, _MM_FROUND_TRUNC));
  __m256 sty = _mm256_sub_ps(cy, _mm256_round_ps(cy, _MM_FROUND_TRUNC));

  __m256 v11 = gridGather(grid, _mm256_add_epi32(icx, _mm256_set1_epi32(-1)),
                          _mm256_add_epi32(icy, _mm256_set1_epi32(-1)));
  __m256 v12 = gridGather(grid, _mm256_add_epi32(icx, _mm256_set1_epi32(0)),
                          _mm256_add_epi32(icy, _mm256_set1_epi32(-1)));
  __m256 v13 = gridGather(grid, _mm256_add_epi32(icx, _mm256_set1_epi32(1)),
                          _mm256_add_epi32(icy, _mm256_set1_epi32(-1)));
  __m256 v14 = gridGather(grid, _mm256_add_epi32(icx, _mm256_set1_epi32(2)),
                          _mm256_add_epi32(icy, _mm256_set1_epi32(-1)));

  __m256 v21 = gridGather(grid, _mm256_add_epi32(icx, _mm256_set1_epi32(-1)),
                          _mm256_add_epi32(icy, _mm256_set1_epi32(0)));
  __m256 v22 = gridGather(grid, _mm256_add_epi32(icx, _mm256_set1_epi32(0)),
                          _mm256_add_epi32(icy, _mm256_set1_epi32(0)));
  __m256 v23 = gridGather(grid, _mm256_add_epi32(icx, _mm256_set1_epi32(1)),
                          _mm256_add_epi32(icy, _mm256_set1_epi32(0)));
  __m256 v24 = gridGather(grid, _mm256_add_epi32(icx, _mm256_set1_epi32(2)),
                          _mm256_add_epi32(icy, _mm256_set1_epi32(0)));

  __m256 v31 = gridGather(grid, _mm256_add_epi32(icx, _mm256_set1_epi32(-1)),
                          _mm256_add_epi32(icy, _mm256_set1_epi32(1)));
  __m256 v32 = gridGather(grid, _mm256_add_epi32(icx, _mm256_set1_epi32(0)),
                          _mm256_add_epi32(icy, _mm256_set1_epi32(1)));
  __m256 v33 = gridGather(grid, _mm256_add_epi32(icx, _mm256_set1_epi32(1)),
                          _mm256_add_epi32(icy, _mm256_set1_epi32(1)));
  __m256 v34 = gridGather(grid, _mm256_add_epi32(icx, _mm256_set1_epi32(2)),
                          _mm256_add_epi32(icy, _mm256_set1_epi32(1)));

  __m256 v41 = gridGather(grid, _mm256_add_epi32(icx, _mm256_set1_epi32(-1)),
                          _mm256_add_epi32(icy, _mm256_set1_epi32(2)));
  __m256 v42 = gridGather(grid, _mm256_add_epi32(icx, _mm256_set1_epi32(0)),
                          _mm256_add_epi32(icy, _mm256_set1_epi32(2)));
  __m256 v43 = gridGather(grid, _mm256_add_epi32(icx, _mm256_set1_epi32(1)),
                          _mm256_add_epi32(icy, _mm256_set1_epi32(2)));
  __m256 v44 = gridGather(grid, _mm256_add_epi32(icx, _mm256_set1_epi32(2)),
                          _mm256_add_epi32(icy, _mm256_set1_epi32(2)));

  __m256 v1 = CubicHermite(stx, v11, v12, v13, v14);
  __m256 v2 = CubicHermite(stx, v21, v22, v23, v24);
  __m256 v3 = CubicHermite(stx, v31, v32, v33, v34);
  __m256 v4 = CubicHermite(stx, v41, v42, v43, v44);

  return CubicHermite(sty, v1, v2, v3, v4);
}

class DoubleBuffered2DGrid {
public:
  DoubleBuffered2DGrid(int width, int height)
      : width(width), height(height), front(0),
        back(1), v{std::vector<float>(width * height),
                   std::vector<float>(width * height)} {}

  DoubleBuffered2DGrid() : width(0), height(0){};

  int idx(int x, int y) const { return y * width + x; }
  void swap() { std::swap(front, back); }

  float &f(int x, int y) {
    assert(x < width && x >= 0 && y < height && y >= 0);
    return v[front][idx(x, y)];
  }
  float &b(int x, int y) {
    assert(x < width && x >= 0 && y < height && y >= 0);
    return v[back][idx(x, y)];
  }
  float &f(glm::ivec2 c) { return f(c.x, c.y); }
  float &b(glm::ivec2 c) { return b(c.x, c.y); }

  float f(int x, int y) const {
    assert(x < width && x >= 0 && y < height && y >= 0);
    return v[front][idx(x, y)];
  }
  float b(int x, int y) const {
    assert(x < width && x >= 0 && y < height && y >= 0);
    return v[back][idx(x, y)];
  }
  float f(glm::ivec2 c) const { return f(c.x, c.y); }
  float b(glm::ivec2 c) const { return b(c.x, c.y); }

  float &operator()(int x, int y) { return f(x, y); }
  float &operator()(glm::ivec2 c) { return f(c.x, c.y); }

  float operator()(int x, int y) const { return f(x, y); }
  float operator()(glm::ivec2 c) const { return f(c.x, c.y); }

  void copyFrontToBack() {
#pragma omp for
    for (int i = 0; i < width * height; i++) {
      v[back][i] = v[front][i];
    }
  }

  float *data() { return v[front].data(); }
  float const *data() const { return v[front].data(); }
  float *back_data() { return v[back].data(); }
  int width, height;

private:
  int front, back;
  std::vector<float> v[2];
};
