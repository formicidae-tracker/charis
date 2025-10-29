#include "fort/gl/Shader.hpp"
#include "fort/gl/TextRenderer.hpp"
#include "fort/gl/Window.hpp"

#include <Eigen/src/Core/Matrix.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <codecvt>
#include <cstdint>
#include <locale>
#include <memory>
#include <slog++/Attribute.hpp>
#include <slog++/slog++.hpp>
#include <string>
static constexpr size_t font_size = 12;

// clang-format off
static const char32_t display_chars[] = {
    // Basic ASCII
    U'0',U'1',U'2',U'3',U'4',U'5',U'6',U'7',U'8',U'9',
    U'A',U'B',U'C',U'D',U'E',U'F',U'G',U'H',U'I',U'J',
    U'K',U'L',U'M',U'N',U'O',U'P',U'Q',U'R',U'S',U'T',
    U'U',U'V',U'W',U'X',U'Y',U'Z',
    U'a',U'b',U'c',U'd',U'e',U'f',U'g',U'h',U'i',U'j',
    U'k',U'l',U'm',U'n',U'o',U'p',U'q',U'r',U's',U't',
    U'u',U'v',U'w',U'x',U'y',U'z',
    // ASCII punctuation
    U'!',U'@',U'#',U'$',U'%',U'&',U'*',U'+',U'-',U'=',U'/',U'\\',U'|',U'_',U':',U';',
    U'<',U'>',U'?',U'[',U']',U'{',U'}',U'(',U')',U'.',U',',U'~',U'^',

    // Box-drawing and geometric (U+2500–U+257F, U+2580–U+259F)
    U'\u2500',U'\u2502',U'\u250C',U'\u2510',U'\u2514',U'\u2518',U'\u252C',U'\u2534',U'\u253C',
    U'\u2550',U'\u2551',U'\u2554',U'\u2557',U'\u255A',U'\u255D',U'\u256C',U'\u2570',U'\u2571',
    U'\u2580',U'\u2584',U'\u2588',U'\u2591',U'\u2592',U'\u2593',

    // Greek letters (U+0391–U+03C9)
    U'\u0391',U'\u0392',U'\u0393',U'\u0394',U'\u0395',U'\u0396',U'\u0397',U'\u0398',U'\u0399',U'\u039A',
    U'\u039B',U'\u039C',U'\u039D',U'\u039E',U'\u039F',U'\u03A0',U'\u03A1',U'\u03A3',U'\u03A4',U'\u03A5',
    U'\u03A6',U'\u03A7',U'\u03A8',U'\u03A9',
    U'\u03B1',U'\u03B2',U'\u03B3',U'\u03B4',U'\u03B5',U'\u03B6',U'\u03B7',U'\u03B8',U'\u03B9',U'\u03BA',
    U'\u03BB',U'\u03BC',U'\u03BD',U'\u03BE',U'\u03BF',U'\u03C0',U'\u03C1',U'\u03C3',U'\u03C4',U'\u03C5',
    U'\u03C6',U'\u03C7',U'\u03C8',U'\u03C9',


};
// clang-format on
static constexpr size_t DISPLAY_CHAR_NUM =
    sizeof(display_chars) / sizeof(char32_t);

struct StreamLine {
	float                  speed;
	std::string            data;
	float                  y;
	fort::gl::CompiledText _text;

	bool Done(const Eigen::Vector2i &vpSize) const {
		if (data.empty()) {
			return true;
		}
		auto bbox = _text.BoundingBox(
		    {.ViewportSize = vpSize, .Position = {0, y}, .Size = font_size}
		);
		slog::DDebug(
		    "streamline done test",
		    slog::Float("yPos", y),
		    slog::Group(
		        "BBox",
		        slog::Group(
		            "topLeft",
		            slog::Float("x", bbox.x()),
		            slog::Float("y", bbox.y())
		        ),
		        slog::Group(
		            "bottomRight",
		            slog::Float("x", bbox.z()),
		            slog::Float("y", bbox.w())
		        )
		    ),
		    slog::Group(
		        "VP",
		        slog::Int("width", vpSize.x()),
		        slog::Int("height", vpSize.y())
		    )
		);

		return bbox.y() > vpSize.y();
	}

	void
	Resample(fort::gl::TextRenderer &renderer, const Eigen::Vector2i &vpSize) {

		using Converter =
		    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t>;
		static Converter            converter;
		size_t                      total = std::rand() % 50 + 5;
		std::basic_string<char32_t> newStr;
		for (size_t i = 0; i < total; ++i) {
			newStr += display_chars[std::rand() % DISPLAY_CHAR_NUM];
		}
		data = converter.to_bytes(newStr);

		_text = renderer.Compile(data, 1, true);
		y     = std::rand() % 20 - 20;
		y -= _text
		         .BoundingBox(
		             {.ViewportSize = vpSize,
		              .Position     = {0, 0},
		              .Size         = font_size}
		         )
		         .w();
		//-_text.RenderSize().y();
		speed = 40.0 + (std::rand() % 200) / 10.0f;
	}

	void Render(const Eigen::Vector2i &vpSize, size_t x) const {
		_text.SetColor({0.0f, 1.0f, 0.0f, 1.0f});
		_text.SetBackgroundColor({1.0f, 0.0f, 0.0f, 1.0f});
		_text.Render(
		    {
		        .ViewportSize = vpSize,
		        .Position     = {x, y},
		        .Size         = font_size,
		    },
		    false
		);
	}
};

class Window : public fort::gl::Window {
	std::atomic<bool>                     d_continue = true;
	fort::gl::TextRenderer                d_renderer;
	std::vector<StreamLine>               d_seeds;
	std::chrono::system_clock::time_point d_last;
	fort::gl::CompiledText                d_test;

public:
	Window(int width, int height)
	    : fort::gl::Window{width, height, "gl-matrix"}
	    , d_renderer{"UbuntuMonoNerdFontMono", font_size} {
		updateStreamLineSeeds();
		d_last = std::chrono::system_clock::now();
		d_test = d_renderer.Compile("Hello\njurassic World!");
	}

	void Draw() override {
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		float x   = 4.0f;
		auto  now = std::chrono::system_clock::now();
		auto  ellapsed =
		    std::chrono::duration_cast<std::chrono::milliseconds>(now - d_last)
		        .count() /
		    1000.0f;
		d_last = now;

		for (auto &s : d_seeds) {
			s.y += s.speed * ellapsed;

			if (s.Done(ViewportSize())) {
				s.Resample(d_renderer, ViewportSize());
			}
			s.Render(ViewportSize(), x);
			x += font_size;
		}

		d_test.SetColor({1.0, 1.0, 0.0, 1.0});
		d_test.Render({
		    .ViewportSize = ViewportSize(),
		    .Position     = {0, font_size},
		    .Size         = font_size,
		});
	}

	bool Continue() const {
		return d_continue.load();
	}

	void OnSizeChanged(int width, int height) override {
		fort::gl::Window::OnSizeChanged(width, height);
		updateStreamLineSeeds();
	}

	void OnKey(int key, int scancode, int action, int mods) override {
		fort::gl::Window::OnKey(key, scancode, action, mods);
		if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) {
			d_continue.store(false);
		}
	}

	// void OnText(unsigned int codepoint) override {}

	void OnMouseMove(double x, double) override {}

	// void OnMouseInput(int button, int action, int mods) {}

	// void OnScroll(double xOffset, double yOffset) {}

	// void OnScaleChanged(float x, float y) {}

	void updateStreamLineSeeds() {
		size_t seeds = ViewportSize().x() / font_size + 1;
		d_seeds.resize(seeds);
		for (auto &s : d_seeds) {
			if (s.Done(ViewportSize())) {
				s.Resample(d_renderer, ViewportSize());
			}
		}
		slog::Info("seeds updated", slog::Int("size", d_seeds.size()));
	}
};

int main() {
	slog::DefaultLogger().From(slog::Level::Debug);
	auto window = std::make_unique<Window>(1024, 720);
	std::array<std::chrono::microseconds, 64> ellapsed;
	size_t                                    i = 0;
	while (window->Continue()) {

		window->Update();
		auto start = std::chrono::steady_clock::now();
		window->Process();
		ellapsed[i++] = std::chrono::duration_cast<std::chrono::microseconds>(
		    std::chrono::steady_clock::now() - start
		);
		i &= 63;
		if (i == 0) {
			int64_t min{1LL << 62}, max{0}, mean{0};
			for (auto e : ellapsed) {
				mean += e.count();
				min = std::min(e.count(), min);
				max = std::max(e.count(), max);
			}

			slog::Info(
			    "duration",
			    slog::Float("mean_ms", float(mean) / 64000.0f),
			    slog::Float("min_ms", min / 1000.0f),
			    slog::Float("max_ms", max / 1000.0f)
			);
		}
	}
}
