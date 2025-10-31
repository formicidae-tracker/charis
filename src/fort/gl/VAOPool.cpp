#include "VAOPool.hpp"
#include "fort/utils/ObjectPool.hpp"
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
    : utils::
          ObjectPool<VertexArrayObject, std::function<VertexArrayObject *()>>(
              [this]() { return new VertexArrayObject{d_logger}; },
              utils::DefaultDeleter<VertexArrayObject>{}
          )
    , d_logger{slog::With(slog::String("group", "VAO/" + to_string(this)))} {
	d_logger.DDebug("created");
}

std::shared_ptr<VAOPool> VAOPool::Create() {
	return std::shared_ptr<VAOPool>{new VAOPool()};
}

} // namespace gl
} // namespace fort
