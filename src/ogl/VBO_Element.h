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

// inherits from
#include "VBO_Handler.h"

// system interface headers
#include <vector>

// common interface headers
#include "bzfgl.h"

// This class is for the Element (Indirect drawing)
class VBO_Element: public VBO_Handler
{
public:
    VBO_Element();
    ~VBO_Element() = default;

    // This should be called when there is not enough space on the current
    // VBO, so try to double the size of it.
    // It start from 256
    void resize() override;

    // Init will be called when a GL Context is ready
    // It creates all the GL buffers requested,
    void init() override;
    // Destroy will delete the GL Buffers
    void destroy() override;
    // Name of the VBO
    std::string vboName() override;

    void elementData(
        int index,
        int size,
        const GLuint element[]);

private:
    GLuint elems;

    void bindBuffer();
    void deleteBuffer();

    // in CPU memory contents of GPU memory
    std::vector<GLuint> hostedElements;

    bool vbosReady;
};

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
