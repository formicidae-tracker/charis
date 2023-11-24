#pragma once

#include <GL/glew.h>
// order matter
#include <GL/gl.h>

#include <functional>
#include <memory>

#include <concurrentqueue.h>

#include "Defer.hpp"

namespace fort {
namespace gl {

template <typename T> struct GLTraits;

template <> struct GLTraits<float> {
	constexpr static GLenum GLType = GL_FLOAT;
};

template <> struct GLTraits<double> {
	constexpr static GLenum GLType = GL_DOUBLE;
};

template <typename T, size_t Rows, size_t... ColSizes>
class VAOPool
    : public std::enable_shared_from_this<VAOPool<T, Rows, ColSizes...>> {

public:
	struct VertexArrayObject {
		GLuint VAO, VBO;

		using Ptr = std::unique_ptr<
		    VertexArrayObject,
		    std::function<void(VertexArrayObject *)>>;

		using SharedPtr = std::shared_ptr<VertexArrayObject>;
	};

	constexpr static size_t RowSize = Rows;
	constexpr static size_t ColSize = (... + ColSizes);
	constexpr static size_t Size    = RowSize * ColSize;

	typename VertexArrayObject::Ptr Get() {
		auto self = this->shared_from_this();
		auto res  = new VertexArrayObject{};

		if (d_queue.try_dequeue(*res) == false) {
			buildVAO(*res);
		}

		return {res, [self](VertexArrayObject *object) {
			        self->d_queue.enqueue(*object);
			        delete object;
		        }};
	}

private:
	static void buildVAO(VertexArrayObject &obj) {
		glGenVertexArrays(1, &obj.VAO);
		glGenBuffers(1, &obj.VBO);
		glBindVertexArray(obj.VAO);
		glBindBuffer(GL_ARRAY_BUFFER, obj.VBO);
		defer {
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		};

		glBufferData(
		    GL_ARRAY_BUFFER,
		    sizeof(T) * Size,
		    nullptr,
		    GL_DYNAMIC_DRAW
		);
		enableVertexAttrib(0, 0, ColSizes...);
	}

	template <typename... U>
	static void
	enableVertexAttrib(size_t idx, size_t start, size_t size, U... rest) {
		glEnableVertexAttribArray(idx);
		glVertexAttribPointer(
		    idx,
		    size,
		    GLTraits<T>::GLType,
		    GL_FALSE,
		    ColSize * sizeof(T),
		    (const void *)(sizeof(T) * start)
		);

		if constexpr (sizeof...(rest) > 0) {
			enableVertexAttrib(idx + 1, start + size, rest...);
		}
	}

	void announce(const VertexArrayObject &obj) {
		d_queue.enqueue(obj);
	}

	moodycamel::ConcurrentQueue<VertexArrayObject> d_queue;
};

} // namespace gl
} // namespace fort
