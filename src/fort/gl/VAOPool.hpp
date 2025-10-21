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

template <typename T> struct GLTraits;

template <> struct GLTraits<float> {
	constexpr static GLenum GLType = GL_FLOAT;
};

template <> struct GLTraits<double> {
	constexpr static GLenum GLType = GL_DOUBLE;
};

template <typename T, size_t... ColSizes>
class VAOPool : public std::enable_shared_from_this<VAOPool<T, ColSizes...>> {

public:
	constexpr static size_t ColSize = (... + ColSizes);

	template <size_t N>
	static inline slog::Logger<N + 1> buildLogger(const slog::Logger<N> &logger
	) {
		int i = 0;
		return logger.With(slog::Group(
		    "cols",
		    slog::Int("col[" + std::to_string(i++) + "]", ColSizes)...
		));
	}

	VAOPool()
	    : d_logger{buildLogger(slog::DefaultLogger())} {}

	struct VertexArrayObject {

		GLuint          VAO, VBO;
		slog::Logger<3> Logger;

		using Ptr = std::unique_ptr<
		    VertexArrayObject,
		    std::function<void(VertexArrayObject *)>>;

		using SharedPtr = std::shared_ptr<VertexArrayObject>;

		void BindBuffer(int type, const void *data, size_t len) {
			if (len % ColSize != 0) {
				throw std::invalid_argument(
				    "length of buffer doesn't align with total colsize (" +
				    std::to_string(ColSize) + ")"
				);
			}
			Logger.Debug("binding VAO");
			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			defer {
				Logger.Debug("unbinding VAO");
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindVertexArray(0);
			};
			Logger.Debug("buffering VAO");
			glBufferData(GL_ARRAY_BUFFER, sizeof(T) * len, data, type);
			enableVertexAttrib(0, 0, ColSizes...);
		}

	private:
		template <typename... U>
		void
		enableVertexAttrib(size_t idx, size_t start, size_t size, U... rest) {
			Logger.Debug(
			    "enabling attribs for VAO",
			    slog::Int("start", start),
			    slog::Int("size", size)
			);
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
	};

	typename VertexArrayObject::Ptr Get() {
		auto self = this->shared_from_this();
		auto res  = new VertexArrayObject{VertexArrayObject{
		     .VAO    = 0,
		     .VBO    = 0,
		     .Logger = d_logger.With(slog::Int("VAO", 0), slog::Int("VBO", 0))
        }};

		if (d_queue.try_dequeue(*res) == false) {
			buildVAO(*res, d_logger);
		}

		return {res, [self](VertexArrayObject *object) {
			        self->d_queue.enqueue(*object);
			        delete object;
		        }};
	}

	private:
	    static void
	    buildVAO(VertexArrayObject &obj, const slog::Logger<1> &logger) {
		    glGenVertexArrays(1, &obj.VAO);
		    glGenBuffers(1, &obj.VBO);
		    obj.Logger = logger.With(
		        slog::Int("VAO", obj.VAO),
		        slog::Int("VBO", obj.VBO)
		    );

		    obj.Logger.Debug("created VAO");
	    }

	    void announce(const VertexArrayObject &obj) {
			d_queue.enqueue(obj);
		}

		moodycamel::ConcurrentQueue<VertexArrayObject> d_queue;
	    slog::Logger<1>                                d_logger;
	};

} // namespace gl
} // namespace fort
