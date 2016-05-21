#pragma once

#include <string>
#include <vector>
#include <GL/glew.h>

GLuint loadShader( std::string vshader, std::string fshader,
                   std::vector<std::pair<GLuint, std::string>> attribs);
