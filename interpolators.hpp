#ifndef __INTERPOLATORS_H_
#define __INTERPOLATORS_H_


#include "db2dgrid.hpp"
#include <glm/glm.hpp>
#include <immintrin.h>

#include "immintrin.h"

template <typename GridType>
inline float bilinearSample(const GridType &grid, glm::vec2 c) {

  c = glm::clamp(c, glm::vec2(0.0f, 0.0f),
                 glm::vec2(grid.width - 1.1f, grid.height - 1.1f));

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
inline void bilinearScatter(GridType &grid, glm::vec2 c, float v) {
  c = glm::clamp(c, glm::vec2(0.0f, 0.0f),
                 glm::vec2(grid.width - 1.1f, grid.height - 1.1f));
  glm::ivec2 ic = c;
  glm::vec2 st = fract(c);
  glm::vec2 ast = glm::vec2(1.0f, 1.0f) - st;
  grid(ic + glm::ivec2{1, 1}) += st.x * st.y * v;
  grid(ic + glm::ivec2{0, 1}) += ast.x * st.y * v;
  grid(ic + glm::ivec2{1, 0}) += st.x * ast.y * v;
  grid(ic + glm::ivec2{0, 0}) += ast.x * ast.y * v;
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
__attribute__((always_inline)) inline __m256 bicubicSample(const GridType &grid, __m256 cx, __m256 cy) {
  cx = _mm256_max_ps(_mm256_min_ps(cx, _mm256_set1_ps(grid.width - 3.0f)),
                     _mm256_set1_ps(3.0f));
  cy = _mm256_max_ps(_mm256_min_ps(cy, _mm256_set1_ps(grid.height - 3.0f)),
                     _mm256_set1_ps(3.0f));

  __m256i icx = _mm256_cvttps_epi32(cx);
  __m256i icy = _mm256_cvttps_epi32(cy);

  __m256 stx = _mm256_sub_ps(cx, _mm256_round_ps(cx, _MM_FROUND_TRUNC));
  __m256 sty = _mm256_sub_ps(cy, _mm256_round_ps(cy, _MM_FROUND_TRUNC));


  __m256i idx1 =
      _mm256_add_epi32(_mm256_sub_epi32(icx, _mm256_set1_epi32(1)), _mm256_mullo_epi32(_mm256_sub_epi32(icy, _mm256_set1_epi32(1)), _mm256_set1_epi32(grid.width)));

  __m256i idx2 = _mm256_add_epi32(idx1, _mm256_set1_epi32(grid.width));
  __m256i idx3 = _mm256_add_epi32(idx2, _mm256_set1_epi32(grid.width));
  __m256i idx4 = _mm256_add_epi32(idx3, _mm256_set1_epi32(grid.width));

  // Load four rows and compute vertical interpoilation of two SIMD lanes
  __m128 l00 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx1, 0));
  __m128 l01 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx2, 0));
  __m128 l02 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx3, 0));
  __m128 l03 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx4, 0));

  __m128 l10 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx1, 4));
  __m128 l11 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx2, 4));
  __m128 l12 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx3, 4));
  __m128 l13 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx4, 4));



   __m256 L00 = _mm256_insertf128_ps(_mm256_castps128_ps256(l00), l10, 1);
   __m256 L01 = _mm256_insertf128_ps(_mm256_castps128_ps256(l01), l11, 1);
   __m256 L02 = _mm256_insertf128_ps(_mm256_castps128_ps256(l02), l12, 1);
   __m256 L03 = _mm256_insertf128_ps(_mm256_castps128_ps256(l03), l13, 1);


  __m256 C0 = CubicHermite( _mm256_permute_ps(sty, 0b00000000), L00, L01, L02, L03);


  // Load four rows and compute vertical interpolation of two SIMD lanes
   l00 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx1, 1));
   l01 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx2, 1));
   l02 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx3, 1));
   l03 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx4, 1));

   l10 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx1, 5));
   l11 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx2, 5));
   l12 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx3, 5));
   l13 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx4, 5));


   L00 = _mm256_insertf128_ps(_mm256_castps128_ps256(l00), l10, 1);
   L01 = _mm256_insertf128_ps(_mm256_castps128_ps256(l01), l11, 1);
   L02 = _mm256_insertf128_ps(_mm256_castps128_ps256(l02), l12, 1);
   L03 = _mm256_insertf128_ps(_mm256_castps128_ps256(l03), l13, 1);

  __m256 C1 = CubicHermite( _mm256_permute_ps(sty, 0b01010101), L00, L01, L02, L03);

   __m256 t1 = _mm256_unpacklo_ps(C0, C1);
   __m256 t3 = _mm256_unpackhi_ps(C0, C1);

  // Load four rows and compute vertical interpoilation of two SIMD lanes
   l00 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx1, 2));
   l01 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx2, 2));
   l02 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx3, 2));
   l03 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx4, 2));

   l10 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx1, 6));
   l11 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx2, 6));
   l12 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx3, 6));
   l13 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx4, 6));


   L00 = _mm256_insertf128_ps(_mm256_castps128_ps256(l00), l10, 1);
   L01 = _mm256_insertf128_ps(_mm256_castps128_ps256(l01), l11, 1);
   L02 = _mm256_insertf128_ps(_mm256_castps128_ps256(l02), l12, 1);
   L03 = _mm256_insertf128_ps(_mm256_castps128_ps256(l03), l13, 1);

  __m256 C2 = CubicHermite( _mm256_permute_ps(sty, 0b10101010), L00, L01, L02, L03);


  // Load four rows and compute vertical interpoilation of two SIMD lanes
   l00 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx1, 3));
   l01 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx2, 3));
   l02 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx3, 3));
   l03 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx4, 3));

   l10 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx1, 7));
   l11 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx2, 7));
   l12 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx3, 7));
   l13 = _mm_loadu_ps( grid.data() + _mm256_extract_epi32(idx4, 7));

   L00 = _mm256_insertf128_ps(_mm256_castps128_ps256(l00), l10, 1);
   L01 = _mm256_insertf128_ps(_mm256_castps128_ps256(l01), l11, 1);
   L02 = _mm256_insertf128_ps(_mm256_castps128_ps256(l02), l12, 1);
   L03 = _mm256_insertf128_ps(_mm256_castps128_ps256(l03), l13, 1);

   __m256 C3 = CubicHermite( _mm256_permute_ps(sty, 0b11111111), L00, L01, L02, L03);

   __m256 t2 = _mm256_unpacklo_ps(C2, C3);
   __m256 v0 = _mm256_shuffle_ps(t1, t2, 0b01000100);
   __m256 v1 = _mm256_shuffle_ps(t1, t2, 0b11101110);


   __m256 t4 = _mm256_unpackhi_ps(C2, C3);
   __m256 v2 = _mm256_shuffle_ps(t3, t4, 0b01000100);
   __m256 v3 = _mm256_shuffle_ps(t3, t4, 0b11101110);

  return CubicHermite(stx, v0, v1, v2, v3);

}

#endif // __INTERPOLATORS_H_
