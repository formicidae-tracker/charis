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
    : d_textProgram{0}
    , d_backgroundProgram{0}
    , d_logger{slog::With(
          slog::String("group", "CompiledText"),
          slog::String("font", ""),
          slog::String("text", "")
      )} {}

CompiledText::CompiledText(
    GLuint textProgram, GLuint backgroundProgram, slog::Logger<3> &&logger
)
    : d_textProgram{textProgram}
    , d_backgroundProgram{backgroundProgram}
    , d_logger{std::move(logger)} {}

Eigen::Matrix4f
buildProjectionMatrix(const CompiledText::TextScreenPosition &args) {
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

Eigen::Vector4f CompiledText::BoundingBox(const TextScreenPosition &pos) const {
	Eigen::Vector4f res;

	Eigen::Matrix4f proj;
	proj << pos.Size, 0.0, 0.0, pos.Position.x(), // row 0
	    0.0, -pos.Size, 0.0, pos.Position.y(),    // row 1
	    0.0, 0.0, 1.0, 0.0,                       // row 3
	    0.0, 0.0, 0.0, 1.0;

	res.block<2, 1>(0, 0) =
	    (proj *
	     Eigen::Vector4f(d_boundingBox.x(), d_boundingBox.w(), 0.0f, 1.0f))
	        .block<2, 1>(0, 0);

	res.block<2, 1>(2, 0) =
	    (proj *
	     Eigen::Vector4f(d_boundingBox.z(), d_boundingBox.y(), 0.0f, 1.0f))
	        .block<2, 1>(0, 0);

	return res;
}

void CompiledText::SetColor(const Eigen::Vector4f &color) const {
	glUseProgram(d_textProgram);
	Upload(d_textProgram, "textColor", color);
}

void CompiledText::SetBackgroundColor(const Eigen::Vector4f &color) const {
	glUseProgram(d_backgroundProgram);
	Upload(d_backgroundProgram, "backgroundColor", color);
}

void CompiledText::renderBackground(const Eigen::Matrix4f &proj) const {
	glUseProgram(d_backgroundProgram);
	Upload(d_backgroundProgram, "projection", proj);
	const auto &f = d_fragments.front();
	glBindVertexArray(f.VAO->VAO);
	FORT_CHARIS_defer {
		glBindVertexArray(0);
	};
	glDrawArrays(GL_TRIANGLES, f.Elements, 6);
}

void CompiledText::Render(const TextScreenPosition &pos, bool renderBackground)
    const {
	if (d_fragments.empty()) {
		return;
	}
	auto projectionMatrix = buildProjectionMatrix(pos);
	if (renderBackground == true) {
		this->renderBackground(projectionMatrix);
	}

	glUseProgram(d_textProgram);

	Upload(d_backgroundProgram, "projection", projectionMatrix);

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

GLuint TextRenderer::s_textProgram{0};
GLuint TextRenderer::s_backgroundProgram{0};

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
	if (s_textProgram == 0) {
		s_textProgram = CompileProgram(
		    std::string{
		        shaders_text_vertex_start,
		        shaders_text_vertex_end,
		    },
		    std::string{
		        shaders_text_fragment_start,
		        shaders_text_fragment_end,
		    }
		);
	}
	if (s_backgroundProgram == 0) {
		s_backgroundProgram = CompileProgram(
		    std::string{
		        shaders_text_vertex_start,
		        shaders_text_vertex_end,
		    },
		    std::string{
		        shaders_background_fragment_start,
		        shaders_background_fragment_end
		    }
		);
	}

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
    int                             border_pt,
    bool                            vertical,
    slog::Logger<3>               &&logger
) const {
	static auto pool = std::make_shared<VAOPool>();
	logger.DTrace("compiling", slog::Int("size", characters.size()));
	CompiledText res{s_textProgram, s_backgroundProgram, std::move(logger)};

	using VertexData = std::vector<float>;

	std::map<GLuint, VertexData> dataPerTexture;

	float           width  = 0.0;
	float           height = 0.0;
	Eigen::Vector4f BBbox  = {
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        0.0,
        0.0
    };

	for (const auto &ch : characters) {
		if (ch.TextureBottomRight == ch.TextureTopLeft) {
			if (vertical == false) {
				width += ch.AdvanceX;
			} else {
				height -= ch.AdvanceY;
			}

			continue;
		}

		auto &data = dataPerTexture[ch.Texture];
		data.reserve(6 * 4 * characters.size());

		Eigen::Vector2f scTL =
		    ch.ScreenTopLeft + Eigen::Vector2f{width, height};
		Eigen::Vector2f scBR =
		    ch.ScreenBottomRight + Eigen::Vector2f{width, height};

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
			width += ch.AdvanceX;
		} else {
			height += -ch.AdvanceY;
		}
	}
	BBbox +=
	    Eigen::Vector4f{-1, -1, 1, 1} * float(border_pt) / d_atlas.FontSize();

	res.d_boundingBox = BBbox;

	d_logger.DDebug(
	    "added",
	    slog::Group(
	        "BBox",
	        slog::Group(
	            "topLeft",
	            slog::Float("x", BBbox.x()),
	            slog::Float("y", BBbox.y())
	        ),
	        slog::Group(
	            "bottomRight",
	            slog::Float("x", BBbox.z()),
	            slog::Float("y", BBbox.w())
	        )
	    )
	);

	if (dataPerTexture.size() == 0) {
		return res;
	}

	auto &firstData = dataPerTexture.begin()->second;
	firstData.insert(
	    firstData.end(),
	    {
	        BBbox.x(), BBbox.y(), 0.0f,
	        0.0f, // topLeft
	        BBbox.z(), BBbox.y(), 0.0f,
	        0.0f, // topRight
	        BBbox.x(), BBbox.w(), 0.0f,
	        0.0f, // bottomLeft
	        BBbox.z(), BBbox.y(), 0.0f,
	        0.0f, // topRight
	        BBbox.x(), BBbox.w(), 0.0f,
	        0.0f, // bottomLeft
	        BBbox.z(), BBbox.w(), 0.0f,
	        0.0f, // bottomRight
	    }
	);

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
	res.d_fragments.front().Elements -= 6; // removes the background.

	return res;
}

} // namespace gl
} // namespace fort
