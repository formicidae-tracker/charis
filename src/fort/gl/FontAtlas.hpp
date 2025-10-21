#pragma once

#include <GL/glew.h>
// order matter
#include <GL/gl.h>

#include <Eigen/Core>

#include <codecvt>
#include <locale>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <slog++/slog++.hpp>
#include "RectanglePacker.hpp"


namespace fort {
namespace gl {

class FTFace;

struct CharTexture {
	GLuint          Texture;
	Eigen::Vector2f ScreenTopLeft, ScreenBottomRight, TextureTopLeft,
	    TextureBottomRight;
	float AdvanceX;
	float AdvanceY;
};

class FontAtlas {
public:
	using CodecUTF8     = std::codecvt_utf8<char32_t>;
	using UTF8Converter = std::wstring_convert<CodecUTF8, char32_t>;

	FontAtlas(const std::string &fontName, size_t fontSize, size_t pageSize);

	~FontAtlas();

	const CharTexture &Get(char32_t code) noexcept;

	template <typename Str>
	inline std::vector<CharTexture> Get(Str &&str) noexcept {
		std::vector<CharTexture> result;
		auto decoded = converter.from_bytes(std::forward<Str>(str));
		for (char32_t code : decoded) {
			result.push_back(Get(code));
		}
		return result;
	}

	void LoadASCII() noexcept;

	template <typename Str> inline void Load(Str &&characters) noexcept {
		auto decoded = converter.from_bytes(std::forward<Str>(characters));
		for (char32_t code : decoded) {
			try {
				load(code);
			} catch (const std::exception &) {
			}
		}
	}

private:
	using FacePtr = std::unique_ptr<FTFace>;

	static UTF8Converter converter;

	using Atlas     = std::unordered_map<char32_t, CharTexture>;
	using Packer    = RectanglePacker<int>;
	using Placement = Packer::Placement;
	using Point     = Packer::Point;

	struct AtlasPage {
		GLuint            Texture;
		FontAtlas::Packer Packer;
	};

	CharTexture load(char32_t code);

	AtlasPage buildPage() const noexcept;

	slog::Logger<4> loggerForCode(char32_t code) const noexcept;

	std::tuple<Placement, GLuint> place(const Point &size);

	size_t                 d_pageSize;
	float                  d_fontSize;
	FacePtr                d_face;
	Atlas                  d_atlas;
	std::vector<AtlasPage> d_pages;
	slog::Logger<2>        d_logger;
};

} // namespace gl
}; // namespace fort
