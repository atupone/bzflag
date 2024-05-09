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

// bzflag common headers
#include "common.h"
#include "global.h"

// interface header
#include "TankGeometryMgr.h"

// system headers
#include <stdlib.h>
#include <math.h>
#include <string>

// common implementation headers
#include "SceneRenderer.h"
#include "StateDatabase.h"
#include "BZDBCache.h"
#include "OpenGLGState.h"
#include "Vertex_Chunk.h"


// use the namespaces
using namespace TankGeometryMgr;
using namespace TankGeometryEnums;
using namespace TankGeometryUtils;


// Local Variables
// ---------------

// the display lists
static Vertex_Chunk displayLists
[LastTankSize][LastTankPart];

static Vertex_Chunk lightsOnTank;

// triangle counts
static int partTriangles
[LastTankSize][LastTankPart];

// the scaling factors
static glm::vec3 scaleFactors[LastTankSize] =
{
    {1.0f, 1.0f, 1.0f},   // Normal
    {1.0f, 1.0f, 1.0f},   // Obese
    {1.0f, 1.0f, 1.0f},   // Tiny
    {1.0f, 0.001f, 1.0f}, // Narrow
    {1.0f, 1.0f, 1.0f}    // Thief
};
// the current scaling factors
static auto currentScaleFactor = scaleFactors[Normal];

// arrays of functions to avoid large switch statements
typedef int (*partFunction)(void);
static const partFunction partFunctions[BasicTankParts] =
{
    buildHighBody,
    buildHighBarrel,
    buildHighTurret,
    buildHighLCasing,
    buildHighRCasing
};


// Local Function Prototypes
// -------------------------

static void setupScales();
static void bzdbCallback(const std::string& str, void *data);


/****************************************************************************/

// TankGeometryMgr Functions
// -------------------------


void TankGeometryMgr::init()
{
    // initialize the lists to invalid
    {
        {
            for (int size = 0; size < LastTankSize; size++)
            {
                for (int part = 0; part < LastTankPart; part++)
                    partTriangles[size][part] = 0;
            }
        }
    }

    // install the BZDB callbacks
    // This MUST be done after BZDB has been initialized in main()
    BZDB.addCallback (StateDatabase::BZDB_OBESEFACTOR, bzdbCallback, NULL);
    BZDB.addCallback (StateDatabase::BZDB_TINYFACTOR, bzdbCallback, NULL);
    BZDB.addCallback (StateDatabase::BZDB_THIEFTINYFACTOR, bzdbCallback, NULL);
    BZDB.addCallback ("animatedTreads", bzdbCallback, NULL);

    // setup the scaleFactors
    setupScales();
    buildLists();

    return;
}


void TankGeometryMgr::kill()
{
    // remove the BZDB callbacks
    BZDB.removeCallback (StateDatabase::BZDB_OBESEFACTOR, bzdbCallback, NULL);
    BZDB.removeCallback (StateDatabase::BZDB_TINYFACTOR, bzdbCallback, NULL);
    BZDB.removeCallback (StateDatabase::BZDB_THIEFTINYFACTOR, bzdbCallback, NULL);
    BZDB.removeCallback ("animatedTreads", bzdbCallback, NULL);

    deleteLists();

    return;
}


void TankGeometryMgr::deleteLists()
{
    // delete the lists that have been aquired
    {
        {
            for (int size = 0; size < LastTankSize; size++)
            {
                for (int part = 0; part < LastTankPart; part++)
                    displayLists[size][part] = Vertex_Chunk();
            }
        }
    }
    lightsOnTank = Vertex_Chunk();
    return;
}


static std::vector<glm::vec2> tankTextur;
static std::vector<glm::vec3> tankNormal;
static std::vector<glm::vec3> tankVertex;

void TankGeometryMgr::buildLists()
{
    // setup the tread style
    setTreadStyle(BZDB.evalInt("treadStyle"));

    // setup the scale factors
    setupScales();
    currentScaleFactor = scaleFactors[Normal];
    const bool animated = BZDBCache::animatedTreads;

    // setup the quality level
    const int divisionLevels[4][2] =   // wheel divs, tread divs
    {
        {4, 4},   // low
        {8, 16},  // med
        {12, 24}, // high
        {16, 32}  // experimental
    };
    int quality = RENDERER.useQuality();
    if (quality < 0)
        quality = 0;
    else if (quality > 3)
        quality = 3;
    int wheelDivs = divisionLevels[quality][0];
    int treadDivs = divisionLevels[quality][1];

    {
        {
            for (int size = 0; size < LastTankSize; size++)
            {

                // only do the basics, unless we're making an animated tank
                int lastPart = BasicTankParts;
                if (animated)
                    lastPart = HighTankParts;

                for (int part = 0; part < lastPart; part++)
                {

                    auto &list = displayLists[size][part];
                    int &count = partTriangles[size][part];

                    tankTextur.clear();
                    tankNormal.clear();
                    tankVertex.clear();

                    // setup the scale factor
                    currentScaleFactor = scaleFactors[size];

                    if ((part <= Turret) || (!animated))
                    {
                        // the basic parts
                        count = partFunctions[part]();
                    }
                    else
                    {
                        // the animated parts
                        if (part == LeftCasing)
                            count = buildHighLCasingAnim();
                        else if (part == RightCasing)
                            count = buildHighRCasingAnim();
                        else if (part == LeftTread)
                            count = buildHighLTread(treadDivs);
                        else if (part == RightTread)
                            count = buildHighRTread(treadDivs);
                        else if ((part >= LeftWheel0) && (part <= LeftWheel3))
                        {
                            int wheel = part - LeftWheel0;
                            count = buildHighLWheel(wheel, (float)wheel * (float)(M_PI / 2.0),
                                                    wheelDivs);
                        }
                        else if ((part >= RightWheel0) && (part <= RightWheel3))
                        {
                            int wheel = part - RightWheel0;
                            count = buildHighRWheel(wheel, (float)wheel * (float)(M_PI / 2.0),
                                                    wheelDivs);
                        }
                    }

                    list = Vertex_Chunk(Vertex_Chunk::VTN, tankVertex.size());
                    list.textureData(tankTextur);
                    list.normalData(tankNormal);
                    list.vertexData(tankVertex);

                } // part
            } // size
        } // lod
    }

    {
        lightsOnTank = Vertex_Chunk(Vertex_Chunk::VC, 3);

        const glm::vec4 colors[3] =
        {
            { 1.0f, 1.0f, 1.0f, 1.0f },
            { 1.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 1.0f, 0.0f, 1.0f }
        };

        const glm::vec3 vertex[3] =
        {
            { -1.53f,  0.00f, 2.1f },
            {  0.10f,  0.75f, 2.1f },
            {  0.10f, -0.75f, 2.1f }
        };

        lightsOnTank.colorData(colors);
        lightsOnTank.vertexData(vertex);

    }

    return;
}


int TankGeometryMgr::drawPart(bool isShadow,
                              TankGeometryEnums::TankPart part,
                              TankGeometryEnums::TankSize size)
{
    // get the list
    displayLists[size][part].draw(GL_TRIANGLE_STRIP, isShadow);
    int count = partTriangles[size][part];
    return count;
}


void TankGeometryMgr::drawLightsOnTank(bool colorOverride)
{
    lightsOnTank.draw(GL_POINTS, colorOverride);
}


const glm::vec3 &TankGeometryMgr::getScaleFactor(TankSize size)
{
    return scaleFactors[size];
}


/****************************************************************************/

// Local Functions
// ---------------


static void bzdbCallback(const std::string& UNUSED(name), void * UNUSED(data))
{
    buildLists();
    return;
}



static void setupScales()
{
    float scale;

    scaleFactors[Normal][0] = BZDBCache::tankLength;
    scale = (float)atof(BZDB.getDefault(StateDatabase::BZDB_TANKLENGTH).c_str());
    scaleFactors[Normal][0] /= scale;

    scaleFactors[Normal][1] = BZDBCache::tankWidth;
    scale = (float)atof(BZDB.getDefault(StateDatabase::BZDB_TANKWIDTH).c_str());
    scaleFactors[Normal][1] /= scale;

    scaleFactors[Normal][2] = BZDBCache::tankHeight;
    scale = (float)atof(BZDB.getDefault(StateDatabase::BZDB_TANKHEIGHT).c_str());
    scaleFactors[Normal][2] /= scale;

    scale = BZDB.eval(StateDatabase::BZDB_OBESEFACTOR);
    scaleFactors[Obese][0] = scale * scaleFactors[Normal][0];
    scaleFactors[Obese][1] = scale * scaleFactors[Normal][1];
    scaleFactors[Obese][2] = scaleFactors[Normal][2];

    scale = BZDB.eval(StateDatabase::BZDB_TINYFACTOR);
    scaleFactors[Tiny][0] = scale * scaleFactors[Normal][0];
    scaleFactors[Tiny][1] = scale * scaleFactors[Normal][1];
    scaleFactors[Tiny][2] = scaleFactors[Normal][2];

    scale = BZDB.eval(StateDatabase::BZDB_THIEFTINYFACTOR);
    scaleFactors[Thief][0] = scale * scaleFactors[Normal][0];
    scaleFactors[Thief][1] = scale * scaleFactors[Normal][1];
    scaleFactors[Thief][2] = scaleFactors[Normal][2];

    scaleFactors[Narrow][0] = scaleFactors[Normal][0];
    scaleFactors[Narrow][1] = 0.001f;
    scaleFactors[Narrow][2] = scaleFactors[Normal][2];

    return;
}


/****************************************************************************/

// TankGeometryUtils Functions
// ---------------------------


static glm::vec3 normal;
static glm::vec2 textur;

void TankGeometryUtils::doVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
    const auto &scale = currentScaleFactor;
    const auto xyz = glm::vec3(x, y, z) * scale;
    tankNormal.push_back(normal);
    tankTextur.push_back(textur);
    tankVertex.push_back(xyz);
    return;
}


void TankGeometryUtils::doNormal3f(GLfloat x, GLfloat y, GLfloat z)
{
    const auto &scale = currentScaleFactor;
    auto sxyz = glm::vec3(x, y, z) * scale;
    if (sxyz.x || sxyz.y || sxyz.z)
        sxyz = glm::normalize(sxyz);
    normal = sxyz;
    return;
}


void TankGeometryUtils::doTexCoord2f(GLfloat x, GLfloat y)
{
    textur = glm::vec2(x, y);
    return;
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
