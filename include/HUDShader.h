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

/* inherits from */
#include "Singleton.h"
#include "Shader.h"

#define HUDSHADER (HUDShader::instance())

class HUDShader: public Singleton<HUDShader>, public Shader
{
public:
    HUDShader();
    virtual ~HUDShader();

    void init() override;

    bool setTexturing(bool on);
protected:
    friend class Singleton<HUDShader>;

private:
    GLint texturingUniform;

    bool    texturing;
};

// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8
