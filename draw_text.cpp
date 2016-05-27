
#include "draw_text.hpp"

#include <cstdint>
#include <iostream>
#include <map>
#include "SDL2/SDL_ttf.h"
#include "gl_error.hpp"
#include "load_shader.hpp"
using namespace std;

namespace DrawText {

TTF_Font* font = NULL;

GLuint color_tex_shader;
GLuint vao, vbo, tbo, tex_id;
GLint color_tex_shader_tex_uloc, color_tex_shader_aspect_ratio_uloc,
    color_tex_shader_origin_uloc;
int const font_size = 14;

map<int, TTF_Font*> fonts;

const string font_name = "DroidSans.ttf";

void init() {
  TTF_Init();

  color_tex_shader = loadShader("./scale_translate2D.vert", "./color_tex.frag",
                                {{0, "in_Position"}});
  color_tex_shader_tex_uloc = glGetUniformLocation(color_tex_shader, "tex");
  color_tex_shader_origin_uloc =
      glGetUniformLocation(color_tex_shader, "origin");
  color_tex_shader_aspect_ratio_uloc =
      glGetUniformLocation(color_tex_shader, "aspect_ratio");

  GL_CALL(glGenVertexArrays(1, &vao));
  GL_CALL(glGenBuffers(1, &vbo));

  float vertex_data[4][4] = {{-1.0f, -1.0f, 0.0f, 1.0f},
                             {1.0f, -1.0f, 0.0f, 1.0f},
                             {-1.0f, 1.0f, 0.0f, 1.0f},
                             {1.0f, 1.0f, 0.0f, 1.0f}};

  GL_CALL(glBindVertexArray(vao));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
  GL_CALL(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER, 4 * 4 * sizeof(GLfloat), vertex_data,
                       GL_STATIC_DRAW));
}

void draw(std::string text, float x_pos, float y_pos, float size,
          int screen_width, int screen_height) {
  int font_height = size * screen_height * 0.856;
  if (fonts.count(font_height) == 0) {
    fonts[font_height] = TTF_OpenFont(font_name.c_str(), font_height);
    // cout << "Create Font: " << font_height << "\n";
    if (!fonts[font_height]) {
      cout << "TTF_OpenFont: " << TTF_GetError() << " ";
    }
  }

  GL_CALL(glEnable(GL_BLEND));
  GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

  SDL_Color text_color = {255, 255, 50};
  SDL_Surface* sFont =
      TTF_RenderText_Blended(fonts[font_height], text.c_str(), text_color);
  if (!sFont) {
    cout << "TTF_RenderText_Blended: " << TTF_GetError() << "\n";
  }

  float ratio_y = (float)-sFont->h / screen_height;
  float ratio_x =
      (float)sFont->w / sFont->h * -ratio_y * screen_height / screen_width;

  // Upload generated buffer as texture
  GL_CALL(glGenTextures(1, &tex_id));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, tex_id));
  GL_CALL(glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, sFont->w, sFont->h));
  GL_CALL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sFont->w, sFont->h, GL_BGRA,
                          GL_UNSIGNED_BYTE, sFont->pixels));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

  // Draw Quad with texture
  GL_CALL(glBindVertexArray(vao));
  GL_CALL(glEnableVertexAttribArray(0));
  GL_CALL(glActiveTexture(GL_TEXTURE0));
  GL_CALL(glUseProgram(color_tex_shader));
  GL_CALL(glUniform1i(color_tex_shader_tex_uloc, 0));
  GL_CALL(glUniform2f(color_tex_shader_aspect_ratio_uloc, ratio_x, ratio_y));
  GL_CALL(glUniform2f(color_tex_shader_origin_uloc, x_pos + ratio_x,
                      y_pos - ratio_y * 0.5));
  GL_CALL(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
  GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

  GL_CALL(glDeleteTextures(1, &tex_id));
  SDL_FreeSurface(sFont);
}
}
