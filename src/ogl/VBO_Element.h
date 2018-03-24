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

// This class is for the ELement (Indirect drawing)
class VBO_Element: public VBO_Handler
{
public:
    VBO_Element();
    ~VBO_Element() = default;

    // Increase the size
    void resize();

    void init() override;
    void destroy() override;
    std::string vboName() override;

    void elementData(
        int index,
        int size,
        const GLuint element[]);

private:
    GLuint elems;

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
