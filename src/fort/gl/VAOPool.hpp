#pragma once

#include <GL/glew.h>
// order matter
#include <GL/gl.h>

#include <functional>
#include <memory>

#include <concurrentqueue.h>

#include <fort/utils/Defer.hpp>
#include <slog++/Attribute.hpp>
#include <slog++/slog++.hpp>
#include <stdexcept>

namespace fort {
namespace gl {

namespace details {
template <typename T> struct GLTraits;

template <> struct GLTraits<float> {
	constexpr static GLenum GLType = GL_FLOAT;
};

template <> struct GLTraits<double> {
	constexpr static GLenum GLType = GL_DOUBLE;
};
}; // namespace details

class VAOPool;

class VertexArrayObject {
public:
	const GLuint VAO, VBO;

	VertexArrayObject(
	    const slog::Logger<1> &logger =
	        slog::With(slog::String("group", "FreeVAO"))
	);
	~VertexArrayObject();

	VertexArrayObject(const VertexArrayObject &other)            = delete;
	VertexArrayObject(VertexArrayObject &&other)                 = delete;
	VertexArrayObject &operator=(const VertexArrayObject &other) = delete;
	VertexArrayObject &operator=(VertexArrayObject &&other)      = delete;

	template <typename T, size_t... ColSizes>
	void BufferData(GLint type, const T *data, size_t len) {
		constexpr static size_t ColSize = (... + ColSizes);
		if ((len % ColSize) != 0) {
			throw std::invalid_argument(
			    "length of buffer doesn't align with total colsize (" +
			    std::to_string(ColSize) + ")"
			);
		}
		d_logger.DTrace("binding VAO");
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		FORT_CHARIS_defer {
			d_logger.DTrace("Unbinding VAO");
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		};
		d_logger.DTrace("buffering data");
		glBufferData(GL_ARRAY_BUFFER, sizeof(T) * len, data, type);
		enableVertexAttrib<T, ColSize>(0, 0, ColSizes...);
	}

private:
	template <typename T, size_t ColSize, typename... Cols>
	void
	enableVertexAttrib(size_t idx, size_t start, size_t size, Cols... rest) {
		d_logger.DTrace(
		    "enabling attribs",
		    slog::Int("idx", idx),
		    slog::Int("start", start),
		    slog::Int("size", size)
		);
		glEnableVertexAttribArray(idx);
		glVertexAttribPointer(
		    idx,
		    size,
		    details::GLTraits<T>::GLType,
		    GL_FALSE,
		    ColSize * sizeof(T),
		    (const void *)(sizeof(T) * start)
		);
		if constexpr (sizeof...(rest) > 0) {
			enableVertexAttrib<T, ColSize>(idx + 1, start + size, rest...);
		}
	}

	slog::Logger<3> d_logger;
};

class VAOPool : public std::enable_shared_from_this<VAOPool> {

public:
	VAOPool();

	using VertexArrayObjectPtr = std::
	    unique_ptr<VertexArrayObject, std::function<void(VertexArrayObject *)>>;

	VertexArrayObjectPtr Get();

private:
	moodycamel::ConcurrentQueue<std::unique_ptr<VertexArrayObject>> d_queue;
	slog::Logger<1>                                                 d_logger;
};

} // namespace gl
} // namespace fort
