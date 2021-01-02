#pragma once

#include <GL/glew.h>
#include <map>
#include <string>
#include <vector>

class Shader {
public:
  Shader() : id(0){};
  Shader(GLuint id) : id(id){};

  GLuint uloc(std::string name) {
    if (ulocs.count(name))
      return ulocs[name];
    ulocs[name] = glGetUniformLocation(id, name.c_str());
    return ulocs[name];
  }

  GLuint id;
  std::map<std::string, GLuint> ulocs;
};

GLuint loadShader(std::string vshader, std::string fshader,
                  std::vector<std::pair<GLuint, std::string>> attribs);

GLuint loadComputeShader(std::string cshader);
