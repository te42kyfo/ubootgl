#include "draw_streamlines.hpp"
#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include "gl_error.hpp"
#include "load_shader.hpp"

using namespace std;

namespace DrawStreamlines {

const int LINE_COUNT = 1000;
const int BORDER_PADDING = 3;
const float LINE_WIDTH = 1;

GLuint line_shader;
GLuint vao, vbo_vertices, vbo_alphas;
GLint line_shader_aspect_ratio_uloc, line_shader_origin_uloc;

struct vec2 {
  float x, y;
};

vector<vec2> start_points;

void init() {
  line_shader = loadShader("./scale_translate2D.vert", "./color.frag",
                           {{0, "in_Position"}, {1, "in_Alpha"}});
  line_shader_aspect_ratio_uloc =
      glGetUniformLocation(line_shader, "aspect_ratio");
  line_shader_origin_uloc = glGetUniformLocation(line_shader, "origin");
  GL_CALL(glGenVertexArrays(1, &vao));
};
struct vec4 {
  float x, y, z, w;
};
void drawLines(const vector<vec4>& vertices, const vector<float>& alphas,
               const vector<int>& indices, float ratio_x, float ratio_y,
               float x_offset, float y_offset) {
  GL_CALL(glEnable(GL_BLEND));
  GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
  GL_CALL(glBindVertexArray(vao));
  GL_CALL(glGenBuffers(1, &vbo_vertices));
  GL_CALL(glGenBuffers(1, &vbo_alphas));

  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec4),
                       vertices.data(), GL_DYNAMIC_DRAW));
  GL_CALL(glEnableVertexAttribArray(0));
  GL_CALL(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));

  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo_alphas));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER, alphas.size() * sizeof(GLfloat),
                       alphas.data(), GL_DYNAMIC_DRAW));
  GL_CALL(glEnableVertexAttribArray(1));
  GL_CALL(glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, 0));

  GL_CALL(glUseProgram(line_shader));

  GL_CALL(glUniform2f(line_shader_aspect_ratio_uloc, ratio_x, ratio_y));
  GL_CALL(glUniform2f(line_shader_origin_uloc, x_offset, y_offset));
  GL_CALL(glDrawElements(GL_LINES, indices.size(), GL_UNSIGNED_INT,
                         indices.data()));
  GL_CALL(glDeleteBuffers(1, &vbo_vertices));
  GL_CALL(glDeleteBuffers(1, &vbo_alphas));
}

float bilinearSample(float x, float y, float* v, int nx, int ny) {
  int ix = floor(x - 0.5f);
  int iy = floor(y - 0.5f);
  if (ix < 0 || ix >= nx - 1 || iy < 0 || iy >= ny - 1) return 0.0f;;
  float s = x - 0.5f - ix;
  float t = y - 0.5f - iy;
  float as = 1.0f - s;
  float at = 1.0f - t;
  int idx1 = iy * nx + ix;
  int idx2 = iy * nx + ix + 1;
  int idx3 = (iy + 1) * nx + ix;
  int idx4 = (iy + 1) * nx + ix + 1;

  return at * (as * v[idx1] + s * v[idx2]) + t * (as * v[idx3] + s * v[idx4]);
}

void traceStreamLine(vector<vec4>& vertices, vector<float>& alphas,
                     vector<int>& indices, float* vx, float* vy, int nx, int ny,
                     int i0, float dir, float step_length) {
  float x = vertices[i0].x;
  float y = vertices[i0].y;

  const int streamline_length = nx / step_length * 0.1f;

  int last_index = i0;
  for (int i = 0; i < streamline_length; i++) {
    int ix = floor(x - 0.5f);
    int iy = floor(y - 0.5f);

    if (ix < 0 || ix >= nx - 1 || iy < 0 || iy >= ny - 1) break;

    float vxi1 = bilinearSample(x, y, vx, nx, ny);
    float vyi1 = bilinearSample(x, y, vy, nx, ny);
    float l1 = sqrt(vxi1 * vxi1 + vyi1 * vyi1);
    float x1 = x + vxi1 * step_length / l1;
    float y1 = y + vyi1 * step_length / l1;
    float vxi2 = bilinearSample(x1, y1, vx, nx, ny);
    float vyi2 = bilinearSample(x1, y1, vy, nx, ny);
    float l2 = sqrt(vxi2 * vxi2 + vyi2 * vyi2);
    if (l1 + l2 < 1.0e-2) break;

    x += dir * 0.5 * (vxi1 / l1 + vxi2 / l1) * step_length;
    y += dir * 0.5 * (vyi1 / l1 + vyi2 / l2) * step_length;

    vertices.push_back({x, y, 0.0, 1.0});
    alphas.push_back(0.0);
    float strength = 1.0f - i / (streamline_length - 1.0f);
    alphas[last_index] =  strength * min(1.0f, (l1+l2) * 0.2f - 0.1f);
    // cout << l << "\n";
    indices.push_back(last_index);
    indices.push_back(vertices.size() - 1);
    last_index = vertices.size() - 1;
  }
}

void pegToOne(float& xOut, float& yOut, float xIn, float yIn) {
  xOut = xIn / max(xIn, yIn);
  yOut = yIn / max(xIn, yIn);
}
void draw(float* vx, float* vy, int nx, int ny, int screen_width,
          int screen_height, float scale) {
  float screen_ratio_x = 0;
  float screen_ratio_y = 0;
  pegToOne(screen_ratio_x, screen_ratio_y,
           (float)screen_height / screen_width * nx / ny, 1.0f);
  float ratio_x = 2.0 * screen_ratio_x / nx;
  float ratio_y = 2.0 * screen_ratio_y / ny;

  vector<vec4> vertices;
  vector<int> indices;
  vector<float> alphas;

  if (start_points.size() < LINE_COUNT) {
    random_device rd;
    default_random_engine eng(1);
    uniform_real_distribution<float> dis(0.0f, 1.0f);

    while (start_points.size() < LINE_COUNT) {
      start_points.push_back({dis(eng) * nx, dis(eng) * ny});
    }
  }

  for (unsigned int n = 0; n < LINE_COUNT; n++) {
    vertices.push_back({start_points[n].x, start_points[n].y, 0.0f, 1.0f});
    int start_index = vertices.size() - 1;
    alphas.push_back(0.0f);  // min(1.0f, l * 0.2f - 0.1f));
    traceStreamLine(vertices, alphas, indices, vx, vy, nx, ny, start_index, 1.0,
                    2.0f * nx / screen_width);
    traceStreamLine(vertices, alphas, indices, vx, vy, nx, ny, start_index,
                    -1.0, 2.0f * nx / screen_width);
  }

  cout << vertices.size() << " " << alphas.size() << "\n";
  drawLines(vertices, alphas, indices, ratio_x * scale, ratio_y * scale,
            -1.0f * screen_ratio_x * scale, -1.0f * screen_ratio_y * scale);
}

/*
void DrawStreamlinesImplementation::operator()(
vector<Vec2D<float>> seeds;

srand(23123);
for (size_t i = 0; i < LINE_COUNT; ++i) {
Vec2D<float> point((float)(1 + rand() % 1000) / 999.0,
                   (float)(1 + rand() % 1000) / 999.0);
seeds.push_back(point);
}

float depth = 0;
for (auto& seed : seeds) {
depth += 2.0 / LINE_COUNT;
drawStreamline(seed, 0.5, vector_field, scalar_field, depth);
drawStreamline(seed, -0.5, vector_field, scalar_field, depth);
}
}

// Calculates the nearest, positive intersection with the grid. Expects and
// returns coordinates in grid space.
Vec2D<float> step(Vec2D<float> origin, Vec2D<float> v) {
  // Determine nearest grid line
  float tx1 = (floor(origin.x - 1.0e-5) - origin.x) / v.x;
  float tx2 = (ceil(origin.x + 1.0e-5) - origin.x) / v.x;
  float ty1 = (floor(origin.y - 1.0e-5) - origin.y) / v.y;
  float ty2 = (ceil(origin.y + 1.0e-5) - origin.y) / v.y;

  float tx = (tx1 > tx2) ? tx1 : tx2;
  float ty = (ty1 > ty2) ? ty1 : ty2;

  float t = (tx < ty) ? tx : ty;

  return origin + v * t;
}

void addVertices(vector<float>& vertices, vector<float>& colors,
                 Vec2D<float> point, Vec2D<float> v1, QColor color,
                 float z = 0.0) {
  Vec2D<float> normal = v1;
  swap(normal.x, normal.y);
  normal.x *= -1;
  normal.normalize();

  Vec2D<float> p1 = point + normal * LINE_WIDTH;
  Vec2D<float> p2 = point - normal * LINE_WIDTH;

  for (int i = 0; i < 2; i++) {
    colors.push_back(color.redF());
    colors.push_back(color.greenF());
    colors.push_back(color.blueF());
    colors.push_back((color.greenF() + color.redF()) * 0.3f);
  }

  vertices.push_back(p1.x);
  vertices.push_back(p1.y);
  vertices.push_back(z);

  vertices.push_back(p2.x);
  vertices.push_back(p2.y);
  vertices.push_back(z);
}

void DrawStreamlinesImplementation::drawStreamline(
    Vec2D<float> point, float dir, const Grid<Vec2D<float>>& vector_field,
    const Grid<float>& scalar_field, float z) {
  Vec2D<float> gridpoint{point.x * (vector_field.x() - 1),
                         point.y * (vector_field.y() - 1)};

  vector<float> vertices;
  vector<float> colors;

  Vec2D<float> v1 = interpolate(vector_field, point);

  v1 *= dir;
  addVertices(vertices, colors, gridpoint, v1,
              getColorAtPoint(vector_field, scalar_field, point), z);

  Vec2D<float> last_point = gridpoint;

  bool early_exit = false;
  for (size_t i = 0; i < vector_field.x() * 3 && early_exit == false; i++) {
    Vec2D<float> v1 =
        interpolate(vector_field, {gridpoint.x / (vector_field.x() - 1),
                                   gridpoint.y / (vector_field.y() - 1)});
    v1 *= dir;

    // Calculate the predictor point and get the direction at that point.
    Vec2D<float> predictor = step(gridpoint, v1);
    Vec2D<float> v2 =
        interpolate(vector_field, {predictor.x / (vector_field.x() - 1),
                                   predictor.y / (vector_field.y() - 1)});

    v2 *= dir;

    v1 = (v1 + v2) / 2.0;
    if (v1.x * v1.x + v1.y * v1.y < 0.0000001) early_exit = true;

    gridpoint = step(gridpoint, v1);

    if (gridpoint.x < BORDER_PADDING) {
      gridpoint.x = BORDER_PADDING;
      early_exit = true;
    }
    if (gridpoint.x > vector_field.x() - BORDER_PADDING) {
      gridpoint.x = vector_field.x() - BORDER_PADDING;
      early_exit = true;
    }
    if (gridpoint.y < BORDER_PADDING ||
        gridpoint.y > vector_field.y() - BORDER_PADDING)
      early_exit = true;

    auto point = Vec2D<float>{gridpoint.x / (vector_field.x() - 1),
                              gridpoint.y / (vector_field.y() - 1)};

    v1 = interpolate(vector_field, point);
    v1 = v1.normalize();
    v1 *= dir;
    if (early_exit || (gridpoint - last_point).normalize() * v1 < 0.99999999) {
      addVertices(vertices, colors, gridpoint, v1,
                  getColorAtPoint(vector_field, scalar_field, point), z);
      last_point = gridpoint;
    }
  }

  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_VERTEX_ARRAY);

  glVertexPointer(3, GL_FLOAT, 0, (float*)vertices.data());
  glColorPointer(4, GL_FLOAT, 0, (float*)colors.data());

  glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices.size() / 3);

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
}
*/
}
