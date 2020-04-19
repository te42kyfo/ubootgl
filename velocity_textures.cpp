#include "velocity_textures.hpp"
#include "gl_error.hpp"
#include "load_shader.hpp"
#include <GL/glew.h>
#include <cmath>
#include <vector>

using namespace std;

namespace VelocityTextures {

GLuint tex_vx_staggered, tex_vy_staggered, tex_vxy, tex_mag, tex_flag;
float minVel, maxVel;

int nx, ny;
GLuint interp_shader;
GLuint uloc_nx, uloc_ny, uloc_vx, uloc_vy;

GLuint getMagTex() { return tex_mag; }
GLuint getVXYTex() { return tex_vxy; }
GLuint getFlagTex() { return tex_flag; }
void init(int _nx, int _ny) {
  nx = _nx;
  ny = _ny;

  // allocate velocity texture storage
  GL_CALL(glGenTextures(1, &tex_vx_staggered));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, tex_vx_staggered));
  GL_CALL(glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, nx - 1, ny));

  GL_CALL(glGenTextures(1, &tex_vy_staggered));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, tex_vy_staggered));
  GL_CALL(glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, nx, ny - 1));

  GL_CALL(glGenTextures(1, &tex_vxy));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, tex_vxy));
  GL_CALL(glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG32F, 2 * nx - 1, 2 * ny - 1));

  GL_CALL(glGenTextures(1, &tex_mag));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, tex_mag));
  GL_CALL(glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, 2 * nx - 1, 2 * ny - 1));

  interp_shader = loadComputeShader("./interp_shader.cs");

  uloc_nx = glGetUniformLocation(interp_shader, "nx");
  uloc_ny = glGetUniformLocation(interp_shader, "ny");
  uloc_vx = glGetUniformLocation(interp_shader, "tex_vx_staggered");
  uloc_vy = glGetUniformLocation(interp_shader, "tex_vy_staggered");

  // allocate flag texture storage
  GL_CALL(glGenTextures(1, &tex_flag));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, tex_flag));

  int mip_levels = min(4, (int)min(floor(log2(nx)), log2(ny)));
  GL_CALL(glTexStorage2D(GL_TEXTURE_2D, mip_levels, GL_R32F, nx, ny));
}

void updateFromStaggered(float *vx_staggered, float *vy_staggered) {

  GL_CALL(glBindTexture(GL_TEXTURE_2D, tex_vx_staggered));
  GL_CALL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, nx - 1, ny, GL_RED, GL_FLOAT,
                          vx_staggered));

  GL_CALL(glBindTexture(GL_TEXTURE_2D, tex_vy_staggered));
  GL_CALL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, nx, ny - 1, GL_RED, GL_FLOAT,
                          vy_staggered));

  GL_CALL(glUseProgram(interp_shader));

  GL_CALL(glUniform1ui(uloc_nx, nx));
  GL_CALL(glUniform1ui(uloc_ny, ny));
  GL_CALL(glUniform1i(uloc_vx, 0));
  GL_CALL(glUniform1i(uloc_vy, 1));

  GL_CALL(
      glBindImageTexture(0, tex_vxy, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F));
  GL_CALL(
      glBindImageTexture(1, tex_mag, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F));

  GL_CALL(glActiveTexture(GL_TEXTURE0));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, tex_vx_staggered));

  GL_CALL(glActiveTexture(GL_TEXTURE1));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, tex_vy_staggered));

  GL_CALL(glDispatchCompute((2 * nx - 1 - 1) / 32 + 1, (2 * ny - 1 - 1) / 8 + 1,
                            1));
}

void uploadFlag(float *flag) {

  // Upload Texture
  GL_CALL(
      glTextureSubImage2D(tex_flag, 0, 0, 0, nx, ny, GL_RED, GL_FLOAT, flag));
  GL_CALL(glGenerateTextureMipmap(tex_flag));
}

} // namespace VelocityTextures
