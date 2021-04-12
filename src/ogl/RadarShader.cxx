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

// Interface header
#include "RadarShader.h"

// System headers
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

RadarShader::RadarShader(): Shader("radar")
{
}

RadarShader::~RadarShader()
{
}

void RadarShader::init()
{
    Shader::init();
    zTankUniform           = getUniformLocation("zTank");
    radarKindUniform       = getUniformLocation("radarKind");
    texturingUniform       = getUniformLocation("texturing");
    tsProcUniform          = getUniformLocation("tsProc");
    csProcUniform          = getUniformLocation("csProc");
    radarRangeUniform      = getUniformLocation("radarRange");
    noisePatternUniform    = getUniformLocation("noisePattern");
    viewWidthUniform       = getUniformLocation("viewWidth");
    teleDataUniform        = getUniformLocation("teleData");
    shotTypeUniform        = getUniformLocation("shotType");
    segmentedDataUniform   = getUniformLocation("segmentedData");
    colorHeightUniform     = getUniformLocation("colorHeight");
    objColorUniform        = getUniformLocation("objColor");
    dimsRotUniform         = getUniformLocation("dimsRot");
    offsetUniform          = getUniformLocation("offset");
    alphaDimUniform        = getUniformLocation("alphaDim");
}

void RadarShader::setColorProcessing(int colorProcessing)
{
    switch (colorProcessing)
    {
    case noChange :
        setUniform(tsProcUniform, false);
        setUniform(csProcUniform, false);
        break;
    case colorScale :
        setUniform(tsProcUniform, false);
        setUniform(csProcUniform, true);
        break;
    case transScale :
        setUniform(tsProcUniform, true);
        setUniform(csProcUniform, false);
        break;
    case colorTrans :
        setUniform(tsProcUniform, true);
        setUniform(csProcUniform, true);
        break;
    }
}

void RadarShader::setZTank(float zTank)
{
    setUniform(zTankUniform, zTank);
}

void RadarShader::setRadarKind(int radarKind)
{
    setUniform(radarKindUniform, radarKind);
}

void RadarShader::setTexturing(bool on)
{
    setUniform(texturingUniform, on);
}

void RadarShader::setRadarRange(float radarRange)
{
    setUniform(radarRangeUniform, radarRange);
}

void RadarShader::setNoisePattern(const glm::vec4 &noisePattern)
{
    setUniform(noisePatternUniform, noisePattern);
}

void RadarShader::setViewWidth(float viewWidth)
{
    setUniform(viewWidthUniform, viewWidth);
}

void RadarShader::setTeleData(float z, float bh)
{
    auto teleData = glm::vec2(z, bh);
    setUniform(teleDataUniform, teleData);
}

void RadarShader::setShotType(int shotType)
{
    setUniform(shotTypeUniform, shotType);
}

void RadarShader::setSegmentedData(const float *orig, const glm::vec2 &dir,
                                   float size, float length)
{
    glm::vec4 segmentedData[2];
    segmentedData[0] = glm::vec4(glm::make_vec3(orig), 0.0f);
    segmentedData[1] = glm::vec4(dir, size, length);
    setUniform(segmentedDataUniform, 2, segmentedData);
}

void RadarShader::setColorHeight(const glm::vec3 &color, float height)
{
    auto colorHeight = glm::vec4(color, height);
    setUniform(colorHeightUniform, colorHeight);
}

void RadarShader::setObjColor(const glm::vec3 &objColor)
{
    setUniform(objColorUniform, objColor);
}

void RadarShader::setDimsRot(float xSize, float ySize, float rot)
{
    auto dimsRot = glm::vec3(xSize, ySize, rot);
    setUniform(dimsRotUniform, dimsRot);
}

void RadarShader::setOffset(float x, float y, float z)
{
    auto offset = glm::vec3(x, y, z);
    setUniform(offsetUniform, offset);
}

void RadarShader::setAlphaDim(float a)
{
    setUniform(alphaDimUniform, a);
}



// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
