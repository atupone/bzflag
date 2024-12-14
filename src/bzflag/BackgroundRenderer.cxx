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
#include "BackgroundRenderer.h"

// system headers
#include <string.h>
#include <algorithm>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>

// common headers
#include "OpenGLMaterial.h"
#include "TextureManager.h"
#include "BZDBCache.h"
#include "BzMaterial.h"
#include "TextureMatrix.h"
#include "ParseColor.h"
#include "BZDBCache.h"
#include "OpenGLAPI.h"
#include "VBO_Drawing.h"

// local headers
#include "daylight.h"
#include "stars.h"
#include "MainWindow.h"
#include "SceneNode.h"
#include "effectsRenderer.h"
#include "World.h"

static const glm::vec2 squareShape[4] =
{
    {  1.0f,  1.0f }, { -1.0f,  1.0f },
    { -1.0f, -1.0f }, {  1.0f, -1.0f }
};


glm::vec3 BackgroundRenderer::skyPyramid[5];
const GLfloat       BackgroundRenderer::cloudRepeats = 3.0f;
static const int    NumMountainFaces = 16;

glm::vec4       BackgroundRenderer::groundColor[4];
glm::vec4       BackgroundRenderer::groundColorInv[4];

const glm::vec4 BackgroundRenderer::defaultGroundColor[4] =
{
    { 0.0f, 0.35f, 0.0f, 1.0f },
    { 0.0f, 0.20f, 0.0f, 1.0f },
    { 1.0f, 1.00f, 1.0f, 1.0f },
    { 1.0f, 1.00f, 1.0f, 1.0f }
};
const glm::vec4 BackgroundRenderer::defaultGroundColorInv[4] =
{
    { 0.35f, 0.00f, 0.35f, 1.0f },
    { 0.20f, 0.00f, 0.20f, 1.0f },
    { 1.00f, 1.00f, 1.00f, 1.0f },
    { 1.00f, 1.00f, 1.00f, 1.0f }
};

BackgroundRenderer::BackgroundRenderer() :
    blank(false),
    invert(false),
    gridSpacing(60.0f), // meters
    gridCount(4.0f),
    mountainsAvailable(false),
    numMountainTextures(0),
    mountainsGState(NULL),
    mountainsList(NULL),
    cloudDriftU(0.0f),
    cloudDriftV(0.0f)
{
    static bool init = false;
    OpenGLGStateBuilder gstate;
    static const auto black = glm::vec3(0.0f);
    static const auto white = glm::vec3(1.0f);
    OpenGLMaterial defaultMaterial(black, black, 0.0f);
    OpenGLMaterial rainMaterial(white, white, 0.0f);

    sunList = INVALID_GL_LIST_ID;
    moonList = INVALID_GL_LIST_ID;
    starList = INVALID_GL_LIST_ID;
    cloudsList = INVALID_GL_LIST_ID;
    sunXFormList = INVALID_GL_LIST_ID;
    starXFormList = INVALID_GL_LIST_ID;

    // initialize global to class stuff
    if (!init)
    {
        init = true;
        resizeSky();
    }

    // initialize the celestial vectors
    static const auto up = glm::vec3(0.0f, 0.0f, 1.0f);
    sunDirection  = up;
    moonDirection = up;

    // make ground materials
    setupSkybox();
    setupGroundMaterials();

    TextureManager &tm = TextureManager::instance();

    // make grid stuff
    gstate.reset();
    gstate.setBlending();
    gstate.setSmoothing();
    gridGState = gstate.getState();

    // make receiver stuff
    gstate.reset();
    gstate.setShading();
    gstate.setBlending((GLenum)GL_SRC_ALPHA, (GLenum)GL_ONE);
    receiverGState = gstate.getState();

    // sun shadow stuff
    gstate.reset();
    gstate.setStipple(0.5f);
    gstate.disableCulling();
    sunShadowsGState = gstate.getState();

    /* useMoonTexture = BZDBCache::texture && (BZDB.eval("useQuality")>2);
     int moonTexture = -1;
     if (useMoonTexture) {
       moonTexture = tm.getTextureID( "moon" );
       useMoonTexture = moonTexture>= 0;
     }*/
    // sky stuff
    gstate.reset();
    gstate.setShading();
    skyGState = gstate.getState();
    gstate.reset();
    sunGState = gstate.getState();
    gstate.reset();
    gstate.setBlending((GLenum)GL_ONE, (GLenum)GL_ONE);
// if (useMoonTexture)
//   gstate.setTexture(*moonTexture);
    moonGState[0] = gstate.getState();
    gstate.reset();
// if (useMoonTexture)
//   gstate.setTexture(*moonTexture);
    moonGState[1] = gstate.getState();
    gstate.reset();
    starGState[0] = gstate.getState();
    gstate.reset();
    gstate.setBlending();
    gstate.setSmoothing();
    starGState[1] = gstate.getState();

    // make cloud stuff
    cloudsAvailable = false;
    int cloudsTexture = tm.getTextureID( "clouds" );
    if (cloudsTexture >= 0)
    {
        cloudsAvailable = true;
        gstate.reset();
        gstate.setShading();
        gstate.setBlending((GLenum)GL_SRC_ALPHA, (GLenum)GL_ONE_MINUS_SRC_ALPHA);
        gstate.setMaterial(defaultMaterial);
        gstate.setTexture(cloudsTexture);
        gstate.setAlphaFunc();
        cloudsGState = gstate.getState();
    }

    // rain stuff
    weather.init();
    // effects
    EFFECTS.init();

    // make mountain stuff
    mountainsAvailable = false;
    {
        int mountainTexture;
        int height = 0;
        int i;

        numMountainTextures = 0;
        while (1)
        {
            char text[256];
            sprintf (text, "mountain%d", numMountainTextures + 1);
            mountainTexture = tm.getTextureID (text, false);
            if (mountainTexture < 0)
                break;
            const ImageInfo & info = tm.getInfo (mountainTexture);
            height = info.y;
            numMountainTextures++;
        }

        if (numMountainTextures > 0)
        {
            mountainsAvailable = true;

            // prepare common gstate
            gstate.reset ();
            gstate.setShading ();
            gstate.setBlending ();
            gstate.setMaterial (defaultMaterial);
            gstate.setAlphaFunc ();

            // find power of two at least as large as height
            int scaledHeight = 1;
            while (scaledHeight < height)
                scaledHeight <<= 1;

            // choose minimum width
            int minWidth = scaledHeight;
            if (minWidth > scaledHeight)
                minWidth = scaledHeight;
            mountainsMinWidth = minWidth;

            // prepare each texture
            mountainsGState = new OpenGLGState[numMountainTextures];
            mountainsList = new GLuint[numMountainTextures];
            for (i = 0; i < numMountainTextures; i++)
            {
                char text[256];
                sprintf (text, "mountain%d", i + 1);
                gstate.setTexture (tm.getTextureID (text));
                mountainsGState[i] = gstate.getState ();
                mountainsList[i] = INVALID_GL_LIST_ID;
            }
        }
    }

    // create display lists
    doInitDisplayLists();

    // reset the sky color when it changes
    BZDB.addCallback("_skyColor", bzdbCallback, this);

    // recreate display lists when context is recreated
    OpenGLGState::registerContextInitializer(freeContext, initContext,
            (void*)this);

    notifyStyleChange();
}

BackgroundRenderer::~BackgroundRenderer()
{
    BZDB.removeCallback("_skyColor", bzdbCallback, this);
    OpenGLGState::unregisterContextInitializer(freeContext, initContext,
            (void*)this);
    delete[] mountainsGState;
    delete[] mountainsList;
}


void BackgroundRenderer::bzdbCallback(const std::string& name, void* data)
{
    BackgroundRenderer* br = (BackgroundRenderer*) data;
    if (name == "_skyColor")
        br->setSkyColors();
    return;
}


void BackgroundRenderer::setupGroundMaterials()
{
    TextureManager &tm = TextureManager::instance();

    // see if we have a map specified material
    const BzMaterial* bzmat = MATERIALMGR.findMaterial("GroundMaterial");

    groundTextureID = -1;
    groundTextureMatrix = NULL;

    if (bzmat == NULL)
    {
        // default ground material
        for (int i = 0; i < 4; i++)
            groundColor[i] = defaultGroundColor[i];
        groundTextureID = tm.getTextureID(BZDB.get("stdGroundTexture").c_str(), true);
    }
    else
    {
        // map specified material
        bzmat->setReference();
        for (int i = 0; i < 4; i++)
            groundColor[i] = bzmat->getDiffuse();
        if (bzmat->getTextureCount() > 0)
        {
            groundTextureID = tm.getTextureID(bzmat->getTextureLocal(0).c_str(), false);
            if (groundTextureID < 0)
            {
                // use the default as a backup (default color too)
                for (int i = 0; i < 4; i++)
                    groundColor[i] = defaultGroundColor[i];
                groundTextureID = tm.getTextureID(BZDB.get("stdGroundTexture").c_str(), true);
            }
            else
            {
                // only apply the texture matrix if the texture is valid
                const int texMatId = bzmat->getTextureMatrix(0);
                const TextureMatrix* texmat = TEXMATRIXMGR.getMatrix(texMatId);
                if (texmat != NULL)
                    groundTextureMatrix = texmat->getMatrix();
            }
        }
    }

    static const auto black = glm::vec3(0.0f);
    OpenGLMaterial defaultMaterial(black, black, 0.0f);

    OpenGLGStateBuilder gb;

    // ground gstates
    gb.reset();
    groundGState[0] = gb.getState();
    gb.reset();
    gb.setMaterial(defaultMaterial);
    groundGState[1] = gb.getState();
    gb.reset();
    gb.setTexture(groundTextureID);
    gb.setTextureMatrix(groundTextureMatrix);
    groundGState[2] = gb.getState();
    gb.reset();
    gb.setMaterial(defaultMaterial);
    gb.setTexture(groundTextureID);
    gb.setTextureMatrix(groundTextureMatrix);
    groundGState[3] = gb.getState();


    // default inverted ground material
    int groundInvTextureID = -1;
    for (int i = 0; i < 4; i++)
        groundColorInv[i] = defaultGroundColorInv[i];
    if (groundInvTextureID < 0)
        groundInvTextureID = tm.getTextureID(BZDB.get("zoneGroundTexture").c_str(), false);

    // inverted ground gstates
    gb.reset();
    invGroundGState[0] = gb.getState();
    gb.reset();
    gb.setMaterial(defaultMaterial);
    invGroundGState[1] = gb.getState();
    gb.reset();
    gb.setTexture(groundInvTextureID);
    invGroundGState[2] = gb.getState();
    gb.reset();
    gb.setMaterial(defaultMaterial);
    gb.setTexture(groundInvTextureID);
    invGroundGState[3] = gb.getState();

    return;
}


void BackgroundRenderer::notifyStyleChange()
{
    if (BZDBCache::texture)
    {
        if (BZDBCache::lighting)
            styleIndex = 3;
        else
            styleIndex = 2;
    }
    else
    {
        if (BZDBCache::lighting)
            styleIndex = 1;
        else
            styleIndex = 0;
    }

    // some stuff is drawn only for certain states
    cloudsVisible = (styleIndex >= 2 && cloudsAvailable);
    mountainsVisible = (styleIndex >= 2 && mountainsAvailable);
    shadowsVisible = BZDB.isTrue("shadows");
    starGStateIndex = BZDB.isTrue("smooth");

    // fixup gstates
    OpenGLGStateBuilder gstate;
    gstate.reset();
    if (BZDB.isTrue("smooth"))
    {
        gstate.setBlending();
        gstate.setSmoothing();
    }
    gridGState = gstate.getState();
}


void BackgroundRenderer::resize()
{
    resizeSky();
    doFreeDisplayLists();
    doInitDisplayLists();
}


void BackgroundRenderer::setCelestial(const SceneRenderer& renderer,
                                      const glm::vec3 &sunDir,
                                      const glm::vec3 &moonDir)
{
    // set sun and moon positions
    sunDirection  = sunDir;
    moonDirection = moonDir;

    if (sunXFormList != INVALID_GL_LIST_ID)
    {
        glDeleteLists(sunXFormList, 1);
        sunXFormList = INVALID_GL_LIST_ID;
    }
    if (moonList != INVALID_GL_LIST_ID)
    {
        glDeleteLists(moonList, 1);
        moonList = INVALID_GL_LIST_ID;
    }
    if (starXFormList != INVALID_GL_LIST_ID)
    {
        glDeleteLists(starXFormList, 1);
        starXFormList = INVALID_GL_LIST_ID;
    }

    makeCelestialLists(renderer);

    return;
}


void BackgroundRenderer::setSkyColors()
{
    // change sky colors according to the sun position
    glm::vec3 colors[4];
    getSkyColor(sunDirection, colors);

    skyZenithColor      = colors[0];
    skySunDirColor      = colors[1];
    skyAntiSunDirColor  = colors[2];
    skyCrossSunDirColor = colors[3];

    return;
}


void BackgroundRenderer::makeCelestialLists(const SceneRenderer& renderer)
{
    setSkyColors();

    // get a few other things concerning the sky
    doShadows = areShadowsCast(sunDirection);
    doStars = areStarsVisible(sunDirection);
    doSunset = getSunsetTop(sunDirection, sunsetTop);

    // make pretransformed display list for sun
    sunXFormList = glGenLists(1);
    glNewList(sunXFormList, GL_COMPILE);
    {
        glPushMatrix();
        glRotatef((GLfloat)(atan2f(sunDirection[1], (sunDirection[0])) * 180.0 / M_PI),
                  0.0f, 0.0f, 1.0f);
        glRotatef((GLfloat)(asinf(sunDirection[2]) * 180.0 / M_PI), 0.0f, -1.0f, 0.0f);
        glCallList(sunList);
        glPopMatrix();
    }
    glEndList();

    // compute display list for moon
    float coverage = glm::dot(moonDirection, sunDirection);
    // hack coverage to lean towards full
    coverage = (coverage < 0.0f) ? -sqrtf(-coverage) : coverage * coverage;
    float worldSize = BZDBCache::worldSize;
    const float moonRadius = 2.0f * worldSize *
                             atanf((float)((60.0 * M_PI / 180.0) / 60.0));
    // limbAngle is dependent on moon position but sun is so much farther
    // away that the moon's position is negligible.  rotate sun and moon
    // so that moon is on the horizon in the +x direction, then compute
    // the angle to the sun position in the yz plane.
    float sun2[3];
    const float moonAzimuth = atan2f(moonDirection[1], moonDirection[0]);
    const float moonAltitude = asinf(moonDirection[2]);
    sun2[0] = sunDirection[0] * cosf(moonAzimuth) + sunDirection[1] * sinf(moonAzimuth);
    sun2[1] = sunDirection[1] * cosf(moonAzimuth) - sunDirection[0] * sinf(moonAzimuth);
    sun2[2] = sunDirection[2] * cosf(moonAltitude) - sun2[0] * sinf(moonAltitude);
    const float limbAngle = atan2f(sun2[2], sun2[1]);

    const int moonSegements = 64;
    moonList = glGenLists(1);
    glNewList(moonList, GL_COMPILE);
    {
        glPushMatrix();
        glRotatef((GLfloat)(atan2f(moonDirection[1], moonDirection[0]) * 180.0 / M_PI),
                  0.0f, 0.0f, 1.0f);
        glRotatef((GLfloat)(asinf(moonDirection[2]) * 180.0 / M_PI), 0.0f, -1.0f, 0.0f);
        glRotatef((float)(limbAngle * 180.0 / M_PI), 1.0f, 0.0f, 0.0f);
        glBegin(GL_TRIANGLE_STRIP);
        // glTexCoord2f(0,-1);
        glVertex3f(2.0f * worldSize, 0.0f, -moonRadius);
        for (int i = 0; i < moonSegements-1; i++)
        {
            const float angle = (float)(0.5 * M_PI * double(i-(moonSegements/2)-1) / (moonSegements/2.0));
            float sinAngle = sinf(angle);
            float cosAngle = cosf(angle);
            // glTexCoord2f(coverage*cosAngle,sinAngle);
            glVertex3f(2.0f * worldSize, coverage * moonRadius * cosAngle,moonRadius * sinAngle);

            // glTexCoord2f(cosAngle,sinAngle);
            glVertex3f(2.0f * worldSize, moonRadius * cosAngle,moonRadius * sinAngle);
        }
        // glTexCoord2f(0,1);
        glVertex3f(2.0f * worldSize, 0.0f, moonRadius);
        glEnd();
        glPopMatrix();
    }
    glEndList();

    // make pretransformed display list for stars
    starXFormList = glGenLists(1);
    glNewList(starXFormList, GL_COMPILE);
    {
        glPushMatrix();
        glMultMatrixf(renderer.getCelestialTransform());
        glScalef(worldSize, worldSize, worldSize);
        glCallList(starList);
        glPopMatrix();
    }
    glEndList();

    return;
}


void BackgroundRenderer::addCloudDrift(GLfloat uDrift, GLfloat vDrift)
{
    cloudDriftU += 0.01f * uDrift / cloudRepeats;
    cloudDriftV += 0.01f * vDrift / cloudRepeats;
    if (cloudDriftU > 1.0f) cloudDriftU -= 1.0f;
    else if (cloudDriftU < 0.0f) cloudDriftU += 1.0f;
    if (cloudDriftV > 1.0f) cloudDriftV -= 1.0f;
    else if (cloudDriftV < 0.0f) cloudDriftV += 1.0f;
}


void BackgroundRenderer::renderSky(SceneRenderer& renderer,
                                   bool mirror)
{
    if (!BZDBCache::drawSky)
        return;
    {
        drawSky(renderer, mirror);
    }
}


void BackgroundRenderer::renderGround()
{
    {
        drawGround();
    }
}


void BackgroundRenderer::renderGroundEffects(SceneRenderer& renderer,
        bool drawingMirror)
{
    // zbuffer should be disabled.  either everything is coplanar with
    // the ground or is drawn back to front and is occluded by everything
    // drawn after it.  also use projection with very far clipping plane.

    // only draw the grid lines if texturing is disabled
    if (!BZDBCache::texture)
        drawGroundGrid(renderer);

    if (!blank)
    {
        const World *_world = World::getWorld();
        if (doShadows && shadowsVisible && !drawingMirror && _world)
            drawGroundShadows(renderer);

        // draw light receivers on ground (little meshes under light sources so
        // the ground gets illuminated).  this is necessary because lighting is
        // performed only at a vertex, and the ground's vertices are a few
        // kilometers away.
        if (BZDBCache::lighting &&
                !drawingMirror && BZDBCache::drawGroundLights)
        {
            if (BZDBCache::tessellation && (renderer.useQuality() >= 3))
            {
//    (BZDB.get(StateDatabase::BZDB_FOGMODE) == "none")) {
                // not really tessellation, but it is tied to the "Best" lighting,
                // avoid on foggy maps, because the blending function accumulates
                // too much brightness.
                drawAdvancedGroundReceivers(renderer);
            }
            else
                drawGroundReceivers(renderer);
        }

        {
            // light the mountains (so that they get dark when the sun goes down).
            // don't do zbuffer test since they occlude all drawn before them and
            // are occluded by all drawn after.
            if (mountainsVisible && BZDBCache::drawMountains)
                drawMountains();

            // draw clouds
            if (cloudsVisible && BZDBCache::drawClouds)
            {
                cloudsGState.setState();
                glMatrixMode(GL_TEXTURE);
                glPushMatrix();
                glTranslatef(cloudDriftU, cloudDriftV, 0.0f);
                glCallList(cloudsList);
                glLoadIdentity();   // maybe works around bug in some systems
                glPopMatrix();
                glMatrixMode(GL_MODELVIEW);
            }
        }
    }
}


void BackgroundRenderer::renderEnvironment(SceneRenderer& renderer, bool update)
{
    if (blank)
        return;

    if (update)
        weather.update();
    weather.draw(renderer);

    if (update)
        EFFECTS.update();
    EFFECTS.draw(renderer);
}


void BackgroundRenderer::resizeSky()
{
    // sky pyramid must fit inside far clipping plane
    // (adjusted for the deepProjection matrix)
    const GLfloat skySize = 3.0f * BZDBCache::worldSize;
    for (int i = 0; i < 4; i++)
        skyPyramid[i] = glm::vec3(skySize * squareShape[i], 0.0f);
    skyPyramid[4] = glm::vec3(0.0f, 0.0f, skySize);
}


void BackgroundRenderer::setupSkybox()
{
    haveSkybox = false;

    int i;
    const char *skyboxNames[6] =
    {
        "LeftSkyboxMaterial",
        "FrontSkyboxMaterial",
        "RightSkyboxMaterial",
        "BackSkyboxMaterial",
        "TopSkyboxMaterial",
        "BottomSkyboxMaterial"
    };
    TextureManager& tm = TextureManager::instance();
    const BzMaterial* bzmats[6] = {NULL, NULL, NULL, NULL, NULL, NULL};

    // try to load the textures
    for (i = 0; i < 6; i++)
    {
        bzmats[i] = MATERIALMGR.findMaterial(skyboxNames[i]);
        if ((bzmats[i] == NULL) || (bzmats[i]->getTextureCount() <= 0))
            break;
        skyboxTexID[i] = tm.getTextureID(bzmats[i]->getTextureLocal(0).c_str());
        if (skyboxTexID[i] < 0)
            break;
    }

    // unload textures if they were not all successful
    if (i != 6)
    {
        while (i >= 0)
        {
            if ((bzmats[i] != NULL) && (bzmats[i]->getTextureCount() > 0))
            {
                // NOTE: this could delete textures the might be used elsewhere
                tm.removeTexture(bzmats[i]->getTextureLocal(0).c_str());
            }
            i--;
        }
        return;
    }

    // reference map specified materials
    for (i = 0; i < 6; i++)
        bzmats[i]->setReference();

    // setup the corner colors
    const int cornerFaces[8][3] =
    {
        {5, 0, 1}, {5, 1, 2}, {5, 2, 3}, {5, 3, 0},
        {4, 0, 1}, {4, 1, 2}, {4, 2, 3}, {4, 3, 0}
    };
    for (i = 0; i < 8; i++)
    {
        skyboxColor[i] = glm::vec3(0.0f);
        for (int f = 0; f < 3; f++)
            skyboxColor[i] += glm::vec3(bzmats[cornerFaces[i][f]]->getDiffuse());
        skyboxColor[i] /= 3.0f;
    }

    haveSkybox = true;

    return;
}

void BackgroundRenderer::drawSkybox()
{
    // sky box must fit inside far clipping plane
    // (adjusted for the deepProjection matrix)
    const float d = 3.0f * BZDBCache::worldSize;
    const glm::vec3 verts[8] =
    {
        {-d, -d, -d}, {+d, -d, -d}, {+d, +d, -d}, {-d, +d, -d},
        {-d, -d, +d}, {+d, -d, +d}, {+d, +d, +d}, {-d, +d, +d}
    };
    const glm::vec2 txcds[4] =
    {
        {1.0f, 0.0f}, {0.0f, 0.0f},
        {0.0f, 1.0f}, {1.0f, 1.0f}
    };

    TextureManager& tm = TextureManager::instance();

    OpenGLGState::resetState();

    const auto &color = skyboxColor;

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);

    if (!BZDBCache::drawGround)
    {
        tm.bind(skyboxTexID[5]); // bottom
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBegin(GL_TRIANGLE_STRIP);
        {
            glTexCoord(txcds[0]);
            glColor(color[2]);
            glVertex(verts[2]);
            glTexCoord(txcds[1]);
            glColor(color[3]);
            glVertex(verts[3]);
            glTexCoord(txcds[3]);
            glColor(color[1]);
            glVertex(verts[1]);
            glTexCoord(txcds[2]);
            glColor(color[0]);
            glVertex(verts[0]);
        }
        glEnd();
    }

    tm.bind(skyboxTexID[4]); // top
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBegin(GL_TRIANGLE_STRIP);
    {
        glTexCoord(txcds[0]);
        glColor(color[5]);
        glVertex(verts[5]);
        glTexCoord(txcds[1]);
        glColor(color[4]);
        glVertex(verts[4]);
        glTexCoord(txcds[3]);
        glColor(color[6]);
        glVertex(verts[6]);
        glTexCoord(txcds[2]);
        glColor(color[7]);
        glVertex(verts[7]);
    }
    glEnd();

    tm.bind(skyboxTexID[0]); // left
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBegin(GL_TRIANGLE_STRIP);
    {
        glTexCoord(txcds[0]);
        glColor(color[0]);
        glVertex(verts[0]);
        glTexCoord(txcds[1]);
        glColor(color[3]);
        glVertex(verts[3]);
        glTexCoord(txcds[3]);
        glColor(color[4]);
        glVertex(verts[4]);
        glTexCoord(txcds[2]);
        glColor(color[7]);
        glVertex(verts[7]);
    }
    glEnd();

    tm.bind(skyboxTexID[1]); // front
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBegin(GL_TRIANGLE_STRIP);
    {
        glTexCoord(txcds[0]);
        glColor(color[1]);
        glVertex(verts[1]);
        glTexCoord(txcds[1]);
        glColor(color[0]);
        glVertex(verts[0]);
        glTexCoord(txcds[3]);
        glColor(color[5]);
        glVertex(verts[5]);
        glTexCoord(txcds[2]);
        glColor(color[4]);
        glVertex(verts[4]);
    }
    glEnd();

    tm.bind(skyboxTexID[2]); // right
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBegin(GL_TRIANGLE_STRIP);
    {
        glTexCoord(txcds[0]);
        glColor(color[2]);
        glVertex(verts[2]);
        glTexCoord(txcds[1]);
        glColor(color[1]);
        glVertex(verts[1]);
        glTexCoord(txcds[3]);
        glColor(color[6]);
        glVertex(verts[6]);
        glTexCoord(txcds[2]);
        glColor(color[5]);
        glVertex(verts[5]);
    }
    glEnd();

    tm.bind(skyboxTexID[3]); // back
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBegin(GL_TRIANGLE_STRIP);
    {
        glTexCoord(txcds[0]);
        glColor(color[3]);
        glVertex(verts[3]);
        glTexCoord(txcds[1]);
        glColor(color[2]);
        glVertex(verts[2]);
        glTexCoord(txcds[3]);
        glColor(color[7]);
        glVertex(verts[7]);
        glTexCoord(txcds[2]);
        glColor(color[6]);
        glVertex(verts[6]);
    }
    glEnd();

    glShadeModel(GL_FLAT);
    glEnable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
}


void BackgroundRenderer::drawSky(SceneRenderer& renderer, bool mirror)
{
    glPushMatrix();

    const bool doSkybox = haveSkybox;

    if (!doSkybox)
    {
        // rotate sky so that horizon-point-toward-sun-color is actually
        // toward the sun
        glRotatef((GLfloat)((atan2f(sunDirection[1], sunDirection[0]) * 180.0 + 135.0) / M_PI),
                  0.0f, 0.0f, 1.0f);

        // draw sky
        skyGState.setState();
        if (!doSunset)
        {
            // just a pyramid
            glBegin(GL_TRIANGLE_FAN);
            glColor(skyZenithColor);
            glVertex(skyPyramid[4]);
            glColor(skyCrossSunDirColor);
            glVertex(skyPyramid[0]);
            glColor(skySunDirColor);
            glVertex(skyPyramid[3]);
            glColor(skyCrossSunDirColor);
            glVertex(skyPyramid[2]);
            glColor(skyAntiSunDirColor);
            glVertex(skyPyramid[1]);
            glColor(skyCrossSunDirColor);
            glVertex(skyPyramid[0]);
            glEnd();
        }
        else
        {
            // overall shape is a pyramid, but the solar sides are two
            // triangles each.  the top triangle is all zenith color,
            // the bottom goes from zenith to sun-dir color.
            glBegin(GL_TRIANGLE_FAN);
            glColor(skyZenithColor);
            glVertex(skyPyramid[4]);
            glColor(skyCrossSunDirColor);
            glVertex(skyPyramid[2]);
            glColor(skyAntiSunDirColor);
            glVertex(skyPyramid[1]);
            glColor(skyCrossSunDirColor);
            glVertex(skyPyramid[0]);
            glEnd();

            glm::vec3 sunsetTopPoint;
            sunsetTopPoint[0] = skyPyramid[3][0] * (1.0f - sunsetTop);
            sunsetTopPoint[1] = skyPyramid[3][1] * (1.0f - sunsetTop);
            sunsetTopPoint[2] = skyPyramid[4][2] * sunsetTop;
            glBegin(GL_TRIANGLES);
            glColor(skyZenithColor);
            glVertex(skyPyramid[4]);
            glColor(skyCrossSunDirColor);
            glVertex(skyPyramid[0]);
            glColor(skyZenithColor);
            glVertex(sunsetTopPoint);
            glVertex(skyPyramid[4]);
            glVertex(sunsetTopPoint);
            glColor(skyCrossSunDirColor);
            glVertex(skyPyramid[2]);
            glColor(skyZenithColor);
            glVertex(sunsetTopPoint);
            glColor(skyCrossSunDirColor);
            glVertex(skyPyramid[0]);
            glColor(skySunDirColor);
            glVertex(skyPyramid[3]);
            glColor(skyCrossSunDirColor);
            glVertex(skyPyramid[2]);
            glColor(skyZenithColor);
            glVertex(sunsetTopPoint);
            glColor(skySunDirColor);
            glVertex(skyPyramid[3]);
            glEnd();
        }
    }

    glLoadIdentity();
    renderer.getViewFrustum().executeOrientation();

    const bool useClipPlane = (mirror && (doSkybox || BZDBCache::drawCelestial));

    if (useClipPlane)
        glEnable(GL_CLIP_PLANE0);

    if (doSkybox)
        drawSkybox();

    if (BZDBCache::drawCelestial)
    {
        if (sunDirection[2] > -0.009f)
        {
            sunGState.setState();
            glColor(renderer.getSunScaledColor());
            glCallList(sunXFormList);
        }

        if (doStars)
        {
            starGState[starGStateIndex].setState();
            glCallList(starXFormList);
        }

        if (moonDirection[2] > -0.009f)
        {
            moonGState[doStars ? 1 : 0].setState();
            glColor3f(1.0f, 1.0f, 1.0f);
            //   if (useMoonTexture)
            //     glEnable(GL_TEXTURE_2D);
            glCallList(moonList);
        }

    }

    if (useClipPlane)
        glDisable(GL_CLIP_PLANE0);

    glPopMatrix();
}


void BackgroundRenderer::drawGround()
{
    if (!BZDBCache::drawGround)
        return;

    {
        // draw ground
        glNormal3f(0.0f, 0.0f, 1.0f);
        if (invert)
        {
            glColor(groundColorInv[styleIndex]);
            invGroundGState[styleIndex].setState();
        }
        else
        {
            float color[4];
            if (BZDB.isSet("GroundOverideColor") &&
                    parseColorString(BZDB.get("GroundOverideColor"), color))
                glColor4fv(color);
            else
                glColor(groundColor[styleIndex]);
            groundGState[styleIndex].setState();
        }

        {
            drawGroundCentered();
        }
    }
}

void BackgroundRenderer::drawGroundCentered()
{
    const float groundSize = 10.0f * BZDBCache::worldSize;
    const float centerSize = 128.0f;

    const ViewFrustum& frustum = RENDERER.getViewFrustum();
    auto center = glm::vec2(frustum.getEye());
    const float minDist = -groundSize + centerSize;
    const float maxDist = +groundSize - centerSize;
    center = glm::clamp(center, minDist, maxDist);

    const glm::vec2 vertices[8] =
    {
        { -groundSize, -groundSize },
        { +groundSize, -groundSize },
        { +groundSize, +groundSize },
        { -groundSize, +groundSize },
        { center[0] - centerSize, center[1] - centerSize },
        { center[0] + centerSize, center[1] - centerSize },
        { center[0] + centerSize, center[1] + centerSize },
        { center[0] - centerSize, center[1] + centerSize }
    };

    const float repeat = BZDB.eval("groundHighResTexRepeat");
    const int indices[5][4] =
    {
        { 4, 5, 7, 6 },
        { 0, 1, 4, 5 },
        { 1, 2, 5, 6 },
        { 2, 3, 6, 7 },
        { 3, 0, 7, 4 },
    };

    glNormal3f(0.0f, 0.0f, 1.0f);
    {
        for (int q = 0; q < 5; q++)
        {
            glBegin(GL_TRIANGLE_STRIP);
            for (int c = 0; c < 4; c++)
            {
                const int index = indices[q][c];
                glTexCoord(vertices[index] * repeat);
                glVertex(vertices[index]);
            }
            glEnd();
        }
    }

    return;
}


void BackgroundRenderer::drawGroundGrid(
    SceneRenderer& renderer)
{
    const auto &pos = renderer.getViewFrustum().getEye();
    const GLfloat xhalf = gridSpacing * (gridCount + floorf(pos[2] / 4.0f));
    const GLfloat yhalf = gridSpacing * (gridCount + floorf(pos[2] / 4.0f));
    const GLfloat x0 = floorf(pos[0] / gridSpacing) * gridSpacing;
    const GLfloat y0 = floorf(pos[1] / gridSpacing) * gridSpacing;
    GLfloat i;

    gridGState.setState();

    // x lines
    if (doShadows) glColor3f(0.0f, 0.75f, 0.5f);
    else glColor3f(0.0f, 0.4f, 0.3f);
    glPushMatrix();
    glTranslatef(x0 - xhalf, y0, 0.0f);
    glScalef(1.0f, yhalf, 0.0f);
    for (i = -xhalf; i <= xhalf; i += gridSpacing)
    {
        DRAWER.simmetricLineY();
        glTranslatef(gridSpacing, 0.0f, 0.0f);
    }
    glPopMatrix();

    /* z lines */
    if (doShadows) glColor3f(0.5f, 0.75f, 0.0f);
    else glColor3f(0.3f, 0.4f, 0.0f);
    glPushMatrix();
    glTranslatef(x0, y0 - yhalf, 0.0f);
    glScalef(xhalf, 1.0f, 0.0f);
    for (i = -yhalf; i <= yhalf; i += gridSpacing)
    {
        DRAWER.simmetricLineX();
        glTranslatef(0.0f, gridSpacing, 0.0f);
    }
    glPopMatrix();
}

void BackgroundRenderer::drawGroundShadows(
    SceneRenderer& renderer)
{
    // draw sun shadows -- always stippled so overlapping shadows don't
    // accumulate darkness.  make and multiply by shadow projection matrix.
    GLfloat shadowProjection[16];
    shadowProjection[0] = shadowProjection[5] = shadowProjection[15] = 1.0f;
    shadowProjection[8] = -sunDirection[0] / sunDirection[2];
    shadowProjection[9] = -sunDirection[1] / sunDirection[2];
    shadowProjection[1] = shadowProjection[2] =
                              shadowProjection[3] = shadowProjection[4] =
                                          shadowProjection[6] = shadowProjection[7] =
                                                  shadowProjection[10] = shadowProjection[11] =
                                                          shadowProjection[12] = shadowProjection[13] =
                                                                  shadowProjection[14] = 0.0f;
    glPushMatrix();
    glMultMatrixf(shadowProjection);

    // disable color updates
    SceneNode::setColorOverride(true);

    if (BZDBCache::stencilShadows)
    {
        OpenGLGState::resetState();
        const float shadowAlpha = BZDB.eval("shadowAlpha");
        glColor4f(0.0f, 0.0f, 0.0f, shadowAlpha);
        if (shadowAlpha < 1.0f)
        {
            // use the stencil to avoid overlapping shadows
            glClearStencil(0);
            glClear(GL_STENCIL_BUFFER_BIT);
            glStencilFunc(GL_NOTEQUAL, 1, 1);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            glEnable(GL_STENCIL_TEST);

            // turn on blending, and kill culling
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_BLEND);
            glDisable(GL_CULL_FACE);
        }
    }
    else
    {
        // use stippling to avoid overlapping shadows
        sunShadowsGState.setState();
        glColor3f(0.0f, 0.0f, 0.0f);
    }

    // render those nodes
    renderer.getShadowList().render();

    // revert to OpenGLGState defaults
    if (BZDBCache::stencilShadows)
    {
        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glDisable(GL_STENCIL_TEST);
        glBlendFunc(GL_ONE, GL_ZERO);
    }

    // enable color updates
    SceneNode::setColorOverride(false);

    OpenGLGState::resetState();

    glPopMatrix();
}


static void setupBlackFog(glm::vec4 &fogColor)
{
    static const auto black = glm::vec4(0.0f);
    fogColor = glGetFogColor();
    glSetFogColor(black);
}


void BackgroundRenderer::drawGroundReceivers(SceneRenderer& renderer)
{
    static const int receiverRings = 4;
    static const int receiverSlices = 8;
    static const float receiverRingSize = 1.2f;   // meters
    static glm::vec2 angle[receiverSlices + 1];

    static bool init = false;
    if (!init)
    {
        init = true;
        const float receiverSliceAngle = (float)(2.0 * M_PI / double(receiverSlices));
        for (int i = 0; i <= receiverSlices; i++)
        {
            angle[i][0] = cosf((float)i * receiverSliceAngle);
            angle[i][1] = sinf((float)i * receiverSliceAngle);
        }
    }

    const int count = renderer.getNumAllLights();
    if (count == 0)
        return;

    // bright sun dims intensity of ground receivers
    const float B = 1.0f - (0.6f * renderer.getSunBrightness());

    receiverGState.setState();

    // setup black fog
    glm::vec4 fogColor;
    setupBlackFog(fogColor);

    glPushMatrix();
    int i, j;
    for (int k = 0; k < count; k++)
    {
        const OpenGLLight& light = renderer.getLight(k);
        if (light.getOnlyReal())
            continue;

        const auto &pos = light.getPosition();
        const auto &lightColor = light.getColor();
        const GLfloat* atten = light.getAttenuation();

        // point under light
        float d = pos[2];
        float I = B / (atten[0] + d * (atten[1] + d * atten[2]));

        // maximum value
        const float maxVal = glm::compMax(glm::vec3(lightColor));

        // if I is too attenuated, don't bother drawing anything
        if ((I * maxVal) < 0.02f)
            continue;

        // move to the light's position
        glTranslatef(pos[0], pos[1], 0.0f);

        // set the main lighting color
        auto color = lightColor;
        color[3] = I;

        // draw ground receiver, computing lighting at each vertex ourselves
        glBegin(GL_TRIANGLE_FAN);
        {
            glColor(color);
            glVertex2f(0.0f, 0.0f);

            // inner ring
            d = hypotf(receiverRingSize, pos[2]);
            I = B / (atten[0] + d * (atten[1] + d * atten[2]));
            I *= pos[2] / d;
            color[3] = I;
            glColor(color);
            for (j = 0; j <= receiverSlices; j++)
                glVertex(receiverRingSize * angle[j]);
        }
        glEnd();
        triangleCount += receiverSlices;

        GLfloat outerSize = receiverRingSize;
        d = hypotf(outerSize, pos[2]);
        I = B / (atten[0] + d * (atten[1] + d * atten[2]));
        I *= pos[2] / d;

        glBegin(GL_TRIANGLE_STRIP);
        for (i = 1; i < receiverRings; i++)
        {
            const GLfloat innerSize = outerSize;
            outerSize = receiverRingSize * GLfloat((i + 1) * (i + 1));

            float innerAlpha = I;

            if (i + 1 == receiverRings)
                I = 0.0f;
            else
            {
                d = hypotf(outerSize, pos[2]);
                I = B / (atten[0] + d * (atten[1] + d * atten[2]));
                I *= pos[2] / d;
            }
            float outerAlpha = I;

            {
                for (j = 0; j <= receiverSlices; j++)
                {
                    color[3] = innerAlpha;
                    glColor(color);
                    glVertex(angle[j] * innerSize);
                    color[3] = outerAlpha;
                    glColor(color);
                    glVertex(angle[j] * outerSize);
                }
            }
        }
        glEnd();
        triangleCount += (receiverSlices * receiverRings * 2);

        glTranslatef(-pos[0], -pos[1], 0.0f);
    }
    glPopMatrix();

    glSetFogColor(fogColor);
}


void BackgroundRenderer::drawAdvancedGroundReceivers(SceneRenderer& renderer)
{
    const float minLuminance = 0.02f;
    static const int receiverSlices = 32;
    static const float receiverRingSize = 0.5f;   // meters
    static glm::vec2 angle[receiverSlices + 1];

    static bool init = false;
    if (!init)
    {
        init = true;
        const float receiverSliceAngle = (float)(2.0 * M_PI / double(receiverSlices));
        for (int i = 0; i <= receiverSlices; i++)
        {
            angle[i][0] = cosf((float)i * receiverSliceAngle);
            angle[i][1] = sinf((float)i * receiverSliceAngle);
        }
    }

    const int count = renderer.getNumAllLights();
    if (count == 0)
        return;

    // setup the ground tint
    auto gndColor = groundColor[styleIndex];
    glm::vec4 overrideColor;
    if (BZDB.isSet("GroundOverideColor") &&
            parseColorString(BZDB.get("GroundOverideColor"), overrideColor))
        gndColor = overrideColor;

    const bool useTexture = BZDBCache::texture && (groundTextureID >= 0);
    OpenGLGState advGState;
    OpenGLGStateBuilder builder;
    builder.setShading(GL_SMOOTH);
    builder.setBlending((GLenum)GL_ONE, (GLenum)GL_ONE);
    if (useTexture)
    {
        builder.setTexture(groundTextureID);
        builder.setTextureMatrix(groundTextureMatrix);
    }
    advGState = builder.getState();
    advGState.setState();

    // setup black fog
    glm::vec4 fogColor;
    setupBlackFog(fogColor);

    // lazy way to get texcoords
    if (useTexture)
    {
        const float repeat = BZDB.eval("groundHighResTexRepeat");
        const float sPlane[4] = { repeat, 0.0f, 0.0f, 0.0f };
        const float tPlane[4] = { 0.0f, repeat, 0.0f, 0.0f };
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
        glTexGenfv(GL_S, GL_EYE_PLANE, sPlane);
        glTexGenfv(GL_T, GL_EYE_PLANE, tPlane);
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
    }

    glPushMatrix();
    int i, j;
    for (int k = 0; k < count; k++)
    {
        const OpenGLLight& light = renderer.getLight(k);
        if (light.getOnlyReal())
            continue;

        // get the light parameters
        const auto &pos = light.getPosition();
        const auto &lightColor = light.getColor();
        const GLfloat* atten = light.getAttenuation();

        // point under light
        float d = pos[2];
        float I = 1.0f / (atten[0] + d * (atten[1] + d * atten[2]));

        // set the main lighting color
        auto baseColor = glm::vec3(gndColor * lightColor);
        if (invert)   // beats me, should just color logic op the static nodes
            baseColor = 1.0f - baseColor;

        // maximum value
        const float maxVal = glm::compMax(baseColor);

        // if I is too attenuated, don't bother drawing anything
        if ((I * maxVal) < minLuminance)
            continue;

        // move to the light's position
        glTranslatef(pos[0], pos[1], 0.0f);

        float innerSize;
        glm::vec3 innerColor;
        float outerSize;
        glm::vec3 outerColor;

        // draw ground receiver, computing lighting at each vertex ourselves
        glBegin(GL_TRIANGLE_FAN);
        {
            // center point
            innerColor = I * baseColor;
            glColor(innerColor);
            glVertex2f(0.0f, 0.0f);

            // inner ring
            d = hypotf(receiverRingSize, pos[2]);
            I = 1.0f / (atten[0] + d * (atten[1] + d * atten[2]));
            I *= pos[2] / d; // diffuse angle factor
            outerColor = I * baseColor;
            glColor(outerColor);
            outerSize = receiverRingSize;
            for (j = 0; j <= receiverSlices; j++)
                glVertex(outerSize * angle[j]);
        }
        glEnd();
        triangleCount += receiverSlices;

        bool moreRings = true;
        glBegin(GL_TRIANGLE_STRIP);
        for (i = 2; moreRings; i++)
        {
            // inner ring
            innerSize = outerSize;
            innerColor = outerColor;

            // outer ring
            outerSize = receiverRingSize * GLfloat(i * i);
            d = hypotf(outerSize, pos[2]);
            I = 1.0f / (atten[0] + d * (atten[1] + d * atten[2]));
            I *= pos[2] / d; // diffuse angle factor
            if ((I * maxVal) < minLuminance)
            {
                I = 0.0f;
                moreRings = false; // bail after this ring
            }
            outerColor = I * baseColor;

            {
                for (j = 0; j <= receiverSlices; j++)
                {
                    glColor(innerColor);
                    glVertex(angle[j] * innerSize);
                    glColor(outerColor);
                    glVertex(angle[j] * outerSize);
                }
            }
        }
        glEnd();
        triangleCount += (receiverSlices * 2 * (i - 2));

        glTranslatef(-pos[0], -pos[1], 0.0f);
    }
    glPopMatrix();

    if (useTexture)
    {
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
    }

    glSetFogColor(fogColor);
}


void BackgroundRenderer::drawMountains(void)
{
    glColor3f(1.0f, 1.0f, 1.0f);
    for (int i = 0; i < numMountainTextures; i++)
    {
        mountainsGState[i].setState();
        glCallList(mountainsList[i]);
    }
}


void BackgroundRenderer::doFreeDisplayLists()
{
    int i;

    // don't forget the tag-along
    weather.freeContext();
    EFFECTS.freeContext();

    // delete the single lists
    GLuint* const lists[] =
    {
        &cloudsList, &sunList, &sunXFormList,
        &moonList, &starList, &starXFormList
    };
    const int count = bzcountof(lists);
    for (i = 0; i < count; i++)
    {
        if (*lists[i] != INVALID_GL_LIST_ID)
        {
            glDeleteLists(*lists[i], 1);
            *lists[i] = INVALID_GL_LIST_ID;
        }
    }

    // delete the array of lists
    if (mountainsList != NULL)
    {
        for (i = 0; i < numMountainTextures; i++)
        {
            if (mountainsList[i] != INVALID_GL_LIST_ID)
            {
                glDeleteLists(mountainsList[i], 1);
                mountainsList[i] = INVALID_GL_LIST_ID;
            }
        }
    }

    return;
}


void BackgroundRenderer::doInitDisplayLists()
{
    int i, j;
    SceneRenderer& renderer = RENDERER;

    // don't forget the tag-along
    weather.rebuildContext();
    EFFECTS.rebuildContext();

    //
    // sky stuff
    //

    // sun first.  sun is a disk that should be about a half a degree wide
    // with a normal (60 degree) perspective.
    const float worldSize = BZDBCache::worldSize;
    const float sunRadius = (float)(2.0 * worldSize * atanf((float)(60.0*M_PI/180.0)) / 60.0);
    sunList = glGenLists(1);
    glNewList(sunList, GL_COMPILE);
    {
        glBegin(GL_TRIANGLE_FAN);
        {
            glVertex3f(2.0f * worldSize, 0.0f, 0.0f);
            for (i = 0; i < 20; i++)
            {
                const float angle = (float)(2.0 * M_PI * double(i) / 19.0);
                glVertex3f(2.0f * worldSize, sunRadius * sinf(angle),
                           sunRadius * cosf(angle));
            }
        }
        glEnd();
    }
    glEndList();

    // make stars list
    starList = glGenLists(1);
    glNewList(starList, GL_COMPILE);
    {
        glBegin(GL_POINTS);
        for (i = 0; i < (int)NumStars; i++)
        {
            glColor3fv(stars[i]);
            glVertex3fv(stars[i] + 3);
        }
        glEnd();
    }
    glEndList();

    //
    // ground
    //

    const GLfloat groundSize = 10.0f * worldSize;
    glm::vec3 groundPlane[4];
    for (i = 0; i < 4; i++)
        groundPlane[i] = glm::vec3(groundSize * squareShape[i], 0.0f);

    //
    // clouds
    //

    if (cloudsAvailable)
    {
        // make vertices for cloud polygons
        glm::vec3 cloudsOuter[4], cloudsInner[4];
        const GLfloat uvScale = 0.25f;
        for (i = 0; i < 4; i++)
        {
            cloudsOuter[i]   = groundPlane[i];
            cloudsOuter[i].z = 120.0f * BZDBCache::tankHeight;
            cloudsInner[i][0] = uvScale * cloudsOuter[i][0];
            cloudsInner[i][1] = uvScale * cloudsOuter[i][1];
            cloudsInner[i][2] = cloudsOuter[i][2];
        }

        cloudsList = glGenLists(1);
        glNewList(cloudsList, GL_COMPILE);
        {
            glNormal3f(0.0f, 0.0f, 1.0f);
            // inner clouds -- full opacity
            glBegin(GL_TRIANGLE_STRIP);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glTexCoord(uvScale * cloudRepeats * squareShape[3]);
            glVertex(cloudsInner[3]);
            glTexCoord(uvScale * cloudRepeats * squareShape[2]);
            glVertex(cloudsInner[2]);
            glTexCoord(uvScale * cloudRepeats * squareShape[0]);
            glVertex(cloudsInner[0]);
            glTexCoord(uvScale * cloudRepeats * squareShape[1]);
            glVertex(cloudsInner[1]);
            glEnd();

            // outer clouds -- fade to zero opacity at outer edge
            glBegin(GL_TRIANGLE_STRIP);
            glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
            glTexCoord(cloudRepeats * squareShape[1]);
            glVertex(cloudsOuter[1]);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glTexCoord(uvScale * cloudRepeats * squareShape[1]);
            glVertex(cloudsInner[1]);

            glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
            glTexCoord(cloudRepeats * squareShape[2]);
            glVertex(cloudsOuter[2]);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glTexCoord(uvScale * cloudRepeats * squareShape[2]);
            glVertex(cloudsInner[2]);

            glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
            glTexCoord(cloudRepeats * squareShape[3]);
            glVertex(cloudsOuter[3]);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glTexCoord(uvScale * cloudRepeats * squareShape[3]);
            glVertex(cloudsInner[3]);

            glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
            glTexCoord(cloudRepeats * squareShape[0]);
            glVertex(cloudsOuter[0]);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glTexCoord(uvScale * cloudRepeats * squareShape[0]);
            glVertex(cloudsInner[0]);

            glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
            glTexCoord(cloudRepeats * squareShape[1]);
            glVertex(cloudsOuter[1]);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glTexCoord(uvScale * cloudRepeats * squareShape[1]);
            glVertex(cloudsInner[1]);
            glEnd();
        }
        glEndList();
    }

    //
    // mountains
    //

    if (numMountainTextures > 0)
    {
        // prepare display lists.  need at least NumMountainFaces, but
        // we also need a multiple of the number of subtextures.  put
        // all the faces using a given texture into the same list.
        const int numFacesPerTexture = (NumMountainFaces +
                                        numMountainTextures - 1) / numMountainTextures;
        const float angleScale = (float)(M_PI / (numMountainTextures * numFacesPerTexture));
        int n = numFacesPerTexture / 2;
        float hightScale = mountainsMinWidth / 256.0f;

        for (j = 0; j < numMountainTextures; n += numFacesPerTexture, j++)
        {
            mountainsList[j] = glGenLists(1);
            glNewList(mountainsList[j], GL_COMPILE);
            {
                glBegin(GL_TRIANGLE_STRIP);
                for (i = 0; i <= numFacesPerTexture; i++)
                {
                    const float angle = angleScale * (float)(i + n);
                    float frac = (float)i / (float)numFacesPerTexture;
                    if (numMountainTextures != 1)
                        frac = (frac * (float)(mountainsMinWidth - 2) + 1.0f) /
                               (float)mountainsMinWidth;
                    glNormal3f((float)(-M_SQRT1_2 * cosf(angle)),
                               (float)(-M_SQRT1_2 * sinf(angle)),
                               (float)M_SQRT1_2);
                    glTexCoord2f(frac, 0.02f);
                    glVertex3f(2.25f * worldSize * cosf(angle),
                               2.25f * worldSize * sinf(angle),
                               0.0f);
                    glTexCoord2f(frac, 0.99f);
                    glVertex3f(2.25f * worldSize * cosf(angle),
                               2.25f * worldSize * sinf(angle),
                               0.45f * worldSize * hightScale);
                }
                glEnd();
                glBegin(GL_TRIANGLE_STRIP);
                for (i = 0; i <= numFacesPerTexture; i++)
                {
                    const float angle = (float)(M_PI + angleScale * (double)(i + n));
                    float frac = (float)i / (float)numFacesPerTexture;
                    if (numMountainTextures != 1)
                        frac = (frac * (float)(mountainsMinWidth - 2) + 1.0f) /
                               (float)mountainsMinWidth;
                    glNormal3f((float)(-M_SQRT1_2 * cosf(angle)),
                               (float)(-M_SQRT1_2 * sinf(angle)),
                               (float)M_SQRT1_2);
                    glTexCoord2f(frac, 0.02f);
                    glVertex3f(2.25f * worldSize * cosf(angle),
                               2.25f * worldSize * sinf(angle),
                               0.0f);
                    glTexCoord2f(frac, 0.99f);
                    glVertex3f(2.25f * worldSize * cosf(angle),
                               2.25f * worldSize * sinf(angle),
                               0.45f * worldSize*hightScale);
                }
                glEnd();
            }
            glEndList();
        }
    }

    //
    // update objects in sky.  the appearance of these objects will
    // be wrong until setCelestial is called with the appropriate
    // arguments.
    //
    makeCelestialLists(renderer);
}


void BackgroundRenderer::freeContext(void* self)
{
    ((BackgroundRenderer*)self)->doFreeDisplayLists();
}


void BackgroundRenderer::initContext(void* self)
{
    ((BackgroundRenderer*)self)->doInitDisplayLists();
}


const glm::vec3 *BackgroundRenderer::getSunDirection() const
{
    if (areShadowsCast(sunDirection))
        return &sunDirection;
    else
        return NULL;
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
