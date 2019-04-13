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
#include "RadarRenderer.h"

// System headers
#include <glm/ext/matrix_transform.hpp>

// common implementation headers
#include "SceneRenderer.h"
#include "MainWindow.h"
#include "OpenGLGState.h"
#include "BZDBCache.h"
#include "TextureManager.h"
#include "PhysicsDriver.h"
#include "ObstacleMgr.h"
#include "MeshSceneNode.h"
#include "ObstacleList.h"
#include "WallObstacle.h"
#include "BoxBuilding.h"
#include "PyramidBuilding.h"
#include "MeshObstacle.h"
#include "VBO_Drawing.h"
#include "OpenGLCommon.h"

// local implementation headers
#include "LocalPlayer.h"
#include "World.h"
#include "FlashClock.h"
#include "ShotPath.h"


static FlashClock flashTank;
static bool toggleTank = false;

const float RadarRenderer::colorFactor = 40.0f;

RadarRenderer::RadarRenderer(const SceneRenderer&, World* _world)
    : world(_world),
      x(0),
      y(0),
      w(0),
      h(0),
      dimming(0.0f),
      ps(),
      range(),
      decay(0.01f),
      smooth(false),
      jammed(false),
      useTankModels(false),
      useTankDimensions(false),
      triangleCount()
{

    setControlColor();
    buildBoxPyrMeshVBO();
}

void RadarRenderer::setWorld(World* _world)
{
    world = _world;
    buildBoxPyrMeshVBO();
}


void RadarRenderer::buildBoxPyrMeshVBO()
{
    if (world == nullptr)
    {
        meshVBO.clear();
        boxVBO.clear();
        boxOutVBO.clear();
        pyrVBO.clear();
        pyrOutVBO.clear();
        return;
    }

    std::vector<glm::vec3> vertex;
    const ObstacleList& meshes = OBSTACLEMGR.getMeshes();
    int count = meshes.size();

    meshVBO.resize(count);

    for (int i = 0; i < count; i++)
    {
        const MeshObstacle* mesh = (const MeshObstacle*) meshes[i];
        int faces = mesh->getFaceCount();
        meshVBO[i].resize(faces);

        for (int f = 0; f < faces; f++)
        {
            const MeshFace* face = mesh->getFace(f);

            // draw the face as a triangle fan
            int vertexCount = face->getVertexCount();
            meshVBO[i][f] = Vertex_Chunk(Vertex_Chunk::V, vertexCount);
            vertex.resize(vertexCount);
            int start = 0;
            int end   = vertexCount;
            for (int v = 0; v < vertexCount; v++)
            {
                int p;
                if (v == 0)
                    p = 0;
                else if (v % 2 == 1)
                    p = ++start;
                else
                    p = --end;
                const float* pos = face->getVertex(p);
                vertex[v] = glm::vec3(pos[0], pos[1], 0.0f);
            }
            meshVBO[i][f].vertexData(vertex);
        }
    }

    // draw box buildings.
    const ObstacleList& boxes = OBSTACLEMGR.getBoxes();
    count = boxes.size();

    boxVBO.resize(count);
    boxOutVBO.resize(count);

    glm::vec3 vertices[4];
    for (int i = 0; i < count; i++)
    {
        boxVBO[i]    = Vertex_Chunk();
        boxOutVBO[i] = Vertex_Chunk();
        const BoxBuilding& box = *((const BoxBuilding*) boxes[i]);
        if (box.isInvisible())
            continue;
        boxVBO[i]    = Vertex_Chunk(Vertex_Chunk::V, 4);
        boxOutVBO[i] = Vertex_Chunk(Vertex_Chunk::V, 4);
        const float  wx = box.getWidth();
        const float  hy = box.getBreadth();
        const float* pos = box.getPosition();
        const float  rot = box.getRotation();
        auto unity = glm::mat4(1.0f);
        auto translate(glm::translate(unity, glm::vec3(pos[0], pos[1], 0.0f)));
        auto rotate(glm::rotate(translate, rot, glm::vec3(0.0f, 0.0f, 1.0f)));
        vertices[0] = glm::vec3(rotate * glm::vec4(-wx, -hy, 0.0f, 1.0f));
        vertices[1] = glm::vec3(rotate * glm::vec4( wx, -hy, 0.0f, 1.0f));
        vertices[2] = glm::vec3(rotate * glm::vec4(-wx,  hy, 0.0f, 1.0f));
        vertices[3] = glm::vec3(rotate * glm::vec4( wx,  hy, 0.0f, 1.0f));
        boxVBO[i].vertexData(vertices);
        vertices[2] = glm::vec3(rotate * glm::vec4( wx,  hy, 0.0f, 1.0f));
        vertices[3] = glm::vec3(rotate * glm::vec4(-wx,  hy, 0.0f, 1.0f));
        boxOutVBO[i].vertexData(vertices);
    }

    // draw pyramid buildings
    const ObstacleList& pyramids = OBSTACLEMGR.getPyrs();
    count = pyramids.size();

    pyrVBO.resize(count);
    pyrOutVBO.resize(count);

    for (int i = 0; i < count; i++)
    {
        pyrVBO[i]    = Vertex_Chunk(Vertex_Chunk::V, 4);
        pyrOutVBO[i] = Vertex_Chunk(Vertex_Chunk::V, 4);
        const PyramidBuilding& pyr = *((const PyramidBuilding*) pyramids[i]);
        const float  wx  = pyr.getWidth();
        const float  hy  = pyr.getBreadth();
        const float* pos = pyr.getPosition();
        const float  rot = pyr.getRotation();
        auto unity = glm::mat4(1.0f);
        auto translate(glm::translate(unity, glm::vec3(pos[0], pos[1], 0.0f)));
        auto rotate(glm::rotate(translate, rot, glm::vec3(0.0f, 0.0f, 1.0f)));
        vertices[0] = glm::vec3(rotate * glm::vec4(-wx, -hy, 0.0f, 1.0f));
        vertices[1] = glm::vec3(rotate * glm::vec4( wx, -hy, 0.0f, 1.0f));
        vertices[2] = glm::vec3(rotate * glm::vec4(-wx,  hy, 0.0f, 1.0f));
        vertices[3] = glm::vec3(rotate * glm::vec4( wx,  hy, 0.0f, 1.0f));
        pyrVBO[i].vertexData(vertices);
        vertices[2] = glm::vec3(rotate * glm::vec4( wx,  hy, 0.0f, 1.0f));
        vertices[3] = glm::vec3(rotate * glm::vec4(-wx,  hy, 0.0f, 1.0f));
        pyrOutVBO[i].vertexData(vertices);
    }
}


void RadarRenderer::setControlColor(const GLfloat *color)
{
    if (color)
        memcpy(teamColor, color, 3 * sizeof(float));
    else
        memset(teamColor, 0, 3 * sizeof(float));
}


void RadarRenderer::setShape(int _x, int _y, int _w, int _h)
{
    x = _x;
    y = _y;
    w = _w;
    h = _h;
}


void RadarRenderer::setJammed(bool _jammed)
{
    jammed = _jammed;
    decay = 0.01;
}


void RadarRenderer::setDimming(float newDimming)
{
    dimming = (1.0f - newDimming > 1.0f) ? 1.0f : (1.0f - newDimming < 0.0f) ? 0.0f : 1.0f - newDimming;
}


void RadarRenderer::setTankColor(const Player* player)
{
    //The height box also uses the tank color

    const LocalPlayer *myTank = LocalPlayer::getMyTank();

    //my tank
    glm::vec3 color(1.0f);

    if (player->getId() != myTank->getId() )
    {
        //remote player
        auto team = myTank->getFlag() == Flags::Colorblindness
                    ? RogueTeam
                    : player->getTeam();
        color = Team::getRadarColor(team);
        if (player->isPaused() || player->isNotResponding())
        {
            const float dimfactor = 0.4f;
            color *= dimfactor;
        }

        // If this tank is hunted flash it on the radar
        if (player->isHunted() && myTank->getFlag() != Flags::Colorblindness)
        {
            if (flashTank.isOn())
            {
                if (!toggleTank)
                    color = glm::vec3(0.0f, 0.8f, 0.9f);
            }
            else
            {
                toggleTank = !toggleTank;
                flashTank.setClock(0.2f);
            }
        }
    }
    glColor4f(color.r, color.g, color.b, 1.0f);
}

void RadarRenderer::drawTank(const float pos[3], const Player* player, bool useSquares)
{
    FlagType *flag = player->getFlag();

    if (flag != Flags::Null)
    {
        auto c = flag->getRadarColor();
        glColor4f(c.r, c.g, c.b, 1.0f);
        drawFlagOnTank();
    }

    // 'ps' is pixel scale, setup in render()
    const float tankRadius = BZDBCache::tankRadius;
    float minSize = 1.5f + (ps * BZDBCache::radarTankPixels);
    GLfloat size;
    if (tankRadius < minSize)
        size = minSize;
    else
        size = tankRadius;
    if (pos[2] < 0.0f)
        size = 0.5f;

    // NOTE: myTank was checked in render()
    const float myAngle = LocalPlayer::getMyTank()->getAngle();

    // draw the tank
    if (useSquares || !useTankDimensions)
    {
        setTankColor(player);
        // align to the screen axes
        glRotatef(float(myAngle * 180.0 / M_PI), 0.0f, 0.0f, 1.0f);
        glPushMatrix();
        glScalef(size, size, 0.0f);
        DRAWER.simmetricRect();
        glPopMatrix();
    }
    else
    {
        const float tankAngle = player->getAngle();
        glPushMatrix();
        glRotatef(float(tankAngle * 180.0 / M_PI), 0.0f, 0.0f, 1.0f);
        if (useTankModels)
        {
            drawFancyTank(player);
            setTankColor(player);
        }
        else
        {
            setTankColor(player);
            const float* dims = player->getDimensions();
            glScalef(dims[0], dims[1], 0.0f);
            DRAWER.simmetricRect();
        }
        glPopMatrix();

        // align to the screen axes
        glRotatef(float(myAngle * 180.0 / M_PI), 0.0f, 0.0f, 1.0f);
    }

    // adjust with height box size
    const float boxHeight = BZDB.eval(StateDatabase::BZDB_BOXHEIGHT);
    size = size * (1.0f + (0.5f * (pos[2] / boxHeight)));

    // draw the height box
    glScalef(size, size, 0.0f);
    DRAWER.diamondLoop();
}


void RadarRenderer::drawFancyTank(const Player* player)
{
    if (smooth)
        glDisable(GL_BLEND);

    // we use the depth buffer so that the treads look ok
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    OpenGLGState::resetState();
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    RENDERER.enableSun(true);

    player->renderRadar(); // draws at (0,0,0)

    RENDERER.enableSun(false);
    OpenGLGState::resetState();

    glDisable(GL_DEPTH_TEST);

    if (smooth)
    {
        glEnable(GL_BLEND);
        glEnable(GL_POINT_SMOOTH);
    }

    return;
}


void RadarRenderer::drawFlag(const float pos[3])
{
    GLfloat s = BZDBCache::flagRadius > 3.0f * ps ? BZDBCache::flagRadius : 3.0f * ps;
    glPushMatrix();
    glTranslatef(pos[0], pos[1], 0.0f);
    glScalef(s, s, 0);
    DRAWER.cross();
    glPopMatrix();
}

void RadarRenderer::drawFlagOnTank()
{
    glPushMatrix();

    // align it to the screen axes
    const float angle = LocalPlayer::getMyTank()->getAngle();
    glRotatef(float(angle * 180.0 / M_PI), 0.0f, 0.0f, 1.0f);

    float tankRadius = BZDBCache::tankRadius;
    GLfloat s = 2.5f * tankRadius > 4.0f * ps ? 2.5f * tankRadius : 4.0f * ps;
    glScalef(s, s, 0);
    DRAWER.cross();

    glPopMatrix();
}


void RadarRenderer::renderFrame(SceneRenderer& renderer)
{
    const MainWindow& window = renderer.getWindow();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    window.setProjectionPlay();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    OpenGLGState::resetState();

    const int ox = window.getOriginX();
    const int oy = window.getOriginY();

    glScissor(ox + x - 1, oy + y - 1, w + 2, h + 2);

    const float left = float(ox + x) - 0.5f;
    const float top = float(oy + y) - 0.5f;

    float outlineOpacity = RENDERER.getRadarOpacity();
    float fudgeFactor = BZDBCache::hudGUIBorderOpacityFactor; // bzdb cache this maybe?
    if ( outlineOpacity < 1.0f )
        outlineOpacity = (outlineOpacity*fudgeFactor) + (1.0f - fudgeFactor);

    glEnable(GL_BLEND);

    glColor4f(teamColor[0],teamColor[1],teamColor[2],outlineOpacity);

    glPushMatrix();
    glTranslatef(left, top, 0.0f);
    glScalef(float(w + 1), float(h + 1), 0.0f);
    DRAWER.asimmetricSquareLoop();
    glPopMatrix();

    glDisable(GL_BLEND);

    glColor4f(teamColor[0],teamColor[1],teamColor[2],1.0f);

    const float opacity = renderer.getRadarOpacity();
    if ((opacity < 1.0f) && (opacity > 0.0f))
    {
        glScissor(ox + x - 2, oy + y - 2, w + 4, h + 4);
        // draw nice blended background
        if (opacity < 1.0f)
            glEnable(GL_BLEND);
        glColor4f(0.0f, 0.0f, 0.0f, opacity);
        glTranslatef((float)x, (float)y, 0.0f);
        glScalef((float)w, (float)h, 0.0f);
        DRAWER.asimmetricRect();
        if (opacity < 1.0f)
            glDisable(GL_BLEND);
    }

    // note that this scissor setup is used for the reset of the rendering
    glScissor(ox + x, oy + y, w, h);

    if (opacity == 1.0f)
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    return;
}


void RadarRenderer::render(SceneRenderer& renderer, bool blank, bool observer)
{
    RenderNode::resetTriangleCount();

    const float radarLimit = BZDBCache::radarLimit;
    if (!BZDB.isTrue("displayRadar") || (radarLimit <= 0.0f))
    {
        triangleCount = 0;
        return;
    }

    // render the frame
    renderFrame(renderer);

    if (blank)
        return;

    if (!world)
        return;

    smooth = BZDBCache::smooth;
    const bool fastRadar = (BZDBCache::radarStyle == 1) ||
                           (BZDBCache::radarStyle == 2);
    const LocalPlayer *myTank = LocalPlayer::getMyTank();

    // setup the radar range
    float radarRange = BZDB.eval("displayRadarRange") * radarLimit;
    float maxRange = radarLimit;
    // when burrowed, limit radar range
    if (myTank && (myTank->getFlag() == Flags::Burrow) &&
            (myTank->getPosition()[2] < 0.0f))
        maxRange = radarLimit / 4.0f;
    if (radarRange > maxRange)
    {
        radarRange = maxRange;
        // only clamp the user's desired range if it's actually
        // greater then 1. otherwise, we may be resetting it due
        // to burrow radar limiting.
        if (BZDB.eval("displayRadarRange") > 1.0f)
            BZDB.set("displayRadarRange", "1.0");
    }

    // prepare projection matrix
    glMatrixMode(GL_PROJECTION);
    const MainWindow& window = renderer.getWindow();
    // NOTE: the visual extents include passable objects
    double maxHeight = 0.0;
    const Extents* visExts = renderer.getVisualExtents();
    if (visExts)
        maxHeight = (double)visExts->maxs[2];
    window.setProjectionRadar(x, y, w, h, radarRange, (float)(maxHeight + 10.0));

    // prepare modelview matrix
    glMatrixMode(GL_MODELVIEW);

    OpenGLGState::resetState();


    // if jammed then draw white noise.  occasionally draw a good frame.
    if (jammed && (bzfrand() > decay))
    {
        glPushMatrix();
        glLoadIdentity();

        TextureManager &tm = TextureManager::instance();
        int noiseTexture = tm.getTextureID( "noise" );

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glScalef(radarRange, radarRange, 0.0f);

        if ((noiseTexture >= 0) && (renderer.useQuality() > 0))
        {

            const int sequences = 10;

            static float np[] =
            {
                0,         0,     1,  1,
                1,         1,    -1, -1,
                0.5f,   0.5f,     1,  1,
                1.5f,   1.5f,    -1, -1,
                0.25f, 0.25f,     1,  1,
                1.25f, 1.25f,    -1,  1,
                0,      0.5f,     1,  1,
                1,      1.5f,    -1, -1,
                0.5f,      0,     1,  1,
                1.4f,      1, -0.9f, -1,
                0.75f, 0.75f,     1,  1,
                1.75f, 1.75f,    -1, -1,
            };

            int noisePattern = 4 * int(floor(sequences * bzfrand()));

            glEnable(GL_TEXTURE_2D);
            tm.bind(noiseTexture);

            glMatrixMode(GL_TEXTURE);
            glPushMatrix();
            glTranslatef(np[noisePattern], np[noisePattern + 1], 0.0f);
            glScalef(np[noisePattern + 2], np[noisePattern + 3], 0.0f);
            DRAWER.simmetricTexturedRect();
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);

            glDisable(GL_TEXTURE_2D);
        }

        else if ((noiseTexture >= 0) && BZDBCache::texture &&
                 (renderer.useQuality() == 0))
        {
            glEnable(GL_TEXTURE_2D);
            tm.bind(noiseTexture);

            DRAWER.simmetricTexturedRect();

            glDisable(GL_TEXTURE_2D);
        }
        if (decay > 0.015f) decay *= 0.5f;

        glPopMatrix();
    }

    // only draw if there's a local player and a world
    else if (myTank)
    {
        glPushMatrix();
        glLoadIdentity();

        // if decay is sufficiently small then boost it so it's more
        // likely a jammed radar will get a few good frames closely
        // spaced in time.  value of 1 guarantees at least two good
        // frames in a row.
        if (decay <= 0.015f) decay = 1.0f;
        else decay *= 0.5f;


        // get size of pixel in model space (assumes radar is square)
        ps = 2.0f * (radarRange / GLfloat(w));
        MeshSceneNode::setRadarLodScale(ps);

        float tankWidth = BZDBCache::tankWidth;
        float tankLength = BZDBCache::tankLength;
        const float testMin = 8.0f * ps;
        // maintain the aspect ratio if it isn't square
        if ((tankWidth > testMin) &&  (tankLength > testMin))
            useTankDimensions = true;
        else
            useTankDimensions = false;
        if (useTankDimensions && (renderer.useQuality() >= 2))
            useTankModels = true;
        else
            useTankModels = false;

        // relative to my tank
        const float* myPos = myTank->getPosition();
        const float myAngle = myTank->getAngle();

        // draw the view angle below stuff
        // view frustum edges
        if (!BZDB.isTrue("hideRadarViewLines"))
        {
            glColor4f(1.0f, 0.625f, 0.125f, 1.0f);
            const float fovx = renderer.getViewFrustum().getFOVx();
            const float viewWidth = radarRange * tanf(0.5f * fovx);
            glPushMatrix();
            glScalef(viewWidth, radarRange, 0.0f);
            DRAWER.viewAngle();
            glPopMatrix();
        }

        // transform to the observer's viewpoint
        glPushMatrix();
        glRotatef((float)(90.0 - myAngle * 180.0 / M_PI), 0.0f, 0.0f, 1.0f);
        glPushMatrix();
        glTranslatef(-myPos[0], -myPos[1], 0.0f);

        if (useTankModels)
        {
            // new modelview transform requires repositioning
            renderer.setupSun();
        }

        // setup the blending function
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // draw the buildings
        renderObstacles(fastRadar, radarRange);

        if (smooth)
        {
            glEnable(GL_BLEND);
            glEnable(GL_LINE_SMOOTH);
            glEnable(GL_POINT_SMOOTH);
        }

        // draw my shots
        int maxShots = world->getMaxShots();
        int i;
        float muzzleHeight = BZDB.eval(StateDatabase::BZDB_MUZZLEHEIGHT);
        for (i = 0; i < maxShots; i++)
        {
            const ShotPath* shot = myTank->getShot(i);
            if (shot)
            {
                const float cs = colorScale(shot->getPosition()[2], muzzleHeight);
                glColor4f(1.0f * cs, 1.0f * cs, 1.0f * cs, 1.0f);
                shot->radarRender();
            }
        }

        //draw world weapon shots
        WorldPlayer *worldWeapons = World::getWorld()->getWorldWeapons();
        maxShots = worldWeapons->getMaxShots();
        for (i = 0; i < maxShots; i++)
        {
            const ShotPath* shot = worldWeapons->getShot(i);
            if (shot)
            {
                const float cs = colorScale(shot->getPosition()[2], muzzleHeight);
                glColor4f(1.0f * cs, 1.0f * cs, 1.0f * cs, 1.0f);
                shot->radarRender();
            }
        }

        // draw other tanks (and any flags on them)
        // note about flag drawing.  each line segment is drawn twice
        // (once in each direction);  this degrades the antialiasing
        // but on systems that don't do correct filtering of endpoints
        // not doing it makes (half) the endpoints jump wildly.
        const int curMaxPlayers = world->getCurMaxPlayers();
        for (i = 0; i < curMaxPlayers; i++)
        {
            RemotePlayer* player = world->getPlayer(i);
            if (!player)
                continue;
            if (!player->isAlive() &&
                    (!useTankModels || !observer || !player->isExploding()))
                continue;
            if ((player->getFlag() == Flags::Stealth) &&
                    (myTank->getFlag() != Flags::Seer))
                continue;

            const float* position = player->getPosition();

            glPushMatrix();
            glTranslatef(position[0], position[1], 0.0f);
            if (!observer)
                drawTank(position, player, true);
            else
                drawTank(position, player, false);
            glPopMatrix();
        }

        bool coloredShot = BZDB.isTrue("coloredradarshots");
        // draw other tanks' shells
        bool iSeeAll = myTank && (myTank->getFlag() == Flags::Seer);
        maxShots = World::getWorld()->getMaxShots();
        for (i = 0; i < curMaxPlayers; i++)
        {
            RemotePlayer* player = world->getPlayer(i);
            if (!player) continue;
            for (int j = 0; j < maxShots; j++)
            {
                const ShotPath* shot = player->getShot(j);
                if (shot && (shot->getFlag() != Flags::InvisibleBullet || iSeeAll))
                {
                    const float cs = colorScale(shot->getPosition()[2], muzzleHeight);
                    if (coloredShot)
                    {
                        glm::vec3 shotcolor;
                        if (myTank->getFlag() == Flags::Colorblindness)
                            shotcolor = Team::getRadarColor(RogueTeam);
                        else
                            shotcolor = Team::getRadarColor(player->getTeam());
                        glColor4f(shotcolor[0] * cs, shotcolor[1] * cs, shotcolor[2] * cs, 1.0f);
                    }
                    else
                        glColor4f(cs, cs, cs, 1.0f);
                    shot->radarRender();
                }
            }
        }

        // draw flags not on tanks.
        // draw them in reverse order so that the team flags
        // (which come first), are drawn on top of the normal flags.
        const int maxFlags = world->getMaxFlags();
        const bool drawNormalFlags = BZDB.isTrue("displayRadarFlags");
        const bool hideTeamFlagsOnRadar = BZDB.isTrue(StateDatabase::BZDB_HIDETEAMFLAGSONRADAR);
        const bool hideFlagsOnRadar = BZDB.isTrue(StateDatabase::BZDB_HIDEFLAGSONRADAR);
        for (i = (maxFlags - 1); i >= 0; i--)
        {
            const Flag& flag = world->getFlag(i);
            // don't draw flags that don't exist or are on a tank
            if (flag.status == FlagNoExist || flag.status == FlagOnTank)
                continue;
            // don't draw normal flags if we aren't supposed to
            if (flag.type->flagTeam == NoTeam && !drawNormalFlags)
                continue;
            if (hideTeamFlagsOnRadar)
            {
                if (flag.type->flagTeam != ::NoTeam)
                    continue;
            }
            if (hideFlagsOnRadar)
            {
                if (flag.type)
                    continue;
            }
            // Flags change color by height
            const float cs = colorScale(flag.position[2], muzzleHeight);
            const auto flagcolor = flag.type->getRadarColor();
            glColor4f(flagcolor[0] * cs, flagcolor[1] * cs, flagcolor[2] * cs, 1.0f);
            drawFlag(flag.position);
        }
        // draw antidote flag
        const float* antidotePos =
            LocalPlayer::getMyTank()->getAntidoteLocation();
        if (antidotePos)
        {
            glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
            drawFlag(antidotePos);
        }

        // draw these markers above all others always centered
        glPopMatrix();

        // north marker
        GLfloat ns = 0.05f * radarRange, ny = 0.9f * radarRange;
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glTranslatef(0.0f, ny, 0.0f);
        glScalef(ns, ns, 0.0f);
        DRAWER.north();

        // always up
        glPopMatrix();

        // forward tick
        glPushMatrix();
        glTranslatef(0.0f, radarRange - 4.0f * ps, 0.0f);
        glScalef(0.0f, 3.0 * ps, 0.0f);
        DRAWER.asimmetricLineY();
        glPopMatrix();

        if (!observer)
        {
            // revert to the centered transformation
            glRotatef((float)(90.0 - myAngle * 180.0 / M_PI), 0.0f, 0.0f, 1.0f);

            glPushMatrix();
            // my tank
            drawTank(myPos, myTank, false);

            glPopMatrix();

            // re-setup the blending function
            // (was changed by drawing jump jets)
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        glPopMatrix();

        if (dimming > 0.0f)
        {
            if (!smooth)
                glEnable(GL_BLEND);
            // darken the entire radar if we're dimmed
            // we're drawing positively, so dimming is actually an opacity
            glColor4f(0.0f, 0.0f, 0.0f, 1.0f - dimming);
            glPushMatrix();
            glScalef(radarRange, radarRange, 0.0f);
            DRAWER.simmetricRect();
            glPopMatrix();
        }
        glDisable(GL_BLEND);
        glDisable(GL_LINE_SMOOTH);
        glDisable(GL_POINT_SMOOTH);
    }

    triangleCount = RenderNode::getTriangleCount();
}


float RadarRenderer::colorScale(const float z, const float h)
{
    float scaleColor;
    if (BZDBCache::radarStyle > 0)
    {
        const LocalPlayer* myTank = LocalPlayer::getMyTank();

        // Scale color so that objects that are close to tank's level are opaque
        const float zTank = myTank->getPosition()[2];

        if (zTank > (z + h))
            scaleColor = 1.0f - (zTank - (z + h)) / colorFactor;
        else if (zTank < z)
            scaleColor = 1.0f - (z - zTank) / colorFactor;
        else
            scaleColor = 1.0f;

        // Don't fade all the way
        if (scaleColor < 0.35f)
            scaleColor = 0.35f;
    }
    else
        scaleColor = 1.0f;

    return scaleColor;
}


float RadarRenderer::transScale(const float z, const float h)
{
    float scaleColor;
    const LocalPlayer* myTank = LocalPlayer::getMyTank();

    // Scale color so that objects that are close to tank's level are opaque
    const float zTank = myTank->getPosition()[2];
    if (zTank > (z + h))
        scaleColor = 1.0f - (zTank - (z + h)) / colorFactor;
    else if (zTank < z)
        scaleColor = 1.0f - (z - zTank) / colorFactor;
    else
        scaleColor = 1.0f;

    if (scaleColor < 0.5f)
        scaleColor = 0.5f;

    return scaleColor;
}


void RadarRenderer::renderObstacles(bool fastRadar, float _range)
{
    if (smooth)
    {
        glEnable(GL_BLEND);
        glEnable(GL_LINE_SMOOTH);
    }

    // draw the walls
    renderWalls();

    // draw the boxes, pyramids, and meshes
    if (!fastRadar)
        renderBoxPyrMesh();
    else
        renderBoxPyrMeshFast(_range);

    // draw the team bases and teleporters
    renderBasesAndTeles();

    if (smooth)
    {
        glDisable(GL_BLEND);
        glDisable(GL_LINE_SMOOTH);
    }

    return;
}


void RadarRenderer::renderWalls()
{
    const ObstacleList& walls = OBSTACLEMGR.getWalls();
    int count = walls.size();
    glColor4f(0.25f, 0.5f, 0.5f, 1.0f);
    for (int i = 0; i < count; i++)
    {
        const WallObstacle& wall = *((const WallObstacle*) walls[i]);
        const float wid = wall.getBreadth();
        const float* pos = wall.getPosition();
        const float  rot = RAD2DEG * wall.getRotation();
        glPushMatrix();
        glTranslatef(pos[0], pos[1], 0.0f);
        glScalef(wid, wid, 0.0f);
        glRotatef(rot, 0.0f, 0.0f, 1.0f);
        DRAWER.simmetricLineY();
        glPopMatrix();
    }

    return;
}


void RadarRenderer::renderBoxPyrMeshFast(float _range)
{
    // FIXME - This is hack code at the moment, but even when
    //     rendering the full world, it draws the aztec map
    //     3X faster (the culling algo is actually slows us
    //     down in that case)
    //       - need a better default gradient texture
    //         (better colors, and tied in to show max jump height?)
    //       - build a procedural texture if default is missing
    //       - use a GL_TEXTURE_1D
    //       - setup the octree to return Z sorted elements (partially done)
    //       - add a renderClass() member to SceneNode (also for coloring)
    //       - also add a renderShadow() member (they don't need sorting,
    //         and if you don't have double-buffering, you shouldn't be
    //         using shadows)
    //       - vertex shaders would be faster
    //       - it would probably be a better approach to attach a radar
    //         rendering object to each obstacle... no time

    // get the texture
    int gradientTexId = -1;
    TextureManager &tm = TextureManager::instance();
    gradientTexId = tm.getTextureID("radar", false);

    // safety: no texture, no service
    if (gradientTexId < 0)
    {
        renderBoxPyrMesh();
        return;
    }

    // GL state
    OpenGLGStateBuilder gb;
    gb.setTexture(gradientTexId);
    gb.setShading(GL_FLAT);
    gb.setCulling(GL_BACK);
    gb.setBlending(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    OpenGLGState gs = gb.getState();
    gs.setState();

    // now that the texture is bound, setup the clamp mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);

    // do this after the GState setting
    if (smooth)
        glEnable(GL_POLYGON_SMOOTH);

    // setup the texturing mapping
    const float hf = 128.0f; // height factor, goes from 0.0 to 1.0 in texcoords
    const float vfz = RENDERER.getViewFrustum().getEye()[2];
    const GLfloat plane[4] =
    { 0.0f, 0.0f, (1.0f / hf), (((hf * 0.5f) - vfz) / hf) };
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_S, GL_EYE_PLANE, plane);

    // setup texture generation
    glEnable(GL_TEXTURE_GEN_S);

    // set the color
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

//  if (!BZDB.isTrue("visualRadar")) {
    ViewFrustum radarClipper;
    radarClipper.setOrthoPlanes(RENDERER.getViewFrustum(), _range, _range);
    RENDERER.getSceneDatabase()->renderRadarNodes(radarClipper);
//  } else {
//    RENDERER.getSceneDatabase()->renderRadarNodes(RENDERER.getViewFrustum());
//  }

    // restore texture generation
    glDisable(GL_TEXTURE_GEN_S);

    OpenGLGState::resetState();

    // do this after the GState setting
    if (smooth)
    {
        glEnable(GL_BLEND);
        glEnable(GL_LINE_SMOOTH);
        glDisable(GL_POLYGON_SMOOTH);
    }

    return;
}


void RadarRenderer::renderBoxPyrMesh()
{
    int i;

    const bool enhanced = (BZDBCache::radarStyle > 0);

    if (!smooth)
    {
        // smoothing has blending disabled
        if (enhanced)
        {
            glEnable(GL_BLEND); // always blend the polygons if we're enhanced
        }
    }
    else
    {
        // smoothing has blending enabled
        if (!enhanced)
        {
            glDisable(GL_BLEND); // don't blend the polygons if we're not enhanced
        }
    }

    // draw box buildings.
    const ObstacleList& boxes = OBSTACLEMGR.getBoxes();
    int count = boxes.size();
    for (i = 0; i < count; i++)
    {
        const BoxBuilding& box = *((const BoxBuilding*) boxes[i]);
        const float z = box.getPosition()[2];
        const float bh = box.getHeight();
        const float cs = colorScale(z, bh);
        glColor4f(0.25f * cs, 0.5f * cs, 0.5f * cs, transScale(z, bh));
        boxVBO[i].draw(GL_TRIANGLE_STRIP);
    }

    // draw pyramid buildings
    const ObstacleList& pyramids = OBSTACLEMGR.getPyrs();
    count = pyramids.size();
    for (i = 0; i < count; i++)
    {
        const PyramidBuilding& pyr = *((const PyramidBuilding*) pyramids[i]);
        const float z = pyr.getPosition()[2];
        const float bh = pyr.getHeight();
        const float cs = colorScale(z, bh);
        glColor4f(0.25f * cs, 0.5f * cs, 0.5f * cs, transScale(z, bh));
        pyrVBO[i].draw(GL_TRIANGLE_STRIP);
    }

    // draw mesh obstacles
    if (smooth)
        glEnable(GL_POLYGON_SMOOTH);
    if (!enhanced)
        glDisable(GL_CULL_FACE);
    const ObstacleList& meshes = OBSTACLEMGR.getMeshes();
    for (i = 0; i < (int)meshVBO.size(); i++)
    {
        const MeshObstacle* mesh = (const MeshObstacle*) meshes[i];

        for (int f = 0; f < (int)meshVBO[i].size(); f++)
        {
            const MeshFace* face = mesh->getFace(f);
            if (enhanced)
            {
                if (face->getPlane()[2] <= 0.0f)
                    continue;
                const BzMaterial* bzmat = face->getMaterial();
                if ((bzmat != NULL) && bzmat->getNoRadar())
                    continue;
            }
            float z = face->getPosition()[2];
            float bh = face->getSize()[2];

            if (BZDBCache::useMeshForRadar)
            {
                z = mesh->getPosition()[2];
                bh = mesh->getSize()[2];
            }

            const float cs = colorScale(z, bh);
            // draw death faces with a soupcon of red
            const PhysicsDriver* phydrv = PHYDRVMGR.getDriver(face->getPhysicsDriver());
            if ((phydrv != NULL) && phydrv->getIsDeath())
                glColor4f(0.75f * cs, 0.25f * cs, 0.25f * cs, transScale(z, bh));
            else
                glColor4f(0.25f * cs, 0.5f * cs, 0.5f * cs, transScale(z, bh));
            // draw the face as a triangle fan
            meshVBO[i][f].draw(GL_TRIANGLE_STRIP);
        }
    }
    if (!enhanced)
        glEnable(GL_CULL_FACE);
    if (smooth)
        glDisable(GL_POLYGON_SMOOTH);

    // NOTE: revert from the enhanced setting
    if (enhanced && !smooth)
        glDisable(GL_BLEND);

    // now draw antialiased outlines around the polygons
    if (smooth)
    {
        glEnable(GL_BLEND); // NOTE: revert from the enhanced setting
        count = boxes.size();
        for (i = 0; i < count; i++)
        {
            const BoxBuilding& box = *((const BoxBuilding*) boxes[i]);
            const float z = box.getPosition()[2];
            const float bh = box.getHeight();
            const float cs = colorScale(z, bh);
            glColor4f(0.25f * cs, 0.5f * cs, 0.5f * cs, transScale(z, bh));
            boxOutVBO[i].draw(GL_LINE_LOOP);
        }

        count = pyramids.size();
        for (i = 0; i < count; i++)
        {
            const PyramidBuilding& pyr = *((const PyramidBuilding*) pyramids[i]);
            const float z = pyr.getPosition()[2];
            const float bh = pyr.getHeight();
            const float cs = colorScale(z, bh);
            glColor4f(0.25f * cs, 0.5f * cs, 0.5f * cs, transScale(z, bh));
            pyrOutVBO[i].draw(GL_LINE_LOOP);
        }
    }

    return;
}


void RadarRenderer::renderBasesAndTeles()
{
    int i;

    // draw team bases
    if (world->allowTeamFlags())
    {
        for (i = 1; i < NumTeams; i++)
        {
            for (int j = 0;; j++)
            {
                const float *base = world->getBase(i, j);
                if (base == NULL)
                    break;
                auto c = Team::getRadarColor(TeamColor(i));
                glColor4f(c.r, c.g, c.b, 1.0f);
                const float rot = RAD2DEG * base[3];
                glPushMatrix();
                glTranslatef(base[0], base[1], 0.0f);
                glRotatef(rot, 0.0f, 0.0f, 1.0f);
                glScalef(base[4], base[5], 0.0f);
                DRAWER.simmetricSquareLoop();
                glPopMatrix();
            }
        }
    }

    // draw teleporters.  teleporters are pretty thin so use lines
    // (which, if longer than a pixel, are guaranteed to draw something;
    // not so for a polygon).  just in case the system doesn't correctly
    // filter the ends of line segments, we'll draw the line in each
    // direction (which degrades the antialiasing).  Newport graphics
    // is one system that doesn't do correct filtering.
    const ObstacleList& teleporters = OBSTACLEMGR.getTeles();
    int count = teleporters.size();
    glColor4f(1.0f, 1.0f, 0.25f, 1.0f);
    for (i = 0; i < count; i++)
    {
        const Teleporter & tele = *((const Teleporter *) teleporters[i]);
        if (tele.isHorizontal ())
        {
            const float z = tele.getPosition ()[2];
            const float bh = tele.getHeight ();
            const float cs = colorScale (z, bh);
            glColor4f (1.0f * cs, 1.0f * cs, 0.25f * cs, transScale (z, bh));
            const float wx = tele.getWidth();
            const float hy = tele.getBreadth();
            const float *pos = tele.getPosition ();
            const float rot = RAD2DEG * tele.getRotation();
            glPushMatrix();
            glTranslatef(pos[0], pos[1], 0.0f);
            glRotatef(rot, 0.0f, 0.0f, 1.0f);
            glScalef(wx, hy, 0.0f);
            DRAWER.simmetricSquareLoop();
            glPopMatrix();
        }
        else
        {
            const float z = tele.getPosition ()[2];
            const float bh = tele.getHeight ();
            const float cs = colorScale (z, bh);
            glColor4f (1.0f * cs, 1.0f * cs, 0.25f * cs, transScale (z, bh));
            const float tw = tele.getBreadth ();
            const float *pos = tele.getPosition ();
            const float rot = RAD2DEG * tele.getRotation();
            glPushMatrix();
            glTranslatef(pos[0], pos[1], 0.0f);
            glScalef(tw, tw, 0.0f);
            glRotatef(rot, 0.0f, 0.0f, 1.0f);
            DRAWER.simmetricLineY();
            glPopMatrix();
        }
    }

    return;
}


int RadarRenderer::getFrameTriangleCount() const
{
    return triangleCount;
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
