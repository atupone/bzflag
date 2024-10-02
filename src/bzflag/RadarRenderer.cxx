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
#include "OpenGLAPI.h"
#include "VBO_Drawing.h"
#include "PlayingShader.h"

// local implementation headers
#include "LocalPlayer.h"
#include "World.h"
#include "FlashClock.h"
#include "ShotPath.h"


static FlashClock flashTank;
static bool toggleTank = false;

const float RadarRenderer::colorFactor = 40.0f;

RadarRenderer::RadarRenderer(const SceneRenderer&, World* _world)
    :
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
    triangleCount()
{

    setControlColor();
    setWorld(_world);
}

void RadarRenderer::setWorld(World* _world)
{
    world = _world;
    buildBoxPyrMeshVBO();
    buildWallsVBO();
    buildBasesAndTelesVBO();
}

void RadarRenderer::setControlColor(const glm::vec3 &color)
{
    teamColor = color;
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


glm::vec3 RadarRenderer::getTankColor(const Player* player)
{
    //The height box also uses the tank color

    const LocalPlayer *myTank = LocalPlayer::getMyTank();

    //my tank
    if (player->getId() == myTank->getId() )
        return glm::vec3(1.0f);

    //remote player

    // If this tank is hunted flash it on the radar
    if (player->isHunted() && myTank->getFlag() != Flags::Colorblindness)
    {
        if (flashTank.isOn())
        {
            if (!toggleTank)
            {
                const auto flashcolor = glm::vec3(0.0f, 0.8f, 0.9f);
                return flashcolor;
            }
        }
        else
        {
            toggleTank = !toggleTank;
            flashTank.setClock(0.2f);
        }
    }

    auto team = player->getTeam();

    if (myTank->getFlag() == Flags::Colorblindness)
        team = RogueTeam;

    auto color = Team::getRadarColor(team);

    const float dimfactor = 0.4f;
    if (player->isPaused() || player->isNotResponding())
        color *= dimfactor;

    return color;
}

void RadarRenderer::drawTank(const glm::vec3 &pos,
                             float myAngle,
                             float tankSize, float flagSize,
                             const Player* player, bool useSquares)
{
    auto flag = player->getFlag();

    // my flag
    if (flag != Flags::Null)
    {
        glColor(flag->getRadarColor());
        glPushMatrix();

        // align it to the screen axes
        glRotatef(float(myAngle * 180.0 / M_PI), 0.0f, 0.0f, 1.0f);

        glScalef(flagSize, flagSize, 0);
        DRAWER.cross();

        glPopMatrix();
    }

    if (pos[2] < 0.0f)
        tankSize = 0.5f;

    // NOTE: myTank was checked in render()
    // draw the tank
    if (!useSquares)
    {
        const float tankAngle = player->getAngle();
        glPushMatrix();
        glRotatef(float(tankAngle * 180.0 / M_PI), 0.0f, 0.0f, 1.0f);
        drawFancyTank(player);
        glPopMatrix();
    }
    auto tankColor = getTankColor(player);
    glColor(tankColor);

    // align to the screen axes
    glRotatef(float(myAngle * 180.0 / M_PI), 0.0f, 0.0f, 1.0f);
    if (useSquares)
    {
        glPushMatrix();
        glScalef(tankSize, tankSize, 0.0f);
        DRAWER.simmetricRect();
        glPopMatrix();
    }

    // adjust with height box size
    const float boxHeight = BZDB.eval(StateDatabase::BZDB_BOXHEIGHT);
    tankSize *= 1.0f + 0.5f * pos[2] / boxHeight;

    // draw the height box
    glScalef(tankSize, tankSize, 0.0f);
    DRAWER.diamondLoop();
}


void RadarRenderer::drawFancyTank(const Player* player)
{
    if (smooth)
        glDisable(GL_BLEND);

    // we use the depth buffer so that the treads look ok
    {
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
    }

    OpenGLGState::resetState();
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    RENDERER.enableSun(true);

    player->renderRadar(); // draws at (0,0,0)

    RENDERER.enableSun(false);
    OpenGLGState::resetState();

    {
        glDisable(GL_DEPTH_TEST);
    }

    if (smooth)
    {
        glEnable(GL_BLEND);
        glEnable(GL_POINT_SMOOTH);
    }

    return;
}


void RadarRenderer::drawFlag(const glm::vec3 &pos)
{
    GLfloat s = BZDBCache::flagRadius > 3.0f * ps ? BZDBCache::flagRadius : 3.0f * ps;
    glPushMatrix();
    glTranslatef(pos[0], pos[1], 0.0f);
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

    {
        glEnable(GL_BLEND);
    }

    glColor(teamColor, outlineOpacity);

    glPushMatrix();
    glTranslatef(left, top, 0.0f);
    glScalef(float(w + 1), float(h + 1), 0.0f);
    DRAWER.asimmetricSquareLoop();
    glPopMatrix();

    {
        glDisable(GL_BLEND);
    }

    glColor(teamColor);

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
    const bool fastRadar = ((BZDBCache::radarStyle == 1) ||
                            (BZDBCache::radarStyle == 2));
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
        glScalef(radarRange, radarRange, radarRange);

        TextureManager &tm = TextureManager::instance();
        int noiseTexture = tm.getTextureID( "noise" );

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        if (noiseTexture >= 0)
        {

            const int sequences = 10;

            static float np[] =
            {
                0, 0, 1, 1,
                1, 1, 0, 0,
                0.5f, 0.5f, 1.5f, 1.5f,
                1.5f, 1.5f, 0.5f, 0.5f,
                0.25f, 0.25f, 1.25f, 1.25f,
                1.25f, 1.25f, 0.25f, 0.25f,
                0, 0.5f, 1, 1.5f,
                1, 1.5f, 0, 0.5f,
                0.5f, 0, 1.5f, 1,
                1.4f, 1, 0.5f, 0,
                0.75f, 0.75f, 1.75f, 1.75f,
                1.75f, 1.75f, 0.75f, 0.75f,
            };

            int noisePattern = 4 * int(floor(sequences * bzfrand()));

            SHADER.setTexturing(true);
            tm.bind(noiseTexture);

            glMatrixMode(GL_TEXTURE);
            glPushMatrix();
            glTranslatef(np[noisePattern], np[noisePattern + 1], 0.0f);
            glScalef(np[noisePattern+2] - np[noisePattern],
                     np[noisePattern+3] - np[noisePattern + 1],
                     1.0f);
            DRAWER.simmetricTexturedRect();
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);

            SHADER.setTexturing(false);
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
            useTankModels = true;
        else
            useTankModels = false;

        // relative to my tank
        const auto &myPos = myTank->getPosition();
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

        SHADER.setZTank(myPos.z);

        // draw the buildings
        renderObstacles(fastRadar, radarRange);

        if (smooth)
        {
            glEnable(GL_BLEND);
            glEnable(GL_LINE_SMOOTH);
            glEnable(GL_POINT_SMOOTH);
        }

        SHADER.setColorProcessing(SHADER.csColor);
        // draw my shots
        int maxShots = world->getMaxShots();
        int i;
        float muzzleHeight = BZDB.eval(StateDatabase::BZDB_MUZZLEHEIGHT);
        glColor4f(1.0f, 1.0f, 1.0f, muzzleHeight);
        for (i = 0; i < maxShots; i++)
        {
            auto shot = myTank->getShot(i);
            if (shot)
                shot->radarRender();
        }

        //draw world weapon shots
        WorldPlayer *worldWeapons = World::getWorld()->getWorldWeapons();
        maxShots = worldWeapons->getMaxShots();
        for (i = 0; i < maxShots; i++)
        {
            auto shot = worldWeapons->getShot(i);
            if (shot)
                shot->radarRender();
        }

        SHADER.setColorProcessing(SHADER.csNone);

        const float tankRadius = BZDBCache::tankRadius;
        // 'ps' is pixel scale, setup in render()
        const float minSize    = 1.5f + ps * BZDBCache::radarTankPixels;
        const float tankSize   = std::max(minSize, tankRadius);
        const float flagSize   = std::max(2.5f * tankRadius, 4.0f * ps);
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

            const auto &position = player->getPosition();

            glPushMatrix();

            // transform to the tanks location
            glTranslatef(position[0], position[1], 0.0f);

            if (!observer)
                drawTank(position, myAngle, tankSize, flagSize, player, true);
            else
                drawTank(position, myAngle, tankSize, flagSize, player, !useTankModels);

            glPopMatrix();
        }

        bool coloredShot = BZDB.isTrue("coloredradarshots");
        // draw other tanks' shells
        bool iSeeAll = myTank && (myTank->getFlag() == Flags::Seer);
        maxShots = World::getWorld()->getMaxShots();
        SHADER.setColorProcessing(SHADER.csColor);
        for (i = 0; i < curMaxPlayers; i++)
        {
            RemotePlayer* player = world->getPlayer(i);
            if (!player) continue;
            for (int j = 0; j < maxShots; j++)
            {
                auto shot = player->getShot(j);
                if (shot && (shot->getFlag() != Flags::InvisibleBullet || iSeeAll))
                {
                    glm::vec3 shotcolor;
                    if (coloredShot)
                    {
                        if (myTank->getFlag() == Flags::Colorblindness)
                            shotcolor = Team::getRadarColor(RogueTeam);
                        else
                            shotcolor = Team::getRadarColor(player->getTeam());
                    }
                    else
                        shotcolor = glm::vec3(1.0f);
                    glColor(shotcolor, muzzleHeight);
                    shot->radarRender();
                }
            }
        }

        // draw flags not on tanks.
        // draw them in reverse order so that the team flags
        // (which come first), are drawn on top of the normal flags.
        GLfloat s = BZDBCache::flagRadius > 3.0f * ps ? BZDBCache::flagRadius : 3.0f * ps;

        const int maxFlags = world->getMaxFlags();

        bool flagsToBeUpdated = world->areFlagsUpdated();

        static GLfloat oldSize = -1.0f;
        if (s != oldSize)
        {
            oldSize = s;
            flagsToBeUpdated = true;
        }

        static bool drawNormalFlags = true;
        if (drawNormalFlags != BZDB.isTrue("displayRadarFlags"))
        {
            drawNormalFlags  = !drawNormalFlags;
            flagsToBeUpdated = true;
        }

        static bool hideTeamFlagsOnRadar = true;
        if (hideTeamFlagsOnRadar != BZDB.isTrue(StateDatabase::BZDB_HIDETEAMFLAGSONRADAR))
        {
            hideTeamFlagsOnRadar = !hideTeamFlagsOnRadar;
            flagsToBeUpdated     = true;
        }

        static bool hideFlagsOnRadar = true;
        if (hideFlagsOnRadar != BZDB.isTrue(StateDatabase::BZDB_HIDEFLAGSONRADAR))
        {
            hideFlagsOnRadar = !hideFlagsOnRadar;
            flagsToBeUpdated = true;
        }

        if (flagsToBeUpdated)
        {
            std::vector<glm::vec3> vertexes;
            std::vector<glm::vec4> colors;
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
                const auto &flagcolor = flag.type->getRadarColor();
                auto color = glm::vec4(flagcolor, muzzleHeight);
                colors.push_back(color);
                vertexes.push_back(flag.position - glm::vec3(s, 0, 0));
                colors.push_back(color);
                vertexes.push_back(flag.position + glm::vec3(s, 0, 0));
                colors.push_back(color);
                vertexes.push_back(flag.position - glm::vec3(0, s, 0));
                colors.push_back(color);
                vertexes.push_back(flag.position + glm::vec3(0, s, 0));
            }
            flagsVBO = Vertex_Chunk(Vertex_Chunk::VC, vertexes.size());
            flagsVBO.vertexData(vertexes);
            flagsVBO.colorData(colors);
        }

        SHADER.setColorProcessing(SHADER.csColor);
        flagsVBO.draw(GL_LINES);
        SHADER.setColorProcessing(SHADER.csNone);
        // draw antidote flag
        const auto *antidotePos =
            LocalPlayer::getMyTank()->getAntidoteLocation();
        if (antidotePos)
        {
            glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
            drawFlag(*antidotePos);
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
            drawTank(myPos, myAngle, tankSize, flagSize, myTank, !useTankModels);

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


void RadarRenderer::buildWallsVBO()
{
    if (world == nullptr)
    {
        wallsVBO = Vertex_Chunk();
        return;
    }
    std::vector<glm::vec3> vertex;
    const ObstacleList& walls = OBSTACLEMGR.getWalls();
    int count = walls.size();
    for (int i = 0; i < count; i++)
    {
        const WallObstacle& wall = *((const WallObstacle*) walls[i]);
        const float wid = wall.getBreadth();
        const float c   = wid * cosf(wall.getRotation());
        const float s   = wid * sinf(wall.getRotation());
        const auto &pos = wall.getPosition();
        vertex.push_back(glm::vec3(pos.x - s, pos.y + c, 0.0f));
        vertex.push_back(glm::vec3(pos.x + s, pos.y - c, 0.0f));
    }
    wallsVBO = Vertex_Chunk(Vertex_Chunk::V, vertex.size());
    wallsVBO.vertexData(vertex);

    return;
}


void RadarRenderer::renderWalls()
{
    glColor4f(0.25f, 0.5f, 0.5f, 1.0f);
    wallsVBO.draw(GL_LINES);

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

    // do this after the GState setting
    if (smooth)
        glEnable(GL_POLYGON_SMOOTH);

    // setup texture generation
    int oldTexGen = SHADER.setTexGen(SHADER.texGenLinear);

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
    SHADER.setTexGen(oldTexGen);

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


void RadarRenderer::buildBoxPyrMeshVBO()
{
    meshVBO.clear();
    if (world == nullptr)
    {
        boxVBO.clear();
        pyrVBO.clear();
        boxOutlVBO.clear();
        pyrOutlVBO.clear();
        return;
    }


    int i;

    const bool  enhanced       = BZDBCache::radarStyle > 0;
    const ObstacleList& meshes = OBSTACLEMGR.getMeshes();
    int count = meshes.size();
    for (i = 0; i < count; i++)
    {
        const MeshObstacle* mesh = (const MeshObstacle*) meshes[i];
        int faces = mesh->getFaceCount();

        for (int f = 0; f < faces; f++)
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

            // draw death faces with a soupcon of red
            const PhysicsDriver* phydrv = PHYDRVMGR.getDriver(face->getPhysicsDriver());
            glm::vec4 color;
            if ((phydrv != NULL) && phydrv->getIsDeath())
                color = glm::vec4(0.75f, 0.25f, 0.25f, bh);
            else
                color = glm::vec4(0.25f, 0.5f,  0.5f,  bh);

            // draw the face as a triangle fan
            int vertexCount = face->getVertexCount();

            std::vector<glm::vec3> vertex;
            std::vector<glm::vec4> colors;
            for (int v = 0; v < vertexCount; v++)
            {
                const auto pos = face->getVertex(v);
                vertex.push_back(glm::vec3(pos.x, pos.y, z));
                colors.push_back(color);
            }
            Vertex_Chunk vbo(Vertex_Chunk::VC, vertexCount);
            vbo.vertexData(vertex);
            vbo.colorData(colors);
            meshVBO.push_back(std::move(vbo));
        }
    }
    meshChunkes.setChunkes(meshVBO);

    // draw box buildings.
    const ObstacleList& boxes = OBSTACLEMGR.getBoxes();
    count = boxes.size();
    boxVBO.resize(count);
    boxOutlVBO.resize(count);
    for (i = 0; i < count; i++)
    {
        const BoxBuilding& box = *((const BoxBuilding*) boxes[i]);
        if (box.isInvisible())
            continue;
        const float c = cosf(box.getRotation());
        const float s = sinf(box.getRotation());
        const float wx = c * box.getWidth(), wy = s * box.getWidth();
        const float hx = -s * box.getBreadth(), hy = c * box.getBreadth();
        const float bh = box.getHeight();
        const auto &pos = box.getPosition();
        boxVBO[i] = Vertex_Chunk(Vertex_Chunk::VC, 4);
        auto color = glm::vec4(0.25f, 0.5f, 0.5f, bh);
        glm::vec3 v[4];
        v[0] = glm::vec3(pos[0] - wx - hx, pos[1] - wy - hy, pos.z);
        v[1] = glm::vec3(pos[0] + wx - hx, pos[1] + wy - hy, pos.z);
        v[2] = glm::vec3(pos[0] - wx + hx, pos[1] - wy + hy, pos.z);
        v[3] = glm::vec3(pos[0] + wx + hx, pos[1] + wy + hy, pos.z);
        boxVBO[i].vertexData(v);
        glm::vec4 col[4];
        for (int j = 0; j < 4; j++)
            col[j] = color;
        boxVBO[i].colorData(col);
        boxOutlVBO[i] = Vertex_Chunk(Vertex_Chunk::VC, 4);
        std::swap(v[2], v[3]);
        boxOutlVBO[i].vertexData(v);
        boxOutlVBO[i].colorData(col);
    }
    boxChunkes.setChunkes(boxVBO);
    boxOutlChunkes.setChunkes(boxOutlVBO);

    // draw pyramid buildings
    const ObstacleList& pyramids = OBSTACLEMGR.getPyrs();
    count = pyramids.size();
    pyrVBO.resize(count);
    pyrOutlVBO.resize(count);
    for (i = 0; i < count; i++)
    {
        const PyramidBuilding& pyr = *((const PyramidBuilding*) pyramids[i]);
        const float c = cosf(pyr.getRotation());
        const float s = sinf(pyr.getRotation());
        const float wx = c * pyr.getWidth(), wy = s * pyr.getWidth();
        const float hx = -s * pyr.getBreadth(), hy = c * pyr.getBreadth();
        const float bh = pyr.getHeight();
        const auto &pos = pyr.getPosition();
        pyrVBO[i] = Vertex_Chunk(Vertex_Chunk::VC, 4);
        auto color = glm::vec4(0.25f, 0.5f, 0.5f, bh);
        glm::vec3 v[4];
        v[0] = glm::vec3(pos[0] - wx - hx, pos[1] - wy - hy, pos.z);
        v[1] = glm::vec3(pos[0] + wx - hx, pos[1] + wy - hy, pos.z);
        v[2] = glm::vec3(pos[0] - wx + hx, pos[1] - wy + hy, pos.z);
        v[3] = glm::vec3(pos[0] + wx + hx, pos[1] + wy + hy, pos.z);
        pyrVBO[i].vertexData(v);
        glm::vec4 col[4];
        for (int j = 0; j < 4; j++)
            col[j] = color;
        pyrVBO[i].colorData(col);
        pyrOutlVBO[i] = Vertex_Chunk(Vertex_Chunk::VC, 4);
        std::swap(v[2], v[3]);
        pyrOutlVBO[i].vertexData(v);
        pyrOutlVBO[i].colorData(col);
    }
    pyrChunkes.setChunkes(pyrVBO);
    pyrOutlChunkes.setChunkes(pyrOutlVBO);
}

void RadarRenderer::renderBoxPyrMesh()
{
    const bool  enhanced           = (BZDBCache::radarStyle > 0);
    static bool oldEnhanced        = false;
    static bool oldUseMeshForRadar = false;

    if ((enhanced != oldEnhanced)
            || (BZDBCache::useMeshForRadar != oldUseMeshForRadar))
    {
        buildBoxPyrMeshVBO();
        oldEnhanced        = enhanced;
        oldUseMeshForRadar = BZDBCache::useMeshForRadar;
    }

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

    SHADER.setColorProcessing(SHADER.csColTr);

    // draw box buildings.
    boxChunkes.draw(GL_TRIANGLE_STRIP);

    // draw pyramid buildings
    pyrChunkes.draw(GL_TRIANGLE_STRIP);

    // draw mesh obstacles
    if (smooth)
        glEnable(GL_POLYGON_SMOOTH);
    if (!enhanced)
        glDisable(GL_CULL_FACE);
    // Scale color so that objects that are close to tank's level are opaque
    meshChunkes.draw(GL_TRIANGLE_FAN);
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
        boxOutlChunkes.draw(GL_LINE_LOOP);

        pyrOutlChunkes.draw(GL_LINE_LOOP);
    }

    SHADER.setColorProcessing(SHADER.csNone);
    return;
}


void RadarRenderer::buildBasesAndTelesVBO()
{
    baseVBO.clear();
    telesVBO.clear();
    if (world == nullptr)
        return;

    int i;

    if (world->allowTeamFlags())
    {
        baseVBO.resize(NumTeams);
        for (i = 1; i < NumTeams; i++)
        {
            for (int j = 0;; j++)
            {
                glm::vec3 basePos;
                float rotation;
                float ww, bb, hh;
                const bool baseExist = world->getBase(i, j, basePos, rotation,
                                                      ww, bb, hh);
                if (!baseExist)
                    break;
                const float beta = atan2f(bb, ww);
                const float r = hypotf(ww, bb);
                glm::vec3 v[4];
                v[0] = glm::vec3(basePos.x + r * cosf(rotation + beta),
                                 basePos.y + r * sinf(rotation + beta),
                                 0.0f);
                v[1] = glm::vec3(basePos.x + r * cosf((float)(rotation - beta + M_PI)),
                                 basePos.y + r * sinf((float)(rotation - beta + M_PI)),
                                 0.0f);
                v[2] = glm::vec3(basePos.x + r * cosf((float)(rotation + beta + M_PI)),
                                 basePos.y + r * sinf((float)(rotation + beta + M_PI)),
                                 0.0f);
                v[3] = glm::vec3(basePos.x + r * cosf(rotation - beta),
                                 basePos.y + r * sinf(rotation - beta),
                                 0.0f);
                baseVBO[i].push_back(Vertex_Chunk(Vertex_Chunk::V, 4));
                baseVBO[i][j].vertexData(v);
            }
        }
    }

    const ObstacleList& teleporters = OBSTACLEMGR.getTeles();
    int count = teleporters.size();
    telesVBO.resize(count);
    for (i = 0; i < count; i++)
    {
        const Teleporter & tele = *((const Teleporter *) teleporters[i]);
        const float bh = tele.getHeight();
        const auto &pos = tele.getPosition();
        if (tele.isHorizontal ())
        {
            telesVBO[i] = Vertex_Chunk(Vertex_Chunk::VC, 10);

            const float c = cosf (tele.getRotation ());
            const float s = sinf (tele.getRotation ());
            const float wx = c * tele.getWidth (), wy = s * tele.getWidth ();
            const float hx = -s * tele.getBreadth (), hy = c * tele.getBreadth ();

            glm::vec3 v[10];
            v[0] = glm::vec3(pos[0] - wx - hx, pos[1] - wy - hy, pos.z);
            v[1] = glm::vec3(pos[0] + wx - hx, pos[1] + wy - hy, pos.z);
            v[2] = glm::vec3(pos[0] + wx - hx, pos[1] + wy - hy, pos.z);
            v[3] = glm::vec3(pos[0] + wx + hx, pos[1] + wy + hy, pos.z);
            v[4] = glm::vec3(pos[0] + wx + hx, pos[1] + wy + hy, pos.z);
            v[5] = glm::vec3(pos[0] - wx + hx, pos[1] - wy + hy, pos.z);
            v[6] = glm::vec3(pos[0] - wx + hx, pos[1] - wy + hy, pos.z);
            v[7] = glm::vec3(pos[0] - wx - hx, pos[1] - wy - hy, pos.z);
            v[8] = glm::vec3(pos[0] - wx - hx, pos[1] - wy - hy, pos.z);
            v[9] = glm::vec3(pos[0] - wx - hx, pos[1] - wy - hy, pos.z);
            telesVBO[i].vertexData(v);

            glm::vec4 col[10];
            const auto color = glm::vec4(1.0f, 1.0f, 1.0f, bh);
            for (int j = 0; j < 10; j++)
                col[j] = color;
            telesVBO[i].colorData(col);
        }
        else
        {
            telesVBO[i] = Vertex_Chunk(Vertex_Chunk::VC, 4);

            const float tw = tele.getBreadth ();
            const float c = tw * cosf (tele.getRotation ());
            const float s = tw * sinf (tele.getRotation ());

            glm::vec3 v[4];
            v[0] = glm::vec3(pos[0] - s, pos[1] + c, pos.z);
            v[1] = glm::vec3(pos[0] + s, pos[1] - c, pos.z);
            v[2] = glm::vec3(pos[0] + s, pos[1] - c, pos.z);
            v[3] = glm::vec3(pos[0] - s, pos[1] + c, pos.z);
            telesVBO[i].vertexData(v);

            glm::vec4 col[4];
            const auto color = glm::vec4(1.0f, 1.0f, 1.0f, bh);
            for (int j = 0; j < 4; j++)
                col[j] = color;
            telesVBO[i].colorData(col);
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
            glColor(Team::getRadarColor(TeamColor(i)));
            for (auto &vbo : baseVBO[i])
                vbo.draw(GL_LINE_LOOP);
        }
    }

    SHADER.setColorProcessing(SHADER.csColTr);
    // draw teleporters.  teleporters are pretty thin so use lines
    // (which, if longer than a pixel, are guaranteed to draw something;
    // not so for a polygon).  just in case the system doesn't correctly
    // filter the ends of line segments, we'll draw the line in each
    // direction (which degrades the antialiasing).  Newport graphics
    // is one system that doesn't do correct filtering.
    for (auto &vbo : telesVBO)
        vbo.draw(GL_LINES);
    SHADER.setColorProcessing(SHADER.csNone);

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
