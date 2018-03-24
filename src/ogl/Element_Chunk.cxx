/* bzflag
 * Copyright (c) 1993-2018 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

// interface header
#include "Element_Chunk.h"

// Common headers
#include "VBO_Element.h"
#include "OpenGLGState.h"

VBO_Element *Element_Chunk::vbo = nullptr;
unsigned int Element_Chunk::refer = 0;

Element_Chunk::Element_Chunk(const Vertex_Chunk* vChunk_, unsigned int size, GLenum mode)
    : vChunk(vChunk_),
      indexSize(size),
      drawMode(mode)
{
    if (!indexSize)
        return;
    if (vbo)
    {
        refer++;
        index = vbo->alloc(size);
        return;
    }
    vbo = new VBO_Element();
    if (OpenGLGState::haveGLContext())
        vbo->init();
    refer = 1;
    index = vbo->alloc(size);
}

Element_Chunk::~Element_Chunk()
{
    if (!vbo)
        return;
    if (!indexSize)
        return;
    vbo->free(index);
    if (--refer)
        return;
    delete vbo;
    vbo = nullptr;
}

Element_Chunk::Element_Chunk(Element_Chunk&& other) noexcept
    : vChunk(other.vChunk)
    , index(other.index)
    , indexSize(other.indexSize)
    , drawMode(other.drawMode)
{
    other.vChunk    = nullptr;
    other.indexSize = 0;
}

Element_Chunk& Element_Chunk::operator=(Element_Chunk&& other) noexcept
{
    if (this == &other)
        return *this;
    if (indexSize)
    {
        vbo->free(index);
        if (!--refer)
        {
            delete vbo;
            vbo = nullptr;
        }
    }
    vChunk          = other.vChunk;
    index           = other.index;
    indexSize       = other.indexSize;
    drawMode        = other.drawMode;
    other.vChunk    = nullptr;
    other.indexSize = 0;
    return *this;
}

void Element_Chunk::enableArrays(bool texture, bool normal, bool color)
{
    if (vChunk)
        const_cast<Vertex_Chunk*>(vChunk)->enableArrays(texture, normal, color);
}

void Element_Chunk::elementData(const GLuint elements[], int size)
{
    if (!indexSize)
        return;
    if (size < 0)
        size = indexSize;

    // Get the base vertex offset from the associated Vertex_Chunk
    const GLuint baseVertexOffset = vChunk ? vChunk->getIndex() : 0;

    std::vector<GLuint> adjustedIndices(size);
    for (int i = 0; i < size; ++i)
        adjustedIndices[i] = elements[i] + baseVertexOffset;

    vbo->elementData(index, size, adjustedIndices.data());
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
