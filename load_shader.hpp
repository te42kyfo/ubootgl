#pragma once

#include <GL/glew.h>
#include <string>
#include <vector>

GLuint loadShader(std::string vshader, std::string fshader,
                  std::vector<std::pair<GLuint, std::string>> attribs);
