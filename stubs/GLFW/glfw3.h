#pragma once

#include <cstdint>
#include <cstdlib>

#define GLFW_TRUE 1
#define GLFW_FALSE 0

#define GLFW_PRESS 1
#define GLFW_RELEASE 0
#define GLFW_REPEAT 2

#define GLFW_CONTEXT_VERSION_MAJOR 0x00022002
#define GLFW_CONTEXT_VERSION_MINOR 0x00022003
#define GLFW_OPENGL_PROFILE 0x00022008
#define GLFW_OPENGL_ANY_PROFILE 0

#define GLFW_KEY_UNKNOWN -1
#define GLFW_KEY_SPACE 32
#define GLFW_KEY_APOSTROPHE 39
#define GLFW_KEY_COMMA 44
#define GLFW_KEY_MINUS 45
#define GLFW_KEY_PERIOD 46
#define GLFW_KEY_SLASH 47
#define GLFW_KEY_0 48
#define GLFW_KEY_1 49
#define GLFW_KEY_2 50
#define GLFW_KEY_3 51
#define GLFW_KEY_4 52
#define GLFW_KEY_5 53
#define GLFW_KEY_6 54
#define GLFW_KEY_7 55
#define GLFW_KEY_8 56
#define GLFW_KEY_9 57
#define GLFW_KEY_SEMICOLON 59
#define GLFW_KEY_EQUAL 61
#define GLFW_KEY_A 65
#define GLFW_KEY_B 66
#define GLFW_KEY_C 67
#define GLFW_KEY_D 68
#define GLFW_KEY_E 69
#define GLFW_KEY_F 70
#define GLFW_KEY_G 71
#define GLFW_KEY_H 72
#define GLFW_KEY_I 73
#define GLFW_KEY_J 74
#define GLFW_KEY_K 75
#define GLFW_KEY_L 76
#define GLFW_KEY_M 77
#define GLFW_KEY_N 78
#define GLFW_KEY_O 79
#define GLFW_KEY_P 80
#define GLFW_KEY_Q 81
#define GLFW_KEY_R 82
#define GLFW_KEY_S 83
#define GLFW_KEY_T 84
#define GLFW_KEY_U 85
#define GLFW_KEY_V 86
#define GLFW_KEY_W 87
#define GLFW_KEY_X 88
#define GLFW_KEY_Y 89
#define GLFW_KEY_Z 90
#define GLFW_KEY_LEFT_BRACKET 91
#define GLFW_KEY_BACKSLASH 92
#define GLFW_KEY_RIGHT_BRACKET 93
#define GLFW_KEY_GRAVE_ACCENT 96
#define GLFW_KEY_WORLD_1 161
#define GLFW_KEY_WORLD_2 162

#define GLFW_KEY_ESCAPE 256
#define GLFW_KEY_ENTER 257
#define GLFW_KEY_TAB 258
#define GLFW_KEY_BACKSPACE 259
#define GLFW_KEY_INSERT 260
#define GLFW_KEY_DELETE 261
#define GLFW_KEY_RIGHT 262
#define GLFW_KEY_LEFT 263
#define GLFW_KEY_DOWN 264
#define GLFW_KEY_UP 265
#define GLFW_KEY_PAGE_UP 266
#define GLFW_KEY_PAGE_DOWN 267
#define GLFW_KEY_HOME 268
#define GLFW_KEY_END 269
#define GLFW_KEY_CAPS_LOCK 280
#define GLFW_KEY_SCROLL_LOCK 281
#define GLFW_KEY_NUM_LOCK 282
#define GLFW_KEY_PRINT_SCREEN 283
#define GLFW_KEY_PAUSE 284
#define GLFW_KEY_F1 290
#define GLFW_KEY_F2 291
#define GLFW_KEY_F3 292
#define GLFW_KEY_F4 293
#define GLFW_KEY_F5 294
#define GLFW_KEY_F6 295
#define GLFW_KEY_F7 296
#define GLFW_KEY_F8 297
#define GLFW_KEY_F9 298
#define GLFW_KEY_F10 299
#define GLFW_KEY_F11 300
#define GLFW_KEY_F12 301
#define GLFW_KEY_F13 302
#define GLFW_KEY_F14 303
#define GLFW_KEY_F15 304
#define GLFW_KEY_F16 305
#define GLFW_KEY_F17 306
#define GLFW_KEY_F18 307
#define GLFW_KEY_F19 308
#define GLFW_KEY_F20 309
#define GLFW_KEY_F21 310
#define GLFW_KEY_F22 311
#define GLFW_KEY_F23 312
#define GLFW_KEY_F24 313
#define GLFW_KEY_F25 314
#define GLFW_KEY_KP_0 320
#define GLFW_KEY_KP_1 321
#define GLFW_KEY_KP_2 322
#define GLFW_KEY_KP_3 323
#define GLFW_KEY_KP_4 324
#define GLFW_KEY_KP_5 325
#define GLFW_KEY_KP_6 326
#define GLFW_KEY_KP_7 327
#define GLFW_KEY_KP_8 328
#define GLFW_KEY_KP_9 329
#define GLFW_KEY_KP_DECIMAL 330
#define GLFW_KEY_KP_DIVIDE 331
#define GLFW_KEY_KP_MULTIPLY 332
#define GLFW_KEY_KP_SUBTRACT 333
#define GLFW_KEY_KP_ADD 334
#define GLFW_KEY_KP_ENTER 335
#define GLFW_KEY_KP_EQUAL 336
#define GLFW_KEY_LEFT_SHIFT 340
#define GLFW_KEY_LEFT_CONTROL 341
#define GLFW_KEY_LEFT_ALT 342
#define GLFW_KEY_LEFT_SUPER 343
#define GLFW_KEY_RIGHT_SHIFT 344
#define GLFW_KEY_RIGHT_CONTROL 345
#define GLFW_KEY_RIGHT_ALT 346
#define GLFW_KEY_RIGHT_SUPER 347
#define GLFW_KEY_MENU 348

struct GLFWwindow {
    int shouldClose{0};
    int width{0};
    int height{0};
    int frameCount{0};
};

inline GLFWwindow*& stubActiveWindow() {
    static GLFWwindow* window = nullptr;
    return window;
}

using GLFWerrorfun = void (*)(int, const char*);

inline void glfwSetErrorCallback(GLFWerrorfun) {}

inline int glfwInit() { return GLFW_TRUE; }
inline void glfwTerminate();
inline void glfwWindowHint(int, int) {}

inline GLFWwindow* glfwCreateWindow(int width, int height, const char*, void*, void*) {
    GLFWwindow* window = new GLFWwindow();
    window->width = width;
    window->height = height;
    stubActiveWindow() = window;
    return window;
}

inline void glfwDestroyWindow(GLFWwindow* window) {
    if (stubActiveWindow() == window) {
        stubActiveWindow() = nullptr;
    }
    delete window;
}

inline void glfwMakeContextCurrent(GLFWwindow*) {}
inline void glfwSwapInterval(int) {}
inline void glfwSetWindowTitle(GLFWwindow*, const char*) {}
inline void glfwSwapBuffers(GLFWwindow*) {}

inline void glfwGetFramebufferSize(GLFWwindow* window, int* width, int* height) {
    if (!window) {
        if (width) *width = 0;
        if (height) *height = 0;
        return;
    }
    if (width) *width = window->width;
    if (height) *height = window->height;
}

inline int glfwWindowShouldClose(GLFWwindow* window) {
    return window ? window->shouldClose : GLFW_TRUE;
}

inline void glfwSetWindowShouldClose(GLFWwindow* window, int value) {
    if (window) {
        window->shouldClose = value;
    }
}

inline void glfwPollEvents();

inline int glfwGetKey(GLFWwindow* window, int key) {
    if (!window) {
        return GLFW_RELEASE;
    }

    int frame = window->frameCount;

    // Simulate menu navigation and combat input in a deterministic pattern.
    if (frame < 8 && key == GLFW_KEY_ENTER) {
        return GLFW_PRESS; // Start game from title
    }
    if (frame >= 12 && frame < 90) {
        if (key == GLFW_KEY_W || key == GLFW_KEY_D) {
            return (frame % 4 == 0) ? GLFW_PRESS : GLFW_RELEASE;
        }
    }
    if (frame >= 120 && frame < 160 && key == GLFW_KEY_J) {
        return (frame % 6 == 0) ? GLFW_PRESS : GLFW_RELEASE;
    }
    if (frame >= 200 && frame < 240 && key == GLFW_KEY_K) {
        return (frame % 7 == 0) ? GLFW_PRESS : GLFW_RELEASE;
    }
    if (frame >= 260 && frame < 280 && key == GLFW_KEY_Q) {
        return GLFW_PRESS;
    }
    if (frame >= 280 && frame < 320 && key == GLFW_KEY_E) {
        return GLFW_PRESS;
    }
    if (frame >= 360 && frame < 380 && key == GLFW_KEY_SPACE) {
        return (frame % 3 == 0) ? GLFW_PRESS : GLFW_RELEASE;
    }
    if (frame >= 420 && frame < 440 && key == GLFW_KEY_ESCAPE) {
        return GLFW_PRESS; // Return to title
    }
    if (frame >= 460 && frame < 470 && key == GLFW_KEY_ENTER) {
        return GLFW_PRESS; // Confirm exit on title
    }
    if (frame >= 520 && key == GLFW_KEY_ESCAPE) {
        return GLFW_PRESS; // Close window if still running
    }

    return GLFW_RELEASE;
}

inline void glfwPollEvents(GLFWwindow* window) {
    if (window) {
        ++window->frameCount;
        if (window->frameCount > 600) {
            window->shouldClose = GLFW_TRUE;
        }
    }
}

inline void glfwPollEvents() {
    GLFWwindow* window = stubActiveWindow();
    if (window) {
        glfwPollEvents(window);
    }
}

inline void glfwTerminate() {
    GLFWwindow* window = stubActiveWindow();
    if (window) {
        delete window;
        stubActiveWindow() = nullptr;
    }
}

// Minimal OpenGL compatibility layer for headless builds.

using GLenum = unsigned int;
using GLbitfield = unsigned int;
using GLfloat = float;

#define GL_QUADS 0x0007
#define GL_TRIANGLE_FAN 0x0006
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_DEPTH_TEST 0x0B71
#define GL_BLEND 0x0BE2
#define GL_MODELVIEW 0x1700
#define GL_PROJECTION 0x1701

inline void glBegin(GLenum) {}
inline void glEnd() {}
inline void glColor3f(GLfloat, GLfloat, GLfloat) {}
inline void glVertex3f(GLfloat, GLfloat, GLfloat) {}
inline void glVertex2f(GLfloat, GLfloat) {}
inline void glPushMatrix() {}
inline void glPopMatrix() {}
inline void glTranslatef(GLfloat, GLfloat, GLfloat) {}
inline void glRotatef(GLfloat, GLfloat, GLfloat, GLfloat) {}
inline void glScalef(GLfloat, GLfloat, GLfloat) {}
inline void glMatrixMode(GLenum) {}
inline void glLoadIdentity() {}
inline void glOrtho(double, double, double, double, double, double) {}
inline void glLoadMatrixf(const GLfloat*) {}
inline void glViewport(int, int, int, int) {}
inline void glClearColor(GLfloat, GLfloat, GLfloat, GLfloat) {}
inline void glClear(GLbitfield) {}
inline void glDisable(GLenum) {}
inline void glEnable(GLenum) {}

