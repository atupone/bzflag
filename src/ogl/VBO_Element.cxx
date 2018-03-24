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
    if (vbosReady && vboSize)
        glDeleteBuffers(1, &elems);

    int newSize = vboSize < 256 ? 256 : 2 * vboSize;
    VBO_Handler::resize(newSize);
    if (!hostedElements.empty())
        hostedElements.resize(vboSize);
    init();
}

void VBO_Element::init()
{
    vbosReady = true;
    if (!vboSize)
        return;
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
}

std::string VBO_Element::vboName()
{
    return "E";
}

void VBO_Element::elementData(
    int index,
    int size,
    const GLuint element[])
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

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
