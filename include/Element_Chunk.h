/* bzflag
 * Copyright (c) 2019-2019 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#pragma once

// 1st
#include "common.h"

// system interface headers
#include <glm/fwd.hpp>
#include <vector>

// Common headers
#include "bzfgl.h"
#include "Vertex_Chunk.h"

// forward declaration
class VBO_Element;

class Element_Chunk
{
public:
    Element_Chunk() = default;
    Element_Chunk(const Vertex_Chunk* vChunk, unsigned int size, GLenum mode);
    ~Element_Chunk();

    Element_Chunk(Element_Chunk&& other) noexcept;
    Element_Chunk& operator=(Element_Chunk&& data) noexcept;

    // Enables array client states via referenced Vertex_Chunk
    void enableArrays(bool texture = true, bool normal = true, bool color = true);

    // These are for the case the caller provides a pointer to the first element
    void elementData(const GLuint elements[], int size = -1);

    inline void glDrawElements(unsigned int offset = 0) const
    {
        if (!indexSize)
            return;
        ::glDrawElements(
                drawMode,
                indexSize,
                GL_UNSIGNED_INT,
                (const void *)((index + offset) * sizeof(GLuint)));

    }

private:
    const Vertex_Chunk* vChunk = nullptr;
    static VBO_Element *vbo;
    static unsigned int refer;

    int           index = 0;
    unsigned int  indexSize = 0;
    GLenum        drawMode = GL_TRIANGLES;
};

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
