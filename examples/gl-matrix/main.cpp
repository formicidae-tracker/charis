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
static constexpr size_t font_size = 12;

size_t utf8_length(const std::string &w) {
	int len = 0;
	for (char c : w) {
		if ((c & 0xc0) != 0x80) {
			++len;
		}
	}
	return len;
}

bool chr_isvalid(char32_t c) {
	if (c <= 0x7F)
		return true;
	if (0xC080 == c)
		return true; // Accept 0xC080 as representation for '\0'
	if (0xC280 <= c && c <= 0xCFBF)
		return ((c & 0xE0C0) == 0xC080);
	return false;
	if (0xEDA080 <= c && c <= 0xEDBFBF)
		return false; // Reject UTF-16 surrogates
	if (0xE0A080 <= c && c <= 0xEFBFBF)
		return ((c & 0xF0C0C0) == 0xE08080);
	if (0xF0908080 <= c && c <= 0xF48FBFBF)
		return ((c & 0xF8C0C0C0) == 0xF0808080);
	return false;
}

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
		static Converter converter;
		size_t           total = std::rand() % 50 + 5;
		data                   = "";
		while (utf8_length(data) < total) {
			char32_t ch = std::rand() % 127;
			if (chr_isvalid(ch) == false) {
				continue;
			}
			data += converter.to_bytes(std::basic_string<char32_t>(1, ch));
		}

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

public:
	Window(int width, int height)
		: fort::gl::Window{width, height, "gl-matrix"}
		, d_renderer{"UbuntuMono", font_size} {
		updateStreamLineSeeds();
		d_last = std::chrono::system_clock::now();
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
