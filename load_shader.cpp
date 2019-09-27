#include "load_shader.hpp"
#include <fstream>
#include <iostream>

using namespace std;

string readShaderFile(const string fileName) {
  try {
    std::ifstream t(fileName.c_str());
    return std::string((std::istreambuf_iterator<char>(t)),
                       std::istreambuf_iterator<char>());
  } catch (std::exception& e) {
    std::cout << fileName << " - "
              << "readShaderFile: " << e.what() << "\n";
    return string();
  }
}

GLuint loadShader(string vshader, string fshader,
                  vector<pair<GLuint, string>> attribs) {
  GLcharARB log[5000];
  GLsizei length;

  string vertex_shader = readShaderFile(vshader);
  string fragment_shader = readShaderFile(fshader);
  char const* my_fragment_shader_source = fragment_shader.c_str();
  char const* my_vertex_shader_source = vertex_shader.c_str();

  GLuint program = glCreateProgram();
  GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
  GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);

  glShaderSource(vertex, 1, &my_vertex_shader_source, NULL);
  glShaderSource(fragment, 1, &my_fragment_shader_source, NULL);
  glCompileShader(vertex);
  glCompileShader(fragment);
  glGetShaderInfoLog(vertex, 5000, &length, log);
  if (length > 0) std::cout << "Vertex Shader: " << log << "\n";
  glGetShaderInfoLog(fragment, 5000, &length, log);
  if (length > 0) std::cout << "Fragment Shader: " << log << "\n";

  glAttachShader(program, vertex);
  glAttachShader(program, fragment);

  for (auto attrib : attribs) {
    glBindAttribLocation(program, attrib.first, attrib.second.c_str());
  }
  glLinkProgram(program);

  glGetProgramInfoLog(program, 5000, &length, log);
  if (length > 0) std::cout << "Program: " << log << "\n";

  return program;
}
