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
#include "PlayingShader.h"

// System headers
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

PlayingShader::PlayingShader(): Shader("playing"), isLighting(false),
    texturing(false)
{
    for (int i = 0; i < 128; i++)
        lightEnabled[i] = 0;
}

PlayingShader::~PlayingShader()
{
}

void PlayingShader::init()
{
    Shader::init();

    texturingUniform        = getUniformLocation("texturing");
    foggingUniform          = getUniformLocation("fogging");
    fogModeUniform          = getUniformLocation("fogMode");
    lightingUniform         = getUniformLocation("lighting");
    lightsUniform           = getUniformLocation("lights");
    localViewerUniform      = getUniformLocation("localViewer");
    rescaleUniform          = getUniformLocation("rescaleNormal");
    normalizeUniform        = getUniformLocation("normalizeNormal");
    separateColorUniform    = getUniformLocation("separateColor");
    texGenUniform           = getUniformLocation("texGen");
    zTankUniform            = getUniformLocation("zTank");
    repeatUniform           = getUniformLocation("repeat");
    csProcUniform           = getUniformLocation("csProc");

    glGetIntegerv(GL_MAX_LIGHTS, &maxLights);
    if (maxLights > 128)
        maxLights = 128;
}

bool PlayingShader::setTexturing(bool on)
{
    bool oldTexturing = texturing;
    texturing = on;
    if (texturing != oldTexturing)
        setUniform(texturingUniform, on);
    return oldTexturing;
}

void PlayingShader::setFogging(bool on)
{
    setUniform(foggingUniform, on);
}

void PlayingShader::setFogMode(GLuint mode)
{
    if (mode == GL_LINEAR)
        setUniform(fogModeUniform, FogLinear);
    else if (mode == GL_EXP)
        setUniform(fogModeUniform, FogExp);
    else if (mode == GL_EXP2)
        setUniform(fogModeUniform, FogExp2);
}

void PlayingShader::setLighting()
{
    setUniform(lightingUniform, isLighting);
}

void PlayingShader::enableLight(int light, bool on)
{
    lightEnabled[light] = on;
    setLights();
}

void PlayingShader::setLights()
{
    setUniform(lightsUniform, maxLights, lightEnabled);
}

void PlayingShader::setLocalViewer(bool on)
{
    setUniform(localViewerUniform, on);
}

void PlayingShader::setRescaleNormal(bool on)
{
    setUniform(rescaleUniform, on);
}

void PlayingShader::setNormalizeNormal(bool on)
{
    setUniform(normalizeUniform, on);
}

bool PlayingShader::setLighting(bool on)
{
    bool old = isLighting;
    setUniform(lightingUniform, on);
    isLighting = on;
    return old;
}

void PlayingShader::setSeparateColor(bool on)
{
    setUniform(separateColorUniform, on);
}

int PlayingShader::setTexGen(int texGenMode)
{
    int old = texGenMod;
    setUniform(texGenUniform, texGenMode);
    texGenMod = texGenMode;
    return old;
}

void PlayingShader::setZTank(float zTank)
{
    setUniform(zTankUniform, zTank);
}

void PlayingShader::setRepeat(float repeat)
{
    setUniform(repeatUniform, repeat);
}

void PlayingShader::setColorProcessing(int csProc)
{
    setUniform(csProcUniform, csProc);
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4