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
unsigned int Element_Chunk::refer;

Element_Chunk::Element_Chunk()
{
    indexSize = 0;
}

Element_Chunk::Element_Chunk(unsigned int size)
    : indexSize(size)
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

Element_Chunk::Element_Chunk(Element_Chunk&& other)
    : index(other.index)
    , indexSize(other.indexSize)
{
    other.indexSize = 0;
}

Element_Chunk& Element_Chunk::operator=(Element_Chunk&& other)
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
    index           = other.index;
    indexSize       = other.indexSize;
    other.indexSize = 0;
    return *this;
}

void Element_Chunk::glDrawElements(
    GLenum mode,
    unsigned int size,
    unsigned int offset)
{
    if (!indexSize)
        return;
    ::glDrawElements(
        mode,
        size,
        GL_UNSIGNED_INT,
        (const void *)((index + offset) * sizeof(GLuint)));
}

void Element_Chunk::elementData(const GLuint elements[], int size)
{
    if (!indexSize)
        return;
    if (size < 0)
        size = indexSize;
    vbo->elementData(index, size, elements);
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
