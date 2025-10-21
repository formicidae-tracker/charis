#include "Window.hpp"
#include "fort/gl/Shader.hpp"
#include <GLFW/glfw3.h>

#include <slog++/Attribute.hpp>
#include <slog++/Level.hpp>
#include <slog++/slog++.hpp>
#include <stdexcept>

#define throw_glfw_error(ctx)                                                  \
	do {                                                                       \
		const char *glfwErrorDescription;                                      \
		glfwGetError(&glfwErrorDescription);                                   \
		throw std::runtime_error(                                              \
		    std::string(ctx) + ": " + glfwErrorDescription                     \
		);                                                                     \
	} while (0)

namespace fort {
namespace gl {

struct GLFWContext {
	GLFWContext() {
		if (!glfwInit()) {
			throw_glfw_error("could not initialize GLFW");
		}
	}

	~GLFWContext() {
		glfwTerminate();
	}
};

slog::Attribute GLSource(GLenum source) {
	switch (source) {
	case GL_DEBUG_SOURCE_API:
		return slog::String("source", "GL_DEBUG_SOURCE_API");
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		return slog::String("source", "GL_DEBUG_SOURCE_WINDOW_SYSTEM");
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		return slog::String("source", "GL_DEBUG_SOURCE_SHADER_COMPILER");
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		return slog::String("source", "GL_DEBUG_SOURCE_THIRD_PARTY");
	case GL_DEBUG_SOURCE_APPLICATION:
		return slog::String("source", "GL_DEBUG_SOURCE_APPLICATION");
	case GL_DEBUG_SOURCE_OTHER:
		return slog::String("source", "GL_DEBUG_SOURCE_OTHER");
	default:
		return slog::String("source", "unknown");
	}
}

slog::Attribute GLType(GLenum type) {
	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		return slog::String("type", "GL_DEBUG_TYPE_ERROR");
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		return slog::String("type", "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR");
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		return slog::String("type", "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR");
	case GL_DEBUG_TYPE_PORTABILITY:
		return slog::String("type", "GL_DEBUG_TYPE_PORTABILITY");
	case GL_DEBUG_TYPE_PERFORMANCE:
		return slog::String("type", "GL_DEBUG_TYPE_PERFORMANCE");
	case GL_DEBUG_TYPE_MARKER:
		return slog::String("type", "GL_DEBUG_TYPE_MARKER");
	case GL_DEBUG_TYPE_PUSH_GROUP:
		return slog::String("type", "GL_DEBUG_TYPE_PUSH_GROUP");
	case GL_DEBUG_TYPE_POP_GROUP:
		return slog::String("type", "GL_DEBUG_TYPE_POP_GROUP");
	case GL_DEBUG_TYPE_OTHER:
		return slog::String("type", "GL_DEBUG_TYPE_OTHER");
	default:
		return slog::String("type", "unknown");
	}
}

slog::Level GLSeverity(GLenum severity) {
	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH:
		return slog::Level::Error;
	case GL_DEBUG_SEVERITY_MEDIUM:
		return slog::Level::Warn;
	case GL_DEBUG_SEVERITY_LOW:
		return slog::Level::Info;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		return slog::Level::Debug;
	default:
		return slog::Level::Trace;
	}
}

void logGL(
    GLenum        source,
    GLenum        type,
    GLuint        id,
    GLenum        severity,
    GLsizei       length,
    const GLchar *message,
    const void   *userParam
) {
	auto logger = reinterpret_cast<const slog::Logger<1> *>(userParam);

	logger->Log(
	    GLSeverity(severity),
	    message,
	    GLSource(source),
	    GLType(type),
	    slog::Int("ID", id)
	);
}

Window::Window(int width, int height, const std::string &name)
    : d_context{std::make_unique<GLFWContext>()}
    , d_window{OpenWindow(width, height, name)}
    , d_logger{slog::With(slog::String("group", "gl::Window"))}
    , d_needDraw(true) {

	glfwGetWindowSize(
	    d_window.get(),
	    &d_viewportSize.data()[0],
	    &d_viewportSize.data()[1]
	);

	InitOpenGL();

#ifndef NDEBUG
	int flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		auto logger =
		    new slog::Logger<1>{slog::With(slog::String("group", "OpenGL"))};
		glDebugMessageCallback(&logGL, logger);
		glDebugMessageControl(
		    GL_DONT_CARE,
		    GL_DONT_CARE,
		    GL_DONT_CARE,
		    0,
		    nullptr,
		    GL_TRUE
		);
		logger->Info("GL Output Enabled");
	}

#endif

	SetWindowCallback();
}

// needed as we have to define GLFWContext before.
Window::~Window() = default;

Window::GLFWWindowPtr
Window::OpenWindow(int width, int height, const std::string &name) {

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(
	    GLFW_OPENGL_FORWARD_COMPAT,
	    GL_TRUE
	); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

#ifndef NDEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

	// glfwWindowHint(GLFW_SAMPLES, 4);

	auto window = glfwCreateWindow(width, height, name.c_str(), NULL, NULL);
	if (!window) {
		throw_glfw_error("could not open window");
	}

	return GLFWWindowPtr{window, glfwDestroyWindow};
}

GLFWwindow *Window::GLFWwindow() const noexcept {
	return d_window.get();
}

void Window::InitOpenGL() {
	glfwMakeContextCurrent(d_window.get());

	const GLenum err = glewInit();
	if (err != GLEW_OK) {
		throw std::runtime_error(
		    std::string("could not initialize GLEW: ") +
		    reinterpret_cast<const char *>(glewGetErrorString(err))
		);
	}
}

void Window::SetWindowCallback() {
	glfwSetWindowUserPointer(d_window.get(), this);

	glfwSetFramebufferSizeCallback(
	    d_window.get(),
	    [](struct GLFWwindow *w, int width, int height) {
		    static_cast<Window *>(glfwGetWindowUserPointer(w))
		        ->OnSizeChanged(width, height);
	    }
	);

	glfwSetKeyCallback(
	    d_window.get(),
	    [](struct GLFWwindow *w, int key, int scancode, int action, int mods) {
		    static_cast<Window *>(glfwGetWindowUserPointer(w))
		        ->OnKey(key, scancode, action, mods);
	    }
	);

	glfwSetCharCallback(
	    d_window.get(),
	    [](struct GLFWwindow *w, unsigned int codepoint) {
		    static_cast<Window *>(glfwGetWindowUserPointer(w))
		        ->OnText(codepoint);
	    }
	);

	glfwSetCursorPosCallback(
	    d_window.get(),
	    [](struct GLFWwindow *w, double xpos, double ypos) {
		    static_cast<Window *>(glfwGetWindowUserPointer(w))
		        ->OnMouseMove(xpos, ypos);
	    }
	);

	glfwSetMouseButtonCallback(
	    d_window.get(),
	    [](struct GLFWwindow *w, int button, int action, int mods) {
		    static_cast<Window *>(glfwGetWindowUserPointer(w))
		        ->OnMouseInput(button, action, mods);
	    }
	);

	glfwSetScrollCallback(
	    d_window.get(),
	    [](struct GLFWwindow *w, double xOffset, double yOffset) {
		    static_cast<Window *>(glfwGetWindowUserPointer(w))
		        ->OnScroll(xOffset, yOffset);
	    }
	);

	glfwSetWindowContentScaleCallback(
	    d_window.get(),
	    [](struct GLFWwindow *w, float xscale, float yscale) {
		    auto self = static_cast<Window *>(glfwGetWindowUserPointer(w));
		    self->OnScaleChanged(xscale, yscale);
	    }
	);
}

void Window::OnScaleChanged(float xscale, float yscale) {
	d_logger.Debug(
	    "content scale changed",
	    slog::Float("x", xscale),
	    slog::Float("y", yscale)
	);
}

void Window::OnSizeChanged(int width, int height) {
	d_logger.Debug(
	    "size changed",
	    slog::Int("width", width),
	    slog::Int("height", height)
	);
	d_viewportSize = {width, height};

	glViewport(0, 0, width, height);
	Update();
}

void Window::OnKey(int key, int scancode, int action, int mods) {
	d_logger.Debug(
	    "key pressed",
	    slog::Int("key", key),
	    slog::Int("scancode", scancode),
	    slog::Int("action", action),
	    slog::Int("mods", mods)
	);
}

void Window::OnText(unsigned int codepoint) {
	d_logger.Debug("on text", slog::Int("codepoint", codepoint));
}

void Window::OnMouseMove(double x, double y) {
	d_logger.Debug("mouse moved", slog::Float("x", x), slog::Float("y", y));
}

void Window::OnMouseInput(int button, int action, int mods) {
	d_logger.Debug(
	    "mouse input",
	    slog::Int("button", button),
	    slog::Int("action", action),
	    slog::Int("mods", mods)
	);
}

void Window::OnScroll(double xOffset, double yOffset) {
	d_logger.Debug(
	    "mouse scroll",
	    slog::Float("xOffset", xOffset),
	    slog::Float("yOffset", yOffset)
	);
}

bool Window::WantClose() {
	return glfwWindowShouldClose(d_window.get());
}

void Window::Process() {
	glfwPollEvents();
	if (d_needDraw.exchange(false) == false) {
		return;
	}
	Draw();
	glfwSwapBuffers(d_window.get());
}

void Window::Update() {
	d_needDraw.store(true);
}

void Window::ClearUpdate() {
	d_needDraw.store(false);
}

} // namespace gl
} // namespace fort
