#pragma once

#include <GL/glew.h>
#include <iostream>
#include <string>
#include "gl_error.hpp"
#include "lodepng.h"

class Texture {
 public:
  Texture(std::string filename) {
    unsigned error = lodepng::decode(image, width, height, filename);
    if (error)
      std::cout << "decoder error " << error << " loading texture filename "
                << filename << ": " << lodepng_error_text(error) << std::endl;

    // Upload Texture
    GL_CALL(glGenTextures(1, &tex_id));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, tex_id));

    GL_CALL(glTexStorage2D(GL_TEXTURE_2D, 4, GL_SRGB8_ALPHA8, width, height));
    //   GL_CALL(glTexStorage2D(GL_TEXTURE_2D, 4, GL_RGBA8, width, height));
    GL_CALL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA,
                            GL_UNSIGNED_BYTE, image.data()));

    GL_CALL(glGenerateMipmap(GL_TEXTURE_2D));

  }

  GLuint tex_id;
  std::vector<unsigned char> image;
  unsigned width, height;
};
