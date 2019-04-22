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
#ifndef _OPENGLCOMMON_H
#define _OPENGLCOMMON_H

#include "common.h"

// System headers
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// common headers
#include "bzfgl.h"

namespace OpenGLCommon
{
void getFogColor(glm::vec4 &fogColor);
void setFogColor(const glm::vec4 &fogColor);
void setEyePlanes(const glm::vec4 &sPlane, const glm::vec4 &tPlane);
void getViewPort(glm::ivec4 &viewport);
void ClipPlane(int id, glm::vec4 plane);
void Ortho(float left, float right, float bottom, float top, float nearVal, float farVal);
void ClearDepth();
void DepthRange(float depthRange, float depthRangeSize);
void LightModelLocalViewer(bool enable);
void LightModelSpecular(bool enable);
}

#endif

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
