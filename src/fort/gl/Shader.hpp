#pragma once

#include <string>

#include <GL/glew.h>
// order matter here
#include <GL/gl.h>

#include <Eigen/Core>

namespace fort {
namespace gl {

GLuint
CompileProgram(const std::string &vertexCode, const std::string &fragmentCode);

#pragma once
#include <Eigen/Core>
#include <GL/glew.h>
#include <string>

namespace details {

// Primary template — intentionally undefined by default
template <typename T> struct UniformUploader {
	static void upload(GLint /*loc*/, const T & /*value*/) {
		static_assert(
		    sizeof(T) == 0,
		    "No UniformUploader specialization defined for this type."
		);
	}
};

template <> struct UniformUploader<Eigen::Vector3f> {
	static void upload(GLint loc, const Eigen::Vector3f &v) {
		glUniform3fv(loc, 1, v.data());
	}
};

template <> struct UniformUploader<Eigen::Vector4f> {
	static void upload(GLint loc, const Eigen::Vector4f &v) {
		glUniform4fv(loc, 1, v.data());
	}
};

template <> struct UniformUploader<Eigen::Matrix3f> {
	static void upload(GLint loc, const Eigen::Matrix3f &m) {
		glUniformMatrix3fv(loc, 1, GL_FALSE, m.data());
	}
};

template <> struct UniformUploader<Eigen::Matrix4f> {
	static void upload(GLint loc, const Eigen::Matrix4f &m) {
		glUniformMatrix4fv(loc, 1, GL_FALSE, m.data());
	}
};

template <> struct UniformUploader<Eigen::Vector3d> {
	static void upload(GLint loc, const Eigen::Vector3d &v) {
		glUniform3dv(loc, 1, v.data());
	}
};

template <> struct UniformUploader<Eigen::Vector4d> {
	static void upload(GLint loc, const Eigen::Vector4d &v) {
		glUniform4dv(loc, 1, v.data());
	}
};

template <> struct UniformUploader<Eigen::Matrix3d> {
	static void upload(GLint loc, const Eigen::Matrix3d &m) {
		glUniformMatrix3dv(loc, 1, GL_FALSE, m.data());
	}
};

template <> struct UniformUploader<Eigen::Matrix4d> {
	static void upload(GLint loc, const Eigen::Matrix4d &m) {
		glUniformMatrix4dv(loc, 1, GL_FALSE, m.data());
	}
};

template <> struct UniformUploader<float> {
	static void upload(GLint loc, float v) {
		glUniform1f(loc, v);
	}
};

template <> struct UniformUploader<double> {
	static void upload(GLint loc, double v) {
		glUniform1d(loc, v);
	}
};

} // namespace details

template <typename T, typename function = void>
void Upload(GLuint programID, const std::string &name, const T &uniform) {
	auto ID = glGetUniformLocation(programID, name.c_str());
	details::UniformUploader<T>::upload(ID, uniform);
}

} // namespace gl
} // namespace fort
