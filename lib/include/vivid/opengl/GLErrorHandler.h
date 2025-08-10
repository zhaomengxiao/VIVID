#ifndef GLERRORHANDLER_H
#define GLERRORHANDLER_H

#include <GL/glew.h>
#include <iostream>
#include <assert.h>

// OpenGL错误处理
#define GLCall(x)   \
    GLClearError(); \
    x;              \
    assert(GLLogCall(#x, __FILE__, __LINE__))

void GLClearError();
bool GLLogCall(const char *function, const char *file, int line);

#endif // GLERRORHANDLER_H