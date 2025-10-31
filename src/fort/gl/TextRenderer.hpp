#pragma once

#include "FontAtlas.hpp"
#include "VAOPool.hpp"

#include <Eigen/src/Core/Matrix.h>
#include <GL/glew.h>
// order does not matter
#include <GL/gl.h>

#include <Eigen/Core>

namespace fort {
namespace gl {

class CompiledText {
public:
	struct TextScreenPosition {
		Eigen::Vector2i ViewportSize;
		Eigen::Vector2f Position = {0, 0};
		float           Size     = 24.0;
	};

	CompiledText();

	void SetColor(const Eigen::Vector4f &color) const;
	void SetBackgroundColor(const Eigen::Vector4f &color) const;
	void
	Render(const TextScreenPosition &args, bool renderBackground = false) const;

	Eigen::Vector4f BoundingBox(const TextScreenPosition &args) const;

private:
	friend class TextRenderer;

	struct TextFragment {
		GLuint             Texture;
		size_t             Elements;
		VAOPool::ObjectPtr VAO;
	};

	CompiledText(
	    GLuint textProgram, GLuint backgroundProgram, slog::Logger<3> &&logger
	);

	void renderBackground(const Eigen::Matrix4f &proj) const;

	std::vector<TextFragment> d_fragments;
	GLuint                    d_textProgram, d_backgroundProgram;
	Eigen::Vector4f           d_boundingBox;
	slog::Logger<3>           d_logger;
};

class TextRenderer {
public:
	TextRenderer(
	    const std::string &fontName, size_t fontSize, size_t pageSize = 512
	);

	template <typename Str>
	CompiledText
	Compile(Str &&str, int border_pt = 1, bool vertical = false) noexcept {
		auto characters = d_atlas.Get(std::forward<Str>(str));
		return compile(
		    characters,
		    border_pt,
		    vertical,
		    d_logger.With(slog::String("text", std::forward<Str>(str)))
		);
	}

private:
	CompiledText compile(
	    const std::vector<CharTexture> &characters,
	    int                             border_pt,
	    bool                            vertical,
	    slog::Logger<3>               &&logger
	) const;

	FontAtlas       d_atlas;
	static GLuint   s_textProgram, s_backgroundProgram;
	slog::Logger<2> d_logger;
};

} // namespace gl
} // namespace fort
