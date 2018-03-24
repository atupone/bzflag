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

// forward declaration
class VBO_Element;

class Element_Chunk
{
public:
    Element_Chunk();
    Element_Chunk(unsigned int size);
    ~Element_Chunk();

    Element_Chunk(Element_Chunk&& other);
    Element_Chunk& operator=(Element_Chunk&& data);

    // These are for the case the caller provides a pointer to the first element
    void elementData(const GLuint elements[], int size = -1);

    void glDrawElements(GLenum mode, unsigned int size, unsigned int offset);

private:
    static VBO_Element *vbo;
    static unsigned int refer;

    int           index;
    unsigned int  indexSize;
};

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
