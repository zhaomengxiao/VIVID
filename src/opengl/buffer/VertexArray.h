#ifndef VERTEXARRAY_H
#define VERTEXARRAY_H

#include "../GLErrorHandler.h"
#include "VertexBufferLayout.h"

// 前向声明
class VertexBuffer;

class VertexArray
{
private:
    unsigned int m_RendererId;

public:
    VertexArray();
    ~VertexArray();

    void AddBuffer(const VertexBuffer &vb, const VertexBufferLayout &layout);
    void Bind() const;
    void Unbind() const;
};

#endif // VERTEXARRAY_H