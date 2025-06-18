#ifndef VERTEXBUFFER_H
#define VERTEXBUFFER_H

#include "../GLErrorHandler.h"

class VertexBuffer
{
private:
    unsigned int m_RendererId;

public:
    VertexBuffer(const void *data, unsigned int size);
    ~VertexBuffer();

    void Bind() const;
    void Unbind() const;
};

#endif // VERTEXBUFFER_H