#include "fort/gl/Shader.hpp"
#include "fort/gl/TextRenderer.hpp"
#include "fort/gl/VAOPool.hpp"
#include "fort/gl/Window.hpp"

#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include <slog++/Attribute.hpp>
#include <slog++/slog++.hpp>

struct StreamLine {
	std::string data;
	size_t      x, y;
};

class Window : public fort::gl::Window {
	std::atomic<bool>      d_continue = true;
	fort::gl::TextRenderer d_renderer;
	fort::gl::CompiledText d_text;

public:
	Window(int width, int height)
	    : fort::gl::Window{width, height, "gl-matrix"}
	    , d_renderer{"UbuntuMono", 24} {
		d_text = d_renderer.Compile("Hello World!");
	}

	void Draw() override {
		std::time_t now = std::chrono::system_clock::to_time_t(
		    std::chrono::system_clock::now()
		);

		d_text = d_renderer.Compile(std::ctime(&now));
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		d_text.Render(fort::gl::CompiledText::RenderArgs{
		    .ViewportSize = ViewportSize(),
		    .Position     = {20, 20},
		    .Size         = 24.0,
		});
	}

	void OnSizeChanged(int width, int height) override {
		fort::gl::Window::OnSizeChanged(width, height);
	}

	bool Continue() const {
		return d_continue.load();
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
