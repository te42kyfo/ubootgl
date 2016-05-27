#pragma once

#include <GL/glew.h>

#define GL_CALL(CALL) \
  CALL;               \
  printOglError(#CALL, __FILE__, __LINE__)

int static printOglError(const char* callString, const char* file, int line) {
  GLenum glErr;
  int retCode = 0;

  glErr = glGetError();
  if (glErr != GL_NO_ERROR) {
    std::cerr << "GL_CALL: \"" << callString << "\"(" << file << ":" << line
              << ") produced an error: " << glErr << "\n";
    retCode = 1;
  }
  return retCode;
}
