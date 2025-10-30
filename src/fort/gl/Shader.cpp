#include "Shader.hpp"
#include <fort/utils/Defer.hpp>

#ifdef FORT_CHARIS_CAPITALIZE_DEFER
#define defer Defer
#endif

#include <stdexcept>
#include <vector>

namespace fort {
namespace gl {

#define get_compilation_error(type, sType, psID)                               \
	do {                                                                       \
		GLint result;                                                          \
		int   infoLength;                                                      \
		glGet##type##iv(psID, sType, &result);                                 \
		glGet##type##iv(psID, GL_INFO_LOG_LENGTH, &infoLength);                \
		if (result == GL_FALSE) {                                              \
			std::string message = "No Info";                                   \
			if (infoLength > 0) {                                              \
				std::vector<char> message_array(infoLength + 1);               \
				glGet##type##InfoLog(psID, infoLength, NULL, &message[0]);     \
				message =                                                      \
				    std::string(message_array.begin(), message_array.end());   \
			}                                                                  \
			throw std::runtime_error(                                          \
			    "Could not compile " #type ": " + message                      \
			);                                                                 \
		}                                                                      \
	} while (0)

#define get_shader_compilation_error(psID)                                     \
	get_compilation_error(Shader, GL_COMPILE_STATUS, psID)
#define get_program_compilation_error(psID)                                    \
	get_compilation_error(Program, GL_LINK_STATUS, psID)

GLuint CompileShader(const std::string &shaderCode, GLenum type) {
	GLuint shaderID = glCreateShader(type);

	const char *code = shaderCode.c_str();
	glShaderSource(shaderID, 1, &code, NULL);
	glCompileShader(shaderID);

	get_shader_compilation_error(shaderID);

	return shaderID;
}

GLuint CompileProgram(
    const std::string &vertexShaderCode, const std::string &fragmentShaderCode
) {

	// Create the shaders
	GLuint vertexShaderID = CompileShader(vertexShaderCode, GL_VERTEX_SHADER);
	defer {
		glDeleteShader(vertexShaderID);
	};

	GLuint fragmentShaderID =
	    CompileShader(fragmentShaderCode, GL_FRAGMENT_SHADER);
	defer {
		glDeleteShader(fragmentShaderID);
	};

	GLuint programID = glCreateProgram();

	glAttachShader(programID, vertexShaderID);
	glAttachShader(programID, fragmentShaderID);
	defer {
		glDetachShader(programID, vertexShaderID);
		glDetachShader(programID, fragmentShaderID);
	};

	glLinkProgram(programID);

	get_program_compilation_error(programID);

	return programID;
}

} // namespace gl
} // namespace fort
