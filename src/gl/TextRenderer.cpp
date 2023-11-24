#include "TextRenderer.hpp"
#include "apollo/gl/FontAtlas.hpp"
#include "apollo/utils/Defer.hpp"
#include "concurrentqueue.h"
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <type_traits>

#include <apollo/apollo_rc.h>

#include "Shader.hpp"

namespace fort {
namespace gl {

CompiledText::CompiledText() {}

CompiledText::CompiledText(GLuint program)
    : d_program(program) {}

Eigen::Matrix4f buildProjectionMatrix(const CompiledText::RenderArgs &args) {
	Eigen::Matrix4f res;

	// for each vertices, we need to:
	// 1. multiply per desired fontSize
	// 2. add position
	// 3. transform to generalized coordinates.
	res << 2.0 * args.Size / args.ViewportSize.x(), 0.0, 0.0,
	    2.0 * args.Position.x() / args.ViewportSize.x() - 1.0, // row 0
	    0.0, 2.0 * args.Size / args.ViewportSize.y(), 0.0,
	    2.0 * args.Position.y() / args.ViewportSize.y() - 1.0, // row 1
	    0.0, 0.0, 1.0, 0.0,                                    // row 2
	    0.0, 0.0, 0.0, 1.0;                                    // row 3

	return res;
}

Eigen::Vector4f CompiledText::BoundingBox(float size) const {
	return Eigen::Vector4f{0.0, d_hMin, d_width, d_hMax} * size;
}

void CompiledText::Render(const RenderArgs &args) const {
	if (d_fragments.empty()) {
		return;
	}

	const auto &logger = slog::With(slog::String("group", "CompiledText"));

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
		logger.Debug(
		    "rendering fragment",
		    slog::Int("VAO", f.VAO->VAO),
		    slog::Int("elements", f.Elements)
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
    , d_logger{slog::With(slog::String("group", "TextRenderer"))} {
	d_atlas.LoadASCII();
	d_program = CompileProgram(
	    std::string{
	        gl_shaders_text_vertex_start,
	        gl_shaders_text_vertex_end,
	    },
	    std::string{
	        gl_shaders_text_fragment_start,
	        gl_shaders_text_fragment_end,
	    }
	);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

CompiledText TextRenderer::compile(
    const std::vector<CharTexture> &characters, const slog::Logger<2> &logger
) const {
	static auto pool = std::make_shared<CompiledText::Pool>();

	CompiledText res{d_program};

	using VertexData = std::vector<float>;

	std::map<GLuint, VertexData> dataPerTexture;

	res.d_width = 0.0;
	res.d_hMin  = std::numeric_limits<float>::max();
	res.d_hMax  = 0.0;

	for (const auto &ch : characters) {
		if (ch.TextureBottomRight == ch.TextureTopLeft) {
			res.d_width += ch.AdvanceX;
			continue;
		}

		auto &data = dataPerTexture[ch.Texture];
		data.reserve(6 * 4 * characters.size());

		Eigen::Vector2f screenTopLeft =
		    ch.ScreenTopLeft + Eigen::Vector2f{res.d_width, 0};
		Eigen::Vector2f screenBottomRight =
		    ch.ScreenBottomRight + Eigen::Vector2f{res.d_width, 0};

		res.d_hMin = std::min(res.d_hMin, screenTopLeft.y());
		res.d_hMax = std::max(res.d_hMax, screenBottomRight.y());

		data.insert(
		    data.end(),
		    {
		        screenTopLeft.x(),         // topLeft
		        screenTopLeft.y(),         //
		        ch.TextureTopLeft.x(),     //
		        ch.TextureTopLeft.y(),     //
		        screenBottomRight.x(),     // topRight
		        screenTopLeft.y(),         //
		        ch.TextureBottomRight.x(), //
		        ch.TextureTopLeft.y(),     //
		        screenTopLeft.x(),         // bottomLeft
		        screenBottomRight.y(),     //
		        ch.TextureTopLeft.x(),     //
		        ch.TextureBottomRight.y(), //
		        screenBottomRight.x(),     // topRight
		        screenTopLeft.y(),         //
		        ch.TextureBottomRight.x(), //
		        ch.TextureTopLeft.y(),     //
		        screenTopLeft.x(),         // bottomLeft
		        screenBottomRight.y(),     //
		        ch.TextureTopLeft.x(),     //
		        ch.TextureBottomRight.y(), //
		        screenBottomRight.x(),     // bottomRight
		        screenBottomRight.y(),     //
		        ch.TextureBottomRight.x(), //
		        ch.TextureBottomRight.y(), //
		    }
		);

		res.d_width += ch.AdvanceX;
	}

	for (const auto &[textureID, data] : dataPerTexture) {
		for (size_t i = 0; i < data.size(); i += CompiledText::Pool::Size) {

			auto  VAO = pool->Get();
			GLint prebound, postbound;

			glBindBuffer(GL_ARRAY_BUFFER, VAO->VBO);

			size_t fragmentSize =
			    std::min(data.size() - i, CompiledText::Pool::Size);

			logger.Debug(
			    "emit fragment",
			    slog::Int("textureID", textureID),
			    slog::Int("elements", fragmentSize / 4),
			    slog::Int("VAO", VAO->VAO),
			    slog::Int("VBO", VAO->VBO)
			);

			glBufferSubData(
			    GL_ARRAY_BUFFER,
			    0,
			    sizeof(float) * fragmentSize,
			    &(data[i])
			);

			glBindBuffer(GL_ARRAY_BUFFER, 0);

			res.d_fragments.push_back(CompiledText::TextFragment{
			    .Texture  = textureID,
			    .Elements = fragmentSize / 4,
			    .VAO      = std::move(VAO),
			});
		}
	}

	return res;
}

} // namespace gl
} // namespace fort
