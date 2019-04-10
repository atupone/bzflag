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
#include "PlayingShader.h"

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

void ClipPlane(int id, glm::vec4 plane)
{
#ifdef HAVE_GLES
    glClipPlanef(GL_CLIP_PLANE0 + id, value_ptr(plane));
#else
    const GLdouble myPlane[] = { plane[0], plane[1], plane[2], plane[3]};
    glClipPlane(GL_CLIP_PLANE0 + id, myPlane);
#endif
}

void Ortho(float left, float right, float bottom, float top, float nearVal, float farVal)
{
    glLoadIdentity();
#ifdef HAVE_GLES
    glOrthof(left, right, bottom, top, nearVal, farVal);
#else
    glOrtho(left, right, bottom, top, nearVal, farVal);
#endif
}

void ClearDepth()
{
#ifdef HAVE_GLES
    glClearDepthf(1.0);
#else
    glClearDepth(1.0);
#endif
}

void DepthRange(float depthRange, float depthRangeSize)
{
#ifdef HAVE_GLES
    GLfloat x_near = (GLfloat)depthRange * depthRangeSize;
    glDepthRangef(x_near, x_near + depthRangeSize);
#else
    GLclampd x_near = (GLclampd)depthRange * depthRangeSize;
    glDepthRange(x_near, x_near + depthRangeSize);
#endif
}

void LightModelLocalViewer(bool enable)
{
    SHADER.setLocalViewer(enable);
}

void LightModelSpecular(bool enable)
{
    SHADER.setSeparateColor(enable);
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
