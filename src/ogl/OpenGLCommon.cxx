/* bzflag
 * Copyright (c) 1993-2018 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "OpenGLCommon.h"

// System headers
#include <glm/gtc/type_ptr.hpp>

// Common headers
#include "bzfgl.h"

namespace OpenGLCommon
{
void getFogColor(glm::vec4 &fogColor)
{
    glGetFloatv(GL_FOG_COLOR, glm::value_ptr(fogColor));
}

void setFogColor(const glm::vec4 &fogColor)
{
    glFogfv(GL_FOG_COLOR, glm::value_ptr(fogColor));
}

void setEyePlanes(const glm::vec4 &sPlane,
                  const glm::vec4 &tPlane)
{
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_S, GL_EYE_PLANE, glm::value_ptr(sPlane));
    glTexGenfv(GL_T, GL_EYE_PLANE, glm::value_ptr(tPlane));
}

void ClipPlane(int id, glm::vec4 plane)
{
    const GLdouble myPlane[] = { plane[0], plane[1], plane[2], plane[3]};
    glClipPlane(GL_CLIP_PLANE0 + id, myPlane);
}

void Ortho(float left, float right, float bottom, float top, float nearVal, float farVal)
{
    glLoadIdentity();
    glOrtho(left, right, bottom, top, nearVal, farVal);
}

void ClearDepth()
{
    glClearDepth(1.0);
}

void DepthRange(float depthRange, float depthRangeSize)
{
    GLclampd x_near = (GLclampd)depthRange * depthRangeSize;
    glDepthRange(x_near, x_near + depthRangeSize);
}

void LightModelLocalViewer(bool enable)
{
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, enable ? GL_TRUE : GL_FALSE);
}

void LightModelSpecular(bool enable)
{
    const GLint model = enable ? GL_SEPARATE_SPECULAR_COLOR : GL_SINGLE_COLOR;
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, model);
}

void getViewPort(glm::ivec4 &viewport)
{
    glGetIntegerv(GL_VIEWPORT, glm::value_ptr(viewport));
}
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
