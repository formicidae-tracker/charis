#pragma once

#include <atomic>
#include <functional>
#include <memory>

#include <GL/glew.h>
// order matter
#include <GLFW/glfw3.h>

#include <slog++/slog++.hpp>

namespace fort {
namespace gl {

struct GLFWContext;

class Window {

public:
	using Ptr    = std::unique_ptr<Window>;
	using DrawFn = std::function<void()>;

	Window(int width, int height);
	virtual ~Window();

	bool WantClose();

	void Update();
	void ClearUpdate();

	void Process();

	virtual void Draw() = 0;

protected:
	virtual void OnSizeChanged(int width, int height);
	virtual void OnKey(int key, int scancode, int action, int mods);
	virtual void OnText(unsigned int codepoint);
	virtual void OnMouseMove(double x, double y);
	virtual void OnMouseInput(int button, int action, int mods);
	virtual void OnScroll(double xOffset, double yOffset);
	virtual void OnScaleChanged(float x, float y);

	struct GLFWwindow *GLFWwindow() const noexcept;

private:
	using GLFWWindowPtr =
	    std::unique_ptr<::GLFWwindow, void (*)(::GLFWwindow *)>;

	static GLFWWindowPtr OpenWindow(int width, int height);

	void InitOpenGL();
	void SetWindowCallback();

	std::unique_ptr<GLFWContext> d_context;
	GLFWWindowPtr                d_window;
	slog::Logger<1>              d_logger;
	std::atomic<bool>            d_needDraw;
};

} // namespace gl
} // namespace fort
