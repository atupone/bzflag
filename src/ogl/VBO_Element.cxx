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
#include "VBO_Element.h"

// System headers
#include <cstring>

VBO_Element::VBO_Element() :
    VBO_Handler(),
    vbosReady(false)
{
}

void VBO_Element::resize()
{
    // Start with min 256 and double the size after
    int newSize = vboSize < 256 ? 256 : 2 * vboSize;
    // Resize the VBO
    VBO_Handler::resize(newSize);
    // Resize all non empty in CPU memory copy
    if (!hostedElements.empty())
        hostedElements.resize(vboSize);
    if (!vbosReady)
        // Don't do anything on GL Context memory if not ready
        return;
    destroy();
    init();
}

void VBO_Element::init()
{
    vbosReady = true;
    if (!vboSize)
        return;
    // GL Context is ready
    // Bind buffer
    glGenBuffers(1, &elems);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elems);

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        vboSize * sizeof(GLuint),
        hostedElements.data(),
        GL_DYNAMIC_DRAW);
}

void VBO_Element::destroy()
{
    vbosReady = false;
    if (!vboSize)
        return;
    deleteBuffer();
}

std::string VBO_Element::vboName()
{
    return "E";
}

void VBO_Element::elementData(int index, int size, const GLuint element[])
{
    if (hostedElements.empty())
        hostedElements.resize(vboSize);
    memcpy(&hostedElements[index],
           element,
           size * sizeof(GLuint));
    if (!vbosReady)
        return;
    glBufferSubData(
        GL_ELEMENT_ARRAY_BUFFER,
        index * sizeof(GLuint),
        size * sizeof(GLuint),
        element);
}

void VBO_Element::bindBuffer()
{
    // Buffer not bound. Bind it
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elems);
}


void VBO_Element::deleteBuffer()
{
    // from GL Context remove buffer association
    glDeleteBuffers(1, &elems);
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
