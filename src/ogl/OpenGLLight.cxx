/* bzflag
 * Copyright (c) 1993-2023 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

// Interface
#include "OpenGLLight.h"

// System headers
#include <glm/gtc/type_ptr.hpp>

// Common headers
#include "OpenGLGState.h"
#include "ViewFrustum.h"


GLint OpenGLLight::maxLights = 0;


OpenGLLight::OpenGLLight()
{
    pos = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
    color = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    atten[0] = 1.0f;
    atten[1] = 0.0f;
    atten[2] = 0.0f;
    maxDist = 50.0f;
    //
    // Here's the equation for maxDist:
    //
    //   c - the cutoff value (ex: 0.02 = 2%)
    //   d - falloff distance where percentage is less then 1/r
    //
    //   (d^2 * atten[2]) + (d * atten[1]) + (atten[0] - (1/c)) = 0
    //
    // Most of the lights used in BZFlag seem to be under 50, so
    // it doesn't seem to be worth the bother or CPU time to actually
    // use this equation. Grep for 'Attenuation' to find the values.
    //
    onlyReal = false;
    onlyGround = false;
    return;
}


OpenGLLight::OpenGLLight(const OpenGLLight& l)
{
    pos = l.pos;
    color = l.color;
    atten[0] = l.atten[0];
    atten[1] = l.atten[1];
    atten[2] = l.atten[2];
    maxDist = l.maxDist;
    onlyReal = l.onlyReal;
    onlyGround = l.onlyGround;
    return;
}


OpenGLLight& OpenGLLight::operator=(const OpenGLLight& l)
{
    if (this != &l)
    {
        pos = l.pos;
        color = l.color;
        atten[0] = l.atten[0];
        atten[1] = l.atten[1];
        atten[2] = l.atten[2];
        maxDist = l.maxDist;
        onlyReal = l.onlyReal;
        onlyGround = l.onlyGround;
    }
    return *this;
}


void OpenGLLight::setDirection(const glm::vec3 &_pos)
{
    pos = glm::vec4(_pos, 0.0f);
}


void OpenGLLight::setPosition(const glm::vec3 &_pos)
{
    pos = glm::vec4(_pos, 1.0f);
}


void OpenGLLight::setColor(GLfloat r, GLfloat g, GLfloat b)
{
    color = glm::vec4(r, g, b, 1.0f);
}


void OpenGLLight::setColor(const GLfloat* rgb)
{
    color = glm::vec4(rgb[0], rgb[1], rgb[2], 1.0f);
}


void OpenGLLight::setColor(const glm::vec3 &rgb)
{
    color = glm::vec4(rgb, 1.0f);
}


void OpenGLLight::setAttenuation(const GLfloat* _atten)
{
    atten[0] = _atten[0];
    atten[1] = _atten[1];
    atten[2] = _atten[2];
}


void OpenGLLight::setAttenuation(int index, GLfloat value)
{
    atten[index] = value;
}


void OpenGLLight::setOnlyReal(bool value)
{
    onlyReal = value;
    return;
}


void OpenGLLight::setOnlyGround(bool value)
{
    onlyGround = value;
    return;
}


void OpenGLLight::calculateImportance(const ViewFrustum& frustum)
{
    // for away lights count the most
    // (shouldn't happen for dynamic lights?)
    if (pos[3] == 0.0f)
    {
        importance = MAXFLOAT;
        return;
    }

    // This is not an exact test, the real culling shape should
    // be a frustum with extended in all directions by maxDist.
    // The hard edges on the frustum are bogus zones.

    // check if the light is in front of the front viewing plane
    bool sphereCull = true;
    const auto &p = frustum.getDirection();
    const float fd = glm::dot(p, pos);

    // cull against the frustum planes
    if (fd > 0.0f)
    {
        sphereCull = false; // don't need a sphere cull
        const int planeCount = frustum.getPlaneCount();
        for (int i = 1; i < planeCount; i++)
        {
            const auto &plane = frustum.getSide(i);
            const float len = glm::dot(plane, pos);
            if (len < -maxDist)
            {
                importance = -1.0f;
                return;
            }
        }
    }

    // calculate the distance
    const auto &eye = frustum.getEye();
    float dist = glm::distance(eye, glm::vec3(pos));

    // do a sphere cull if requested
    if (sphereCull && (dist > maxDist))
    {
        importance = -1.0f;
        return;
    }

    // compute the 'importance' factor
    if (dist == 0.0f)
        importance = 0.5f * MAXFLOAT;
    else
        importance = 1.0f / dist;

    return;
}


void OpenGLLight::enableLight(int index, bool on) // const
{
    if (on)
        glEnable((GLenum)(GL_LIGHT0 + index));
    else
        glDisable((GLenum)(GL_LIGHT0 + index));
    return;
}


void OpenGLLight::execute(int index) const
{
    {
        genLight((GLenum)(GL_LIGHT0 + index));
        return;
    }
}


void OpenGLLight::genLight(GLenum light) const
{
    glLightfv(light, GL_POSITION, glm::value_ptr(pos));
    glLightfv(light, GL_DIFFUSE, glm::value_ptr(color));
    glLightfv(light, GL_SPECULAR, glm::value_ptr(color));
    glLighti(light, GL_SPOT_EXPONENT, 0);
    glLightf(light, GL_CONSTANT_ATTENUATION, atten[0]);
    glLightf(light, GL_LINEAR_ATTENUATION, atten[1]);
    glLightf(light, GL_QUADRATIC_ATTENUATION, atten[2]);
    return;
}


GLint OpenGLLight::getMaxLights()
{
    if (maxLights == 0)
        glGetIntegerv(GL_MAX_LIGHTS, &maxLights);
    return maxLights;
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
