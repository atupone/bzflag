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

// interface header
#include "FlagSceneNode.h"

// system headers
#include <cstdlib>
#include <cmath>

// common implementation headers
#include "OpenGLGState.h"
#include "OpenGLMaterial.h"
#include "StateDatabase.h"
#include "BZDBCache.h"
#include "Vertex_Chunk.h"

// local implementation headers
#include "ViewFrustum.h"

// FIXME (SceneRenderer.cxx is in src/bzflag)
#include "SceneRenderer.h"

namespace
{
constexpr int maxChunks = 20;
constexpr int waveLists = 8;      // GL list count
int      flagChunks = 8;     // draw flag as 8 quads
bool     geoPole = false;    // draw the pole as quads
bool     realFlag = false;   // don't use billboarding
int      triCount = 0;       // number of rendered triangles

const GLfloat Unit = 0.8f;        // meters
const GLfloat Width = 1.5f * Unit;
const GLfloat Height = Unit;
}


/******************************************************************************/

//
// WaveGeometry  (local helper class)
//

class WaveGeometry
{
public:
    WaveGeometry();

    void refer()
    {
        refCount++;
    }
    void unrefer()
    {
        refCount--;
    }

    void waveFlag(float dt);

    void execute(bool shadow);

private:
    int refCount;
    float ripple1;
    float ripple2;

    Vertex_Chunk vboChunk;
    GLfloat verts[maxChunks * 2][3];
    GLfloat txcds[maxChunks * 2][2];
};


inline void WaveGeometry::execute(bool shadow)
{
    vboChunk.draw(GL_TRIANGLE_STRIP,
                  shadow,
                  (flagChunks + 1) * 2);
}


WaveGeometry::WaveGeometry()
    : refCount(0)
    , ripple1(2.0 * M_PI * bzfrand())
    , ripple2(2.0 * M_PI * bzfrand())
    , vboChunk(Vertex_Chunk::VT, maxChunks * 2)
{
}

void WaveGeometry::waveFlag(float dt)
{
    if (!refCount)
        return;

    // TODO: there are a lot of magic numbers here (x * M_PI) that have no
    // explanation. Some documentation would be useful
    constexpr auto RippleSpeed1 = float(2.4 * M_PI);
    constexpr auto RippleSpeed2 = float(1.724 * M_PI);
    constexpr auto TWO_PI       = float(2 * M_PI);


    ripple1 += dt * RippleSpeed1;
    if (ripple1 >= TWO_PI)
        ripple1 -= TWO_PI;
    ripple2 += dt * RippleSpeed2;
    if (ripple2 >= TWO_PI)
        ripple2 -= TWO_PI;
    float sinRipple2  = sinf(ripple2);
    float sinRipple2S = sinf((float)(ripple2 + 1.16 * M_PI));
    float wave0[maxChunks];
    float wave1[maxChunks];
    float wave2[maxChunks];
    for (auto i = 0; i <= flagChunks; i++)
    {
        const float x      = float(i) / float(flagChunks);
        const float damp   = 0.1f * x;
        const float angle1 = (float)(ripple1 - 4.0 * M_PI * x);
        const float angle2 = (float)(angle1 - 0.28 * M_PI);

        wave0[i] = damp * sinf(angle1);
        wave1[i] = damp * (sinf(angle2) + sinRipple2S);
        wave2[i] = wave0[i] + damp * sinRipple2;
    }
    float base = BZDBCache::flagPoleSize;
    for (auto i = 0; i <= flagChunks; i++)
    {
        const float x      = float(i) / float(flagChunks);
        const float shift1 = wave0[i];
        verts[i*2][0] = verts[i*2+1][0] = Width * x;
        if (realFlag)
        {
            // flag pole is Z axis
            verts[i*2][1] = wave1[i];
            verts[i*2+1][1] = wave2[i];
            verts[i*2][2] = base + Height - shift1;
            verts[i*2+1][2] = base - shift1;
        }
        else
        {
            // flag pole is Y axis
            verts[i*2][1] = base + Height - shift1;
            verts[i*2+1][1] = base - shift1;
            verts[i*2][2] = wave1[i];
            verts[i*2+1][2] = wave2[i];
        }
        txcds[i*2][0] = txcds[i*2+1][0] = x;
        txcds[i*2][1] = 1.0f;
        txcds[i*2+1][1] = 0.0f;
    }

    vboChunk.vertexData(verts);
    vboChunk.textureData(txcds);

    triCount = flagChunks * 2;

    return;
}


std::vector<WaveGeometry> allWaves;


/******************************************************************************/

//
// FlagSceneNode
//

FlagSceneNode::FlagSceneNode(const GLfloat pos[3]) :
    billboard(true),
    angle(0.0f),
    tilt(0.0f),
    hscl(1.0f),
    transparent(false),
    texturing(false),
    renderNode(this)
{
    setColor(1.0f, 1.0f, 1.0f, 1.0f);
    move(pos);
    setRadius(6.0f * Unit * Unit);
}

FlagSceneNode::~FlagSceneNode()
{
    // do nothing
}

void            FlagSceneNode::waveFlag(float dt)
{
    for (auto &wave : allWaves)
        wave.waveFlag(dt);
}

void            FlagSceneNode::freeFlag()
{
    allWaves.clear();
}

void            FlagSceneNode::move(const GLfloat pos[3])
{
    setCenter(glm::make_vec3(pos));
}


void            FlagSceneNode::setAngle(GLfloat _angle)
{
    angle = (float)(_angle * 180.0 / M_PI);
    tilt = 0.0f;
    hscl = 1.0f;
}


void            FlagSceneNode::setWind(const GLfloat wind[3], float dt)
{
    if (!realFlag)
    {
        angle = atan2f(wind[1], wind[0]) * (float)(180.0 / M_PI);
        tilt = 0.0f;
        hscl = 1.0f;
    }
    else
    {
        // the angle points from the end of the flag to the pole
        const float cos_val = cosf(angle * (float)(M_PI / 180.0f));
        const float sin_val = sinf(angle * (float)(M_PI / 180.0f));
        const float force = (wind[0] * sin_val) - (wind[1] * cos_val);
        const float angleScale = 25.0f;
        angle = fmodf(angle + (force * dt * angleScale), 360.0f);

        const float horiz = sqrtf((wind[0] * wind[0]) + (wind[1] * wind[1]));
        const float it = -0.75f; // idle tilt
        const float tf = +5.00f; // tilt factor
        const float desired = (wind[2] / (horiz + tf)) +
                              (it * (1.0f - horiz / (horiz + tf)));

        const float tt = dt * 5.0f;
        tilt = (tilt * (1.0f - tt)) + (desired * tt);

        const float maxTilt = 1.5f;
        if (tilt > +maxTilt)
            tilt = +maxTilt;
        else if (tilt < -maxTilt)
            tilt = -maxTilt;
        hscl = 1.0f / sqrtf(1.0f + (tilt * tilt));
    }
    return;
}


void            FlagSceneNode::setBillboard(bool _billboard)
{
    billboard = _billboard;
}

void            FlagSceneNode::setColor(
    GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    color[0] = r;
    color[1] = g;
    color[2] = b;
    color[3] = a;
    transparent = (color[3] != 1.0f);
}

void            FlagSceneNode::setColor(const GLfloat* rgba)
{
    color[0] = rgba[0];
    color[1] = rgba[1];
    color[2] = rgba[2];
    color[3] = rgba[3];
    transparent = (color[3] != 1.0f);
}

void            FlagSceneNode::setTexture(const int texture)
{
    OpenGLGStateBuilder builder(gstate);
    builder.setTexture(texture);
    builder.enableTexture(texture>=0);
    gstate = builder.getState();
}

void            FlagSceneNode::notifyStyleChange()
{
    const int quality = RENDERER.useQuality();
    geoPole = (quality >= 1);
    realFlag = (quality >= 3);

    texturing = BZDBCache::texture;
    OpenGLGStateBuilder builder(gstate);
    builder.enableTexture(texturing);

    if (transparent)
    {
        builder.setBlending(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        builder.setStipple(1.0f);
        builder.resetAlphaFunc();
    }
    else
    {
        builder.resetBlending();
        builder.setStipple(1.0f);
        if (texturing)
            builder.setAlphaFunc(GL_GEQUAL, 0.9f);
        else
            builder.resetAlphaFunc();
    }

    if (billboard && !realFlag)
        builder.setCulling(GL_BACK);
    else
        builder.disableCulling();
    gstate = builder.getState();

    flagChunks = BZDBCache::flagChunks;
    if (flagChunks >= maxChunks)
        flagChunks = maxChunks - 1;
}


void FlagSceneNode::addRenderNodes(SceneRenderer& renderer)
{
    renderer.addRenderNode(&renderNode, &gstate);
}


void FlagSceneNode::addShadowNodes(SceneRenderer& renderer)
{
    renderer.addShadowNode(&renderNode);
}


bool FlagSceneNode::cullShadow(const std::vector<glm::vec4> &planes) const
{
    const auto s = glm::vec4(getCenter(), 1.0f);
    const auto r = getRadius2();
    for (const auto &p : planes)
    {
        const float d = glm::dot(p, s);
        if ((d < 0.0f) && ((d * d) > r))
            return true;
    }
    return false;
}


/******************************************************************************/

//
// FlagSceneNode::FlagRenderNode
//

FlagSceneNode::FlagRenderNode::FlagRenderNode(
    const FlagSceneNode* _sceneNode) :
    sceneNode(_sceneNode)
{
    waveReference = (int)((double)waveLists * bzfrand());
    if (waveReference >= waveLists)
        waveReference = waveLists - 1;
    allWaves.resize(waveLists);
    allWaves[waveReference].refer();
}

FlagSceneNode::FlagRenderNode::~FlagRenderNode()
{
    allWaves[waveReference].unrefer();
}


void            FlagSceneNode::FlagRenderNode::renderShadow()
{
    render(true);
}

void            FlagSceneNode::FlagRenderNode::render()
{
    render(false);
}

void            FlagSceneNode::FlagRenderNode::render(bool shadow)
{
    float base = BZDBCache::flagPoleSize;
    float poleWidth = BZDBCache::flagPoleWidth;
    const bool doing_texturing = sceneNode->texturing;
    const bool is_billboard = sceneNode->billboard;

    const auto &sphere = sceneNode->getCenter();
    const float topHeight = base + Height;

    myColor4fv(sceneNode->color);

    glPushMatrix();
    {
        glTranslatef(sphere[0], sphere[1], sphere[2]);

        if (!is_billboard || realFlag)
            glRotatef(sceneNode->angle + 180.0f, 0.0f, 0.0f, 1.0f);

        // Flag drawing
        if (is_billboard)
        {
            // Wawing flag
            if (realFlag)
            {
                const float Tilt = sceneNode->tilt;
                const float Hscl = sceneNode->hscl;
                static GLfloat shear[16] = {Hscl, 0.0f, Tilt, 0.0f,
                                            0.0f, 1.0f, 0.0f, 0.0f,
                                            0.0f, 0.0f, 1.0f, 0.0f,
                                            0.0f, 0.0f, 0.0f, 1.0f
                                           };
                shear[0] = Hscl; // maintains the flag length
                shear[2] = Tilt; // pulls the flag up or down
                glPushMatrix();
                glMultMatrixf(shear);
            }
            else
                RENDERER.getViewFrustum().executeBillboard();

            allWaves[waveReference].execute(shadow);
            addTriangleCount(triCount);

            if (realFlag)
                glPopMatrix();
        }
        else
        {
            // Not wawing flag
            glBegin(GL_TRIANGLE_STRIP);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(0.0f, 0.0f, base);
            glTexCoord2f(1.0f, 0.0f);
            glVertex3f(Width, 0.0f, base);
            glTexCoord2f(0.0f, 1.0f);
            glVertex3f(0.0f, 0.0f, topHeight);
            glTexCoord2f(1.0f, 1.0f);
            glVertex3f(Width, 0.0f, topHeight);
            glEnd();
            addTriangleCount(2);
        }

        // Drawing the pole black untextured
        myColor4f(0.0f, 0.0f, 0.0f, sceneNode->color[3]);

        if (doing_texturing)
            glDisable(GL_TEXTURE_2D);

        if (is_billboard && realFlag)
        {
            glBegin(GL_TRIANGLE_STRIP);
            {
                glVertex3f(-poleWidth, 0.0f, 0.0f);
                glVertex3f(-poleWidth, 0.0f, topHeight);
                glVertex3f(0.0f, -poleWidth, 0.0f);
                glVertex3f(0.0f, -poleWidth, topHeight);
                glVertex3f(+poleWidth, 0.0f, 0.0f);
                glVertex3f(+poleWidth, 0.0f, topHeight);
                glVertex3f(0.0f, +poleWidth, 0.0f);
                glVertex3f(0.0f, +poleWidth, topHeight);
                glVertex3f(-poleWidth, 0.0f, 0.0f);
                glVertex3f(-poleWidth, 0.0f, topHeight);
            }
            glEnd();
            addTriangleCount(8);
        }
        else if (geoPole)
        {
            if (is_billboard)
                glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

            glBegin(GL_TRIANGLE_STRIP);
            {
                glVertex3f(-poleWidth, 0.0f, 0.0f);
                glVertex3f(+poleWidth, 0.0f, 0.0f);
                glVertex3f(-poleWidth, 0.0f, topHeight);
                glVertex3f(+poleWidth, 0.0f, topHeight);
            }
            glEnd();
            addTriangleCount(2);
        }
        else
        {
            if (is_billboard)
                glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

            glBegin(GL_LINE_STRIP);
            {
                glVertex3f(0.0f, 0.0f, 0.0f);
                glVertex3f(0.0f, 0.0f, topHeight);
            }
            glEnd();
            addTriangleCount(1);
        }

        if (doing_texturing)
            glEnable(GL_TEXTURE_2D);
    }
    glPopMatrix();
}

const glm::vec3 FlagSceneNode::FlagRenderNode::getPosition() const
{
    return sceneNode->getCenter();
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
