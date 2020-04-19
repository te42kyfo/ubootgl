#ifndef __VELOCITY_TEXTURES_H_
#define __VELOCITY_TEXTURES_H_

#include <GL/glew.h>

namespace VelocityTextures {

GLuint getMagTex();
GLuint getVXYTex();
GLuint getFlagTex();
void init(int nx, int ny);
void updateFromStaggered(float *vx_staggered, float *vy_staggered);
void uploadFlag(float *flag);
} // namespace VelocityTextures

#endif // __VELOCITY_TEXTURES_H_
