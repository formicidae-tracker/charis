#include "fort/gl/Shader.hpp"
#include "fort/gl/VAOPool.hpp"
#include "fort/gl/Window.hpp"

#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include <slog++/Attribute.hpp>
#include <slog++/slog++.hpp>

const char *vertexShaderSource =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";
const char *fragmentShaderSource =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\n\0";

class Window : public fort::gl::Window {
	std::atomic<bool> d_continue = true;
	using VAOPool                = fort::gl::VAOPool<float, 3>;
	std::shared_ptr<VAOPool>        d_pool;
	VAOPool::VertexArrayObject::Ptr d_triangle;
	GLuint                          d_program;

public:
	Window(int width, int height)
	    : fort::gl::Window{width, height} {
		d_pool           = std::make_shared<VAOPool>();
		d_triangle       = d_pool->Get();
		// clang-format off
		float vertices[] = {
		    -0.5f, -0.5f, 0.0f, //
		     0.5f, -0.5f, 0.0f, //
		     0.0f,  0.5f, 0.0f //
		};
		// clang-format on
		d_triangle->BindBuffer(GL_STATIC_DRAW, vertices, 9);
		d_program =
		    fort::gl::CompileProgram(vertexShaderSource, fragmentShaderSource);
		slog::Info("shader program", slog::Int("programID", d_program));
	}

	void Draw() override {
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(d_program);
		glBindVertexArray(d_triangle->VAO);

		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindVertexArray(0);
	}

	void OnSizeChanged(int width, int height) override {}

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

			slog::Trace(
			    "duration",
			    slog::Float("mean_ms", float(mean) / 64000.0f),
			    slog::Float("min_ms", min / 1000.0f),
			    slog::Float("max_ms", max / 1000.0f)
			);
		}
	}
}
