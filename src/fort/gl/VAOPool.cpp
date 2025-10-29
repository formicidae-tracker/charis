#include "VAOPool.hpp"
#include <memory>
#include <sstream>

namespace fort {
namespace gl {
static inline GLuint buildVAO() {
	GLuint res;
	glGenVertexArrays(1, &res);
	return res;
}

static inline GLuint buildVBO() {
	GLuint res;
	glGenBuffers(1, &res);
	return res;
}

VertexArrayObject::VertexArrayObject(const slog::Logger<1> &logger)
    : VAO{buildVAO()}
    , VBO{buildVBO()}
    , d_logger{logger.With(slog::Int("VAO", VAO), slog::Int("VBO", VBO))} {
	d_logger.DDebug("created");
}

VertexArrayObject::~VertexArrayObject() {
	d_logger.DDebug("deleting");
	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);
}

std::string to_string(VAOPool *ptr) {
	std::ostringstream oss;
	oss << ptr;
	return oss.str();
}

VAOPool::VAOPool()
    : d_logger{slog::With(slog::String("group", "VAO/" + to_string(this)))} {
	d_logger.DDebug("created");
}

VAOPool::VertexArrayObjectPtr VAOPool::Get() {
	std::unique_ptr<VertexArrayObject> res;
	if (d_queue.try_dequeue(res) == false) {
		res = std::make_unique<VertexArrayObject>(d_logger);
	}

	return {
	    res.release(),
	    [self = this->shared_from_this()](VertexArrayObject *vao) {
		    self->d_queue.enqueue(std::unique_ptr<VertexArrayObject>(vao));
	    }
	};
}

} // namespace gl
} // namespace fort
