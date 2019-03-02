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
    sphereMapUniform        = getUniformLocation("sphereMap");
    replaceTextureUniform   = getUniformLocation("replaceTexture");

    modelUniform            = getUniformLocation("model");

    lineRainColorUniform    = getUniformLocation("rainLineColor");
    lineRainAlphaModUniform = getUniformLocation("rainLinealphaMod");

    ringParamUniform        = getUniformLocation("ringParam");
    ringParam2Uniform       = getUniformLocation("ringParam2");

    idlGlobalParamUniform  = getUniformLocation("idlGlobalParam");
    idlLocalParamUniform    = getUniformLocation("idlLocalParam");

    glGetIntegerv(GL_MAX_LIGHTS, &maxLights);
    if (maxLights > 128)
        maxLights = 128;
}

bool PlayingShader::setTexturing(bool on)
{
    bool oldTexturing = texturing;
    texturing = on;
    if (texturing |= oldTexturing)
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

void PlayingShader::setSphereMap(bool on)
{
    setUniform(sphereMapUniform, on);
}

void PlayingShader::setReplaceTexture(bool on)
{
    setUniform(replaceTextureUniform, on);
}


void PlayingShader::setModel(int model)
{
    setUniform(modelUniform, model);
}

void PlayingShader::setRingXYParam(float rad,
                                   float topsideOffset,
                                   float bottomUV,
                                   float topUV,
                                   float z)
{
    auto ringParam  = glm::vec4(rad, topsideOffset, bottomUV, topUV);
    auto ringParam2 = glm::vec4(0.0f, z, 0.0f, 0.0f);
    setUniform(ringParamUniform, ringParam);
    setUniform(ringParam2Uniform, ringParam2);
}

void PlayingShader::setRingYZParam(float rad,
                                   float topsideOffset,
                                   float bottomUV,
                                   float topUV,
                                   float z,
                                   float ZOffset)
{
    auto ringParam  = glm::vec4(rad, topsideOffset, bottomUV, topUV);
    auto ringParam2 = glm::vec4(1.0f, z, ZOffset, 0.0f);
    setUniform(ringParamUniform,  ringParam);
    setUniform(ringParam2Uniform, ringParam2);
}

void PlayingShader::setLineRainColor(glm::vec4 color[2])
{
    setUniform(lineRainColorUniform, 2, color);
}

void PlayingShader::setLineRainAlphaMod(float alphaMod)
{
    setUniform(lineRainAlphaModUniform, alphaMod);
}

void PlayingShader::setIDLGlobal(const glm::vec3 &origin, bool colorOverride)
{
    auto globalParam = glm::vec4(origin, colorOverride ? 1.0f : 0.0f);
    setUniform(idlGlobalParamUniform, globalParam);
}

void PlayingShader::setIDLLocal(const glm::vec3 cross[2], float dist)
{
    glm::vec4 localParam[2];
    localParam[0] = glm::vec4(cross[0], dist);
    localParam[1] = glm::vec4(cross[1], dist);
    setUniform(idlLocalParamUniform, 2, localParam);
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
