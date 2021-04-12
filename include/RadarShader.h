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

#define RADARSHADER (RadarShader::instance())

const int noChange   = 0;
const int colorScale = 1;
const int transScale = 2;
const int colorTrans = 3;

class RadarShader: public Singleton<RadarShader>, public Shader
{
public:
    RadarShader();
    ~RadarShader();

    void init() override;

    void setColorProcessing(int colorProcessing);
    void setZTank(float zTank);
    void setRadarKind(int radarKind);
    void setTexturing(bool on);
    void setRadarRange(float radarRange);
    void setNoisePattern(const glm::vec4 &noisePattern);
    void setViewWidth(float viewWidth);
    void setTeleData(float z, float bh);
    void setShotType(int shotType);
    void setSegmentedData(const float *orig, const glm::vec2 &dir,
                          float size, float length);
    void setColorHeight(const glm::vec3 &color, float height);
    void setObjColor(const glm::vec3 &objColor);
    void setDimsRot(float xSize, float ySize, float rot);
    void setOffset(float x, float y, float z);
    void setAlphaDim(float a);

    const int enhancedRadar = 0;
    const int fastRadar     = 1;
    const int emulFixed     = 2;
    const int jammed        = 3;
    const int viewLine      = 4;
    const int flag          = 5;
    const int dimming       = 6;
    const int tele          = 7;
    const int shot          = 8;
    const int solidColor    = 10;

    const int segmentedType = 0;
    const int sizedBullet   = 1;
    const int missileBullet = 2;
    const int noType        = 3;
protected:
    friend class Singleton<RadarShader>;

private:
    GLint zTankUniform;
    GLint radarKindUniform;
    GLint texturingUniform;
    GLint tsProcUniform;
    GLint csProcUniform;
    GLint radarRangeUniform;
    GLint noisePatternUniform;
    GLint viewWidthUniform;
    GLint teleDataUniform;
    GLint shotTypeUniform;
    GLint segmentedDataUniform;
    GLint colorHeightUniform;
    GLint objColorUniform;
    GLint dimsRotUniform;
    GLint offsetUniform;
    GLint alphaDimUniform;
};


// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8
