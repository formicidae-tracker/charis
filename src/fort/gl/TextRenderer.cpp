#include <limits>
#include <map>
#include <memory>

#include "FontAtlas.hpp"
#include "Shader.hpp"
#include "TextRenderer.hpp"
#include <fort/utils/Defer.hpp>
#include <slog++/slog++.hpp>
#include <sstream>

#include "fort-gl_rc.h"

namespace fort {
namespace gl {

CompiledText::CompiledText()
    : d_program{0}
    , d_logger{slog::With(
          slog::String("group", "CompiledText"),
          slog::String("font", ""),
          slog::String("text", "")
      )} {}

CompiledText::CompiledText(GLuint program, slog::Logger<3> &&logger)
    : d_program(program)
    , d_logger{std::move(logger)} {}

Eigen::Matrix4f buildProjectionMatrix(const CompiledText::RenderArgs &args) {
	Eigen::Matrix4f res;

	// for each vertices, we need to:
	// 1. multiply per desired fontSize
	// 2. add position
	// 3. transform to generalized coordinates.
	float vx = 2.0 / args.ViewportSize.x();
	float vy = 2.0 / args.ViewportSize.y();
	// clang-format off
	res << vx * args.Size, 0.0, 0.0, vx * args.Position.x() - 1.0, // row 0
	    0.0, vy * args.Size, 0.0, -vy * args.Position.y() + 1.0, // row 1
	    0.0, 0.0, 1.0, 0.0,                                    // row 2
	    0.0, 0.0, 0.0, 1.0;                                    // row 3
	// clang-format on
	return res;
}

const Eigen::Vector2f &CompiledText::RenderSize() const {
	return d_renderSize;
}

void CompiledText::Render(const RenderArgs &args) const {
	if (d_fragments.empty()) {
		return;
	}

	glUseProgram(d_program);
	auto projection = glGetUniformLocation(d_program, "projection");
	auto color      = glGetUniformLocation(d_program, "textColor");

	auto projectionMatrix = buildProjectionMatrix(args);
	glUniformMatrix4fv(projection, 1, GL_FALSE, projectionMatrix.data());
	glUniform4fv(color, 1, args.Color.data());

	defer {
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
		glUseProgram(0);
	};
	glActiveTexture(GL_TEXTURE0);

	for (const auto &f : d_fragments) {
		d_logger.DTrace(
		    "rendering fragment",
		    slog::Int("VAO", f.VAO->VAO),
		    slog::Int("elements", f.Elements),
		    slog::Int("total", d_fragments.size())
		);

		glBindTexture(GL_TEXTURE_2D, f.Texture);
		glBindVertexArray(f.VAO->VAO);

		glDrawArrays(GL_TRIANGLES, 0, f.Elements);
	}
}

TextRenderer::TextRenderer(
    const std::string &fontName, size_t fontSize, size_t pageSize
)
    : d_atlas{fontName, fontSize, pageSize}
    , d_logger{slog::With(
          slog::String("group", "TextRenderer"),
          slog::Group(
              "font",
              slog::String("name", fontName.c_str()),
              slog::Int("size", fontSize),
              slog::Int("pageSize", pageSize)
          )
      )} {
	d_atlas.LoadASCII();
	d_program = CompileProgram(
	    std::string{
	        shaders_text_vertex_start,
	        shaders_text_vertex_end,
	    },
	    std::string{
	        shaders_text_fragment_start,
	        shaders_text_fragment_end,
	    }
	);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

std::string to_string(const Eigen::Vector2f &v) {
	std::ostringstream oss;
	oss << v.transpose();
	return oss.str();
}

CompiledText TextRenderer::compile(
    const std::vector<CharTexture> &characters,
    bool                            vertical,
    slog::Logger<3>               &&logger
) const {
	static auto pool = std::make_shared<VAOPool>();
	logger.DTrace("compiling", slog::Int("size", characters.size()));
	CompiledText res{d_program, std::move(logger)};

	using VertexData = std::vector<float>;

	std::map<GLuint, VertexData> dataPerTexture;

	res.d_width           = 0.0;
	res.d_height          = 0.0;
	Eigen::Vector4f BBbox = {
	    std::numeric_limits<float>::max(),
	    std::numeric_limits<float>::max(),
	    0.0,
	    0.0
	};

	for (const auto &ch : characters) {
		if (ch.TextureBottomRight == ch.TextureTopLeft) {
			if (vertical == false) {
				res.d_width += ch.AdvanceX;
			} else {
				res.d_height += ch.AdvanceY;
			}

			continue;
		}

		auto &data = dataPerTexture[ch.Texture];
		data.reserve(6 * 4 * characters.size());

		Eigen::Vector2f scTL =
		    ch.ScreenTopLeft + Eigen::Vector2f{res.d_width, res.d_height};
		Eigen::Vector2f scBR =
		    ch.ScreenBottomRight + Eigen::Vector2f{res.d_width, res.d_height};

		const Eigen::Vector2f &txTL = ch.TextureTopLeft;
		const Eigen::Vector2f &txBR = ch.TextureBottomRight;

		BBbox.x() = std::min(BBbox.x(), scTL.x());
		BBbox.y() = std::min(BBbox.y(), scTL.y());
		BBbox.z() = std::max(BBbox.z(), scBR.x());
		BBbox.w() = std::max(BBbox.w(), scBR.y());

		data.insert(
		    data.end(),
		    {
		        scTL.x(), scTL.y(), txTL.x(), txTL.y(), // topLeft
		        scBR.x(), scTL.y(), txBR.x(), txTL.y(), // topRight
		        scTL.x(), scBR.y(), txTL.x(), txBR.y(), // bottomLeft
		        scBR.x(), scTL.y(), txBR.x(), txTL.y(), // topRight
		        scTL.x(), scBR.y(), txTL.x(), txBR.y(), // bottomLeft
		        scBR.x(), scBR.y(), txBR.x(), txBR.y(), // bottomRight
		    }
		);
		d_logger.DTrace(
		    "added character",
		    slog::String("screenTopLeft", to_string(scTL)),
		    slog::String("screenBottomRight", to_string(scBR)),
		    slog::Group(
		        "ch",
		        slog::String("textureTopLeft", to_string(txTL)),
		        slog::String("textureBottomRight", to_string(txBR)),
		        slog::String("screenTopLeft", to_string(ch.ScreenTopLeft)),
		        slog::String(
		            "screenBottomRight",
		            to_string(ch.ScreenBottomRight)
		        )
		    )
		);

		if (vertical == false) {
			res.d_width += ch.AdvanceX;
		} else {
			res.d_height += ch.AdvanceY;
		}
	}
	if (vertical == false) {
		res.d_renderSize = {res.d_width, BBbox.w() - BBbox.y()};
	} else {
		res.d_renderSize = {BBbox.z() - BBbox.x(), res.d_height};
	}

	for (const auto &[textureID, data] : dataPerTexture) {

		auto  VAO = pool->Get();
		GLint prebound, postbound;

		VAO->BufferData<float, 2, 2>(GL_STATIC_DRAW, data.data(), data.size());

		res.d_fragments.push_back(CompiledText::TextFragment{
		    .Texture  = textureID,
		    .Elements = data.size() / 4,
		    .VAO      = std::move(VAO),
		});
	}

	return res;
}

} // namespace gl
} // namespace fort
