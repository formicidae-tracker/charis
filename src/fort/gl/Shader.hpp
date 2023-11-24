#pragma once

#include <string>

#include <GL/glew.h>
// order matter here
#include <GL/gl.h>

namespace fort {
namespace gl {

GLuint
CompileProgram(const std::string &vertexCode, const std::string &fragmentCode);

} // namespace gl
} // namespace fort
