#pragma once

#include "apollo/gl/FontAtlas.hpp"
#include "apollo/gl/VAOPool.hpp"
#include <GL/glew.h>
// order does not matter
#include <GL/gl.h>

#include <Eigen/Core>

namespace fort {
namespace gl {

class CompiledText {
public:
	using Pool              = VAOPool<float, 6 * 20, 2, 2>;
	using VertexArrayObject = Pool::VertexArrayObject;

	struct RenderArgs {
		Eigen::Vector2i ViewportSize;
		Eigen::Vector2i Position = {0, 0};
		Eigen::Vector4f Color           = {1.0, 1.0, 1.0, 1.0};
		Eigen::Vector4f BackgroundColor = {0.0, 0.0, 0.0, 1.0};
		float           Size            = 24.0;
	};

	void Render(const RenderArgs &args) const;

	Eigen::Vector4f BoundingBox(float size) const;

	CompiledText();

private:
	friend class TextRenderer;

	struct TextFragment {
		GLuint                 Texture;
		size_t                 Elements;
		VertexArrayObject::Ptr VAO;
	};

	CompiledText(GLuint program);

	std::vector<TextFragment> d_fragments;
	GLuint                    d_program;
	float                     d_width, d_hMin, d_hMax;
};

class TextRenderer {
public:
	TextRenderer(const std::string &fontName, size_t fontSize, size_t pageSize);

	template <typename Str> CompiledText Compile(Str &&str) noexcept {
		auto characters = d_atlas.Get(std::forward<Str>(str));
		return compile(
		    characters,
		    d_logger.With(slog::String("text", std::forward<Str>(str)))
		);
	}

private:
	CompiledText compile(
	    const std::vector<CharTexture> &characters,
	    const slog::Logger<2>          &logger
	) const;

	FontAtlas       d_atlas;
	GLuint          d_program;
	slog::Logger<1> d_logger;
};

} // namespace gl
} // namespace fort
