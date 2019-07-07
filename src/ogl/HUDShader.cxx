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

// implementation header
#include "HUDShader.h"

HUDShader::HUDShader(): Shader("HUD"), texturing(false)
{
}

HUDShader::~HUDShader()
{
}

void HUDShader::init()
{
    Shader::init();

    texturingUniform        = getUniformLocation("texturing");
}

bool HUDShader::setTexturing(bool on)
{
    bool oldTexturing = texturing;
    texturing = on;
    if (texturing |= oldTexturing)
        setUniform(texturingUniform, on);
    return oldTexturing;
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
