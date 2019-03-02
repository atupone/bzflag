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

#define SHADER (PlayingShader::instance())

class PlayingShader: public Singleton<PlayingShader>, public Shader
{
public:
    PlayingShader();
    virtual ~PlayingShader();

    void init() override;

    bool setTexturing(bool on);
    void setFogging(bool on);
    void setFogMode(GLuint mode);
    bool setLighting(bool on);
    void setLighting();
    void enableLight(int light, bool on);
    void setLights();
    void setRescaleNormal(bool on);
    void setNormalizeNormal(bool on);
    void setSeparateColor(bool on);
    void setSphereMap(bool on);
    void setLocalViewer(bool on);
    void setReplaceTexture(bool on);

    void setModel(int model);

    void setLineRainColor(glm::vec4 color[2]);
    void setLineRainAlphaMod(float alphaMod);

    const int ModelFixedPipe = 0;
    const int ModelLineRain  = 1;
    const int ModelRingXY    = 2;
    const int ModelRingYZ    = 3;
    const int ModelIDL         = 4;

    void setRingXYParam(float rad,
                        float topsideOffset,
                        float bottomUV,
                        float topUV,
                        float z);
    void setRingYZParam(float rad,
                        float topsideOffset,
                        float bottomUV,
                        float topUV,
                        float z,
                        float ZOffset);

    void setIDLGlobal(const glm::vec3 &origin, bool colorOverride);
    void setIDLLocal(const glm::vec3 cross[2], float dist);
protected:
    friend class Singleton<PlayingShader>;

private:
    const int FogExp    = 0;
    const int FogExp2   = 1;
    const int FogLinear = 2;

    GLint texturingUniform;
    GLint foggingUniform;
    GLint fogModeUniform;
    GLint lightingUniform;
    GLint lightsUniform;
    GLint localViewerUniform;

    GLint   rescaleUniform;
    GLint   normalizeUniform;
    GLint   separateColorUniform;
    GLint   sphereMapUniform;
    GLint   replaceTextureUniform;

    GLint modelUniform;

    GLint lineRainColorUniform;
    GLint lineRainAlphaModUniform;

    GLint ringParamUniform;
    GLint ringParam2Uniform;

    GLint   idlGlobalParamUniform;
    GLint   idlLocalParamUniform;

    GLint   lightEnabled[128];
    GLint   maxLights;
    bool    isLighting;
    bool    texturing;
};

// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8
