#include "FontAtlas.hpp"

#include <cstring>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <iostream>

#include <fontconfig/fontconfig.h>
#include <freetype/freetype.h>
#include <freetype/fterrors.h>

#include <slog++/slog++.hpp>

#include <fort/utils/Defer.hpp>


namespace fort {
namespace gl {

class FontConfig {
public:
	FontConfig()
	    : d_config{FcInitLoadConfigAndFonts()} {}

	~FontConfig() {
		FcConfigDestroy(d_config);
	}

	FontConfig(const FontConfig &other)            = delete;
	FontConfig &operator=(const FontConfig &other) = delete;
	FontConfig(FontConfig &&other)                 = delete;
	FontConfig &operator=(FontConfig &&other)      = delete;

	std::string Locate(const std::string &name) {
		auto pattern = std::shared_ptr<FcPattern>(
		    FcNameParse((FcChar8 *)(name.c_str())),
		    &FcPatternDestroy
		);
		FcConfigSubstitute(d_config, pattern.get(), FcMatchPattern);
		FcDefaultSubstitute(pattern.get());
		FcResult   result;
		FcPattern *font = FcFontMatch(d_config, pattern.get(), &result);
		FcChar8   *file = NULL;
		if (font &&
		    FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
			return std::string((char *)(file));
		}
		throw std::runtime_error("could not find font '" + name + "'");
	}

private:
	FcConfig *d_config;
};

class FTFace {
public:
	FTFace(FT_Face face)
	    : d_face{face} {

		auto err = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
		if (err) {
			FT_Done_Face(face);
			throw std::runtime_error(
			    "could not set unicode encoding: " +
			    std::string(FT_Error_String(err))
			);
		}
	}

	~FTFace() {
		FT_Done_Face(d_face);
	}

	void SetPixelHeight(int height) {
		auto err = FT_Set_Pixel_Sizes(d_face, 0, height);
		if (err) {
			throw std::runtime_error(
			    "could not set face height: " +
			    std::string(FT_Error_String(err))
			);
		}
	}

	FT_Face get() const noexcept {
		return d_face;
	}

	FTFace(const FTFace &other)            = delete;
	FTFace &operator=(const FTFace &other) = delete;
	FTFace(FTFace &&other)                 = delete;
	FTFace &operator=(FTFace &&other)      = delete;

private:
	FT_Face d_face;
};

class FTLibrary {
public:
	using Face =
	    std::unique_ptr<std::remove_reference<FT_Face>, void (*)(FT_Face)>;

	FTLibrary() {
		auto err = FT_Init_FreeType(&d_lib);
		if (err) {
			throw std::runtime_error(
			    "could not init FreeType library: " +
			    std::string(FT_Error_String(err))
			);
		}
	}

	~FTLibrary() {
		FT_Done_FreeType(d_lib);
	}

	std::unique_ptr<FTFace> Open(const std::string &name) {
		FT_Face res;
		auto    err = FT_New_Face(d_lib, name.c_str(), 0, &res);
		if (err) {
			throw std::runtime_error(
			    "could not open FreetType face: " +
			    std::string(FT_Error_String(err))
			);
		}
		return std::make_unique<FTFace>(res);
	}

	FTLibrary(const FTLibrary &other)            = delete;
	FTLibrary &operator=(const FTLibrary &other) = delete;
	FTLibrary(FTLibrary &&other)                 = delete;
	FTLibrary &operator=(FTLibrary &&other)      = delete;

private:
	FT_Library d_lib;
};

FontAtlas::FontAtlas(
    const std::string &fontName, size_t fontSize, size_t pageSize
)
    : d_pageSize{pageSize}
    , d_fontSize{float(fontSize)}
    , d_face{nullptr}
    , d_logger{slog::With(
          slog::String("group", "FontAtlas"),
          slog::Group(
              "font",
              slog::String("name", fontName),
              slog::Float("size", float(fontSize)),
              slog::Int("pageSize", pageSize)
          )
      )} {
	static FTLibrary  ft;
	static FontConfig fc;

	auto name = fc.Locate(fontName);
	slog::Info(
	    "font located",
	    slog::String("desired", fontName),
	    slog::String("found", name)
	);
	d_face = ft.Open(fc.Locate(fontName));

	d_face->SetPixelHeight(fontSize);
}

FontAtlas::~FontAtlas() = default;
FontAtlas::UTF8Converter FontAtlas::converter;

const CharTexture &FontAtlas::Get(char32_t code) noexcept {
	try {
		return d_atlas.at(code);
	} catch (const std::exception &) {
	}

	try {
		auto res      = load(code);
		d_atlas[code] = res;

		return d_atlas.at(code);
	} catch (const std::exception &e) {
		loggerForCode(code).Error("could not load glyph", slog::Err(e));

		return Get(char32_t(' ')); // return a space.
	}
}

std::string renderBitmap(FT_Bitmap &bitmap) {
	std::string res{"\n"};
	res.reserve(1 + 2 * bitmap.rows * (bitmap.pitch + 1));

	static std::array<std::string, 8> shades = {
	    "  ", // 0
	    "░░", // 1
	    "░░", // 2
	    "▒▒", // 3
	    "▒▒", // 4
	    "▓▓", // 5
	    "▓▓", // 6
	    "██", // 7
	};

	for (size_t j = 0; j < bitmap.rows; j++) {
		for (size_t i = 0; i < bitmap.width; i++) {
			size_t idx   = i + j * bitmap.pitch;
			size_t value = bitmap.buffer[idx] >> 5;
			res += shades[value];
		}
		res += "\n";
	}
	return res;
}

template <typename T> slog::Attribute slogVector(const char *name, const T &v) {
	return slog::Group(
	    name,
	    slog::Float("x", float(v.x())),
	    slog::Float("y", float(v.y()))
	);
}

void FontAtlas::LoadASCII() noexcept {
	for (char32_t code = 0; code < 128; ++code) {
		try {
			auto cht      = load(code);
			d_atlas[code] = cht;
		} catch (const std::exception &e) {
			d_logger.DError("ASCII char not found", slog::Int("code", code));
		}
	}
}

CharTexture FontAtlas::load(char32_t code) {
	FT_Face face = d_face->get();
	if (code == U'\n') {
		return CharTexture{
		    .Texture            = 0,
		    .ScreenTopLeft      = {0, 0},
		    .ScreenBottomRight  = {0, 0},
		    .TextureTopLeft     = {0, 0},
		    .TextureBottomRight = {0, 0},
		    .newline            = true,
		    .AdvanceX           = 0.0f,
		    .AdvanceY = face->size->metrics.height / (64.0f * d_fontSize),
		};
	}
	if (FT_Get_Char_Index(face, code) == 0) {
		throw std::runtime_error(
		    "could not load char U+" + std::to_string(code) + ": no index found"
		);
	}
	auto err = FT_Load_Char(face, code, FT_LOAD_RENDER);
	if (err) {
		throw std::runtime_error(
		    "could not load char U+" + std::to_string(code) + ": " +
		    FT_Error_String(err)
		);
	}

	Point size{
	    face->glyph->bitmap.width + 2,
	    face->glyph->bitmap.rows + 2,
	};

	if ((size.array() > d_pageSize - 2).any()) {
		std::ostringstream oss;
		oss << "could not put glyph for code U+" << std::hex << code
		    << " in atlas: glyph size (" << size.transpose()
		    << ") does not fit in an atlas page ( " << d_pageSize << "x"
		    << d_pageSize << ")";
		throw std::runtime_error(oss.str());
	}

	auto [placement, texture] = place(size);

	glBindTexture(GL_TEXTURE_2D, texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	defer {
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		glBindTexture(GL_TEXTURE_2D, 0);
	};

	glTexSubImage2D(
	    GL_TEXTURE_2D,
	    0,
	    placement.Position.x() + 1,
	    placement.Position.y() + 1,
	    face->glyph->bitmap.pitch,
	    face->glyph->bitmap.rows,
	    GL_RED,
	    GL_UNSIGNED_BYTE,
	    face->glyph->bitmap.buffer
	);

	Eigen::Vector2f screenTopLeft{
	    face->glyph->bitmap_left,
	    face->glyph->bitmap_top - int(face->glyph->bitmap.rows),
	};
	Eigen::Vector2f screenBottomRight =
	    screenTopLeft +
	    (Eigen::Vector2f{face->glyph->bitmap.width, face->glyph->bitmap.rows});

	Eigen::Vector2f textureTopLeft =
	    placement.Position.cast<float>() + Eigen::Vector2f{1.0, 1.0};

	screenTopLeft /= d_fontSize;
	screenBottomRight /= d_fontSize;
	textureTopLeft /= d_pageSize;

	loggerForCode(code).DDebug(
	    "new glyph in atlas",
	    slog::Int("texture", texture),
	    slogVector("position", placement.Position + Eigen::Vector2i{1, 1}),
	    slog::Group(
	        "screen",
	        slogVector("screenTopLeft", screenTopLeft),
	        slogVector("screenBottomRight", screenBottomRight)
	    ),
	    slog::Group(
	        "bitmap",
	        slog::Int("width", face->glyph->bitmap.width),
	        slog::Int("rows", face->glyph->bitmap.rows),
	        slog::Int("pitch", face->glyph->bitmap.pitch),
	        slog::Int("top", face->glyph->bitmap_top),
	        slog::Int("left", face->glyph->bitmap_left)
	    ),
	    slog::Bool("rotated", placement.Rotated)
	);

	return CharTexture{
	    .Texture           = texture,
	    .ScreenTopLeft     = screenTopLeft,
	    .ScreenBottomRight = screenBottomRight,
	    .TextureTopLeft =
	        textureTopLeft +
	        Eigen::Vector2f{0, face->glyph->bitmap.rows} / d_pageSize,
	    .TextureBottomRight =
	        textureTopLeft +
	        Eigen::Vector2f{face->glyph->bitmap.width, 0} / d_pageSize,
	    .AdvanceX = (face->glyph->metrics.horiAdvance / (64 * d_fontSize)),
	    .AdvanceY =
	        FT_HAS_HORIZONTAL(face)
	            ? (face->glyph->metrics.vertAdvance / (64.0f * d_fontSize))
	            : face->size->metrics.height / (64.0f * d_fontSize)
	};
}

FontAtlas::AtlasPage FontAtlas::buildPage() const noexcept {
	GLuint texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	defer {
		glBindTexture(GL_TEXTURE_2D, 0);
	};
	std::vector<GLubyte> empty(d_pageSize * d_pageSize, 0);
	glTexImage2D(
	    GL_TEXTURE_2D,
	    0,
	    GL_RED,
	    d_pageSize,
	    d_pageSize,
	    0,
	    GL_RED,
	    GL_UNSIGNED_BYTE,
	    empty.data()
	);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	d_logger.DDebug("new page", slog::Int("textureID", texture));

	return {
	    .Texture = texture,
	    .Packer  = FontAtlas::Packer{int(d_pageSize), int(d_pageSize), true},
	};
}

std::tuple<FontAtlas::Placement, GLuint> FontAtlas::place(const Point &size) {
	std::optional<Placement> placement = std::nullopt;
	GLuint                   texture;

	for (auto &page : d_pages) {
		placement = page.Packer.Place(size);
		if (placement.has_value()) {
			texture = page.Texture;
			break;
		}
	}

	if (placement.has_value()) {
		return {placement.value(), texture};
	}

	d_pages.push_back(buildPage());

	return {
	    d_pages.back().Packer.Place(size).value(),
	    d_pages.back().Texture,
	};
}

slog::Logger<4> FontAtlas::loggerForCode(char32_t code) const noexcept {
	std::u32string str;
	str += code;
	return d_logger.With(
	    slog::Int("code", code),
	    slog::String("character", converter.to_bytes(str))
	);
}

} // namespace gl
} // namespace fort
