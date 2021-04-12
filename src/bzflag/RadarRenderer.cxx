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
#include "RadarShader.h"

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
        meshVBOEnhanced.clear();
        boxVBO.clear();
        boxOutVBO.clear();
        pyrVBO.clear();
        pyrOutVBO.clear();
        return;
    }

    std::vector<glm::vec3> vertexes;
    std::vector<glm::vec4> colors;
    glm::vec3 vertex;
    glm::vec4 color;

    const ObstacleList& meshes = OBSTACLEMGR.getMeshes();
    int count = meshes.size();

    meshVBO.resize(count);
    meshVBOEnhanced.resize(count);

    for (int i = 0; i < count; i++)
    {
        const MeshObstacle* mesh = (const MeshObstacle*) meshes[i];
        int faces = mesh->getFaceCount();
        float mz  = mesh->getPosition()[2];
        float mbh = mesh->getSize()[2];

        bool first = true;

        vertexes.clear();
        colors.clear();

        for (int f = 0; f < faces; f++)
        {
            const MeshFace* face = mesh->getFace(f);
            float z              = face->getPosition()[2];
            float bh             = face->getSize()[2];

            if (BZDBCache::useMeshForRadar)
            {
                z  = mz;
                bh = mbh;
            }

            // draw death faces with a soupcon of red
            const PhysicsDriver* phydrv = PHYDRVMGR.getDriver(face->getPhysicsDriver());
            if ((phydrv != NULL) && phydrv->getIsDeath())
                color = glm::vec4(0.75f, 0.25f, 0.25f, bh);
            else
                color = glm::vec4(0.25f, 0.5f,  0.5f,  bh);

            if (first)
                first = false;
            else
            {
                vertexes.push_back(vertex);
                colors.push_back(color);
                if (vertexes.size() % 2 == 0)
                {
                    vertexes.push_back(vertex);
                    colors.push_back(color);
                }
                const float *pos = face->getVertex(0);
                vertex = glm::vec3(pos[0], pos[1], z);
                vertexes.push_back(vertex);
                colors.push_back(color);
            }

            // draw the face as a triangle fan
            int vertexCount = face->getVertexCount();
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
                vertex = glm::vec3(pos[0], pos[1], z);
                vertexes.push_back(vertex);
                colors.push_back(color);
            }
        }
        meshVBO[i] = Vertex_Chunk(Vertex_Chunk::VC, vertexes.size());
        meshVBO[i].colorData(colors);
        meshVBO[i].vertexData(vertexes);
    }

    for (int i = 0; i < count; i++)
    {
        const MeshObstacle* mesh = (const MeshObstacle*) meshes[i];
        float mz                 = mesh->getPosition()[2];
        float mbh                = mesh->getSize()[2];
        int faces                = mesh->getFaceCount();

        bool first = true;

        vertexes.clear();
        colors.clear();

        for (int f = 0; f < faces; f++)
        {
            const MeshFace* face = mesh->getFace(f);
            float z              = face->getPosition()[2];
            float bh             = face->getSize()[2];

            if (BZDBCache::useMeshForRadar)
            {
                z  = mz;
                bh = mbh;
            }

            if (face->getPlane()[2] <= 0.0f)
                continue;
            const BzMaterial* bzmat = face->getMaterial();
            if ((bzmat != NULL) && bzmat->getNoRadar())
                continue;

            // draw death faces with a soupcon of red
            const PhysicsDriver* phydrv = PHYDRVMGR.getDriver(face->getPhysicsDriver());
            if ((phydrv != NULL) && phydrv->getIsDeath())
                color = glm::vec4(0.75f, 0.25f, 0.25f, bh);
            else
                color = glm::vec4(0.25f, 0.5f,  0.5f,  bh);

            if (first)
                first = false;
            else
            {
                vertexes.push_back(vertex);
                colors.push_back(color);
                if (vertexes.size() % 2 == 0)
                {
                    vertexes.push_back(vertex);
                    colors.push_back(color);
                }
                const float *pos = face->getVertex(0);
                vertex = glm::vec3(pos[0], pos[1], z);
                vertexes.push_back(vertex);
                colors.push_back(color);
            }

            int vertexCount = face->getVertexCount();
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
                const float *pos = face->getVertex(p);
                vertex = glm::vec3(pos[0], pos[1], z);
                vertexes.push_back(vertex);
                colors.push_back(color);
            }
        }
        meshVBOEnhanced[i] = Vertex_Chunk(Vertex_Chunk::VC, vertexes.size());
        meshVBOEnhanced[i].colorData(colors);
        meshVBOEnhanced[i].vertexData(vertexes);
    }

    // draw box buildings.
    const ObstacleList& boxes = OBSTACLEMGR.getBoxes();
    count = boxes.size();

    boxVBO.resize(count);
    boxOutVBO.resize(count);

    glm::vec3 vertices[4];
    glm::vec4 boxPyrColors[4];
    for (int i = 0; i < 4; i++)
        boxPyrColors[i] = glm::vec4(0.25f, 0.5f,  0.5f, 0.0f);
    for (int i = 0; i < count; i++)
    {
        boxVBO[i]    = Vertex_Chunk();
        boxOutVBO[i] = Vertex_Chunk();
        const BoxBuilding& box = *((const BoxBuilding*) boxes[i]);
        if (box.isInvisible())
            continue;
        boxVBO[i]    = Vertex_Chunk(Vertex_Chunk::VC, 4);
        boxOutVBO[i] = Vertex_Chunk(Vertex_Chunk::VC, 4);
        const float  wx = box.getWidth();
        const float  hy = box.getBreadth();
        const float* pos = box.getPosition();
        const float  rot = box.getRotation();
        const float  bh = box.getHeight();
        auto unity = glm::mat4(1.0f);
        auto translate(glm::translate(unity, glm::make_vec3(pos)));
        auto rotate(glm::rotate(translate, rot, glm::vec3(0.0f, 0.0f, 1.0f)));
        for (int j = 0; j < 4; j++)
            boxPyrColors[j].a = bh;
        vertices[0] = glm::vec3(rotate * glm::vec4(-wx, -hy, 0.0f, 1.0f));
        vertices[1] = glm::vec3(rotate * glm::vec4( wx, -hy, 0.0f, 1.0f));
        vertices[2] = glm::vec3(rotate * glm::vec4(-wx,  hy, 0.0f, 1.0f));
        vertices[3] = glm::vec3(rotate * glm::vec4( wx,  hy, 0.0f, 1.0f));
        boxVBO[i].vertexData(vertices);
        boxVBO[i].colorData(boxPyrColors);
        vertices[2] = glm::vec3(rotate * glm::vec4( wx,  hy, 0.0f, 1.0f));
        vertices[3] = glm::vec3(rotate * glm::vec4(-wx,  hy, 0.0f, 1.0f));
        boxOutVBO[i].vertexData(vertices);
        boxOutVBO[i].colorData(boxPyrColors);
    }

    // draw pyramid buildings
    const ObstacleList& pyramids = OBSTACLEMGR.getPyrs();
    count = pyramids.size();

    pyrVBO.resize(count);
    pyrOutVBO.resize(count);

    for (int i = 0; i < count; i++)
    {
        pyrVBO[i]    = Vertex_Chunk(Vertex_Chunk::VC, 4);
        pyrOutVBO[i] = Vertex_Chunk(Vertex_Chunk::VC, 4);
        const PyramidBuilding& pyr = *((const PyramidBuilding*) pyramids[i]);
        const float  wx  = pyr.getWidth();
        const float  hy  = pyr.getBreadth();
        const float* pos = pyr.getPosition();
        const float  rot = pyr.getRotation();
        const float bh = pyr.getHeight();
        auto unity = glm::mat4(1.0f);
        auto translate(glm::translate(unity, glm::make_vec3(pos)));
        auto rotate(glm::rotate(translate, rot, glm::vec3(0.0f, 0.0f, 1.0f)));
        for (int j = 0; j < 4; j++)
            boxPyrColors[j].a = bh;
        vertices[0] = glm::vec3(rotate * glm::vec4(-wx, -hy, 0.0f, 1.0f));
        vertices[1] = glm::vec3(rotate * glm::vec4( wx, -hy, 0.0f, 1.0f));
        vertices[2] = glm::vec3(rotate * glm::vec4(-wx,  hy, 0.0f, 1.0f));
        vertices[3] = glm::vec3(rotate * glm::vec4( wx,  hy, 0.0f, 1.0f));
        pyrVBO[i].vertexData(vertices);
        pyrVBO[i].colorData(boxPyrColors);
        vertices[2] = glm::vec3(rotate * glm::vec4( wx,  hy, 0.0f, 1.0f));
        vertices[3] = glm::vec3(rotate * glm::vec4(-wx,  hy, 0.0f, 1.0f));
        pyrOutVBO[i].vertexData(vertices);
        pyrOutVBO[i].colorData(boxPyrColors);
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


glm::vec3 RadarRenderer::getTankColor(const Player* player)
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
    return color;
}

void RadarRenderer::drawTank(const float pos[3], float myAngle,
                             float tankSize, float flagSize,
                             const Player* player, bool useDimensions)
{
    RADARSHADER.setOffset(pos[0], pos[1], 0.0f);
    FlagType *flag = player->getFlag();

    if (flag != Flags::Null)
    {
        auto c = flag->getRadarColor();
        RADARSHADER.setObjColor(c);
        RADARSHADER.setDimsRot(flagSize, flagSize, myAngle);
        DRAWER.cross();
    }

    if (pos[2] < 0.0f)
        tankSize = 0.5f;

    auto color = getTankColor(player);

    const float tankAngle = player->getAngle();

    if (useDimensions && useTankModels)
    {
        RADARSHADER.setRadarKind(RADARSHADER.emulFixed);
        glPushMatrix();
        glTranslatef(pos[0], pos[1], 0.0f);
        glRotatef(float(tankAngle * 180.0 / M_PI), 0.0f, 0.0f, 1.0f);
        drawFancyTank(player);
        glPopMatrix();
        RADARSHADER.setRadarKind(RADARSHADER.solidColor);
    }

    RADARSHADER.setObjColor(color);

    // draw the tank
    if (!useDimensions)
    {
        RADARSHADER.setDimsRot(tankSize, tankSize, myAngle);
        DRAWER.simmetricRect();
    }
    else if (!useTankModels)
    {
        const float* dims = player->getDimensions();
        RADARSHADER.setDimsRot(dims[0], dims[1], tankAngle);
        DRAWER.simmetricRect();
    }

    // adjust with height box size
    const float boxHeight = BZDB.eval(StateDatabase::BZDB_BOXHEIGHT);
    tankSize = tankSize * (1.0f + (0.5f * (pos[2] / boxHeight)));

    // draw the height box
    // align to the screen axes
    RADARSHADER.setDimsRot(tankSize, tankSize, myAngle);
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
    RADARSHADER.setOffset(pos[0], pos[1], pos[2]);
    DRAWER.cross();
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

    RADARSHADER.push();

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

    RADARSHADER.setRadarRange(radarRange);

    // if jammed then draw white noise.  occasionally draw a good frame.
    if (jammed && (bzfrand() > decay))
    {
        TextureManager &tm = TextureManager::instance();
        int noiseTexture = tm.getTextureID( "noise" );

        if ((noiseTexture >= 0)
                && (BZDBCache::texture || renderer.useQuality() > 0))
        {
            RADARSHADER.setRadarKind(RADARSHADER.jammed);

            const int sequences = 10;

            static glm::vec4 np[] =
            {
                {0,         0,     1,  1},
                {1,         1,    -1, -1},
                {0.5f,   0.5f,     1,  1},
                {1.5f,   1.5f,    -1, -1},
                {0.25f, 0.25f,     1,  1},
                {1.25f, 1.25f,    -1,  1},
                {0,      0.5f,     1,  1},
                {1,      1.5f,    -1, -1},
                {0.5f,      0,     1,  1},
                {1.4f,      1, -0.9f, -1},
                {0.75f, 0.75f,     1,  1},
                {1.75f, 1.75f,    -1, -1}
            };

            tm.bind(noiseTexture);

            if (renderer.useQuality() > 0)
            {
                int noisePattern = int(floor(sequences * bzfrand()));
                RADARSHADER.setNoisePattern(np[noisePattern]);
            }
            else
                RADARSHADER.setNoisePattern(np[0]);
            DRAWER.simmetricTexturedRect();
        }
        if (decay > 0.015f) decay *= 0.5f;
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
            const float fovx = renderer.getViewFrustum().getFOVx();
            const float viewWidth = radarRange * tanf(0.5f * fovx);
            RADARSHADER.setRadarKind(RADARSHADER.viewLine);
            RADARSHADER.setViewWidth(viewWidth);
            DRAWER.viewAngle();
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

        RADARSHADER.setRadarKind(RADARSHADER.shot);
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

        // draw my shots
        int maxShots = world->getMaxShots();
        int i;
        float muzzleHeight = BZDB.eval(StateDatabase::BZDB_MUZZLEHEIGHT);
        RADARSHADER.setColorHeight(glm::vec3(1.0f, 1.0f, 1.0f), muzzleHeight);
        for (i = 0; i < maxShots; i++)
        {
            const ShotPath* shot = myTank->getShot(i);
            if (shot)
                shot->radarRender();
        }

        //draw world weapon shots
        WorldPlayer *worldWeapons = World::getWorld()->getWorldWeapons();
        maxShots = worldWeapons->getMaxShots();
        for (i = 0; i < maxShots; i++)
        {
            const ShotPath* shot = worldWeapons->getShot(i);
            if (shot)
                shot->radarRender();
        }
        glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);

        RADARSHADER.setRadarKind(RADARSHADER.solidColor);
        // 'ps' is pixel scale, setup in render()
        const float tankRadius = BZDBCache::tankRadius;
        float minSize = 1.5f + (ps * BZDBCache::radarTankPixels);
        const float tankSize = std::max(tankRadius, minSize);
        const float flagSize = std::max(2.5f * tankRadius, 4.0f * ps);
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

            drawTank(position, myAngle, tankSize, flagSize, player,
                     observer && useTankDimensions);
        }

        RADARSHADER.setRadarKind(RADARSHADER.shot);
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

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
                if (!shot)
                    continue;
                if (shot->getFlag() == Flags::InvisibleBullet && !iSeeAll)
                    continue;
                auto myTeam = myTank->getFlag() == Flags::Colorblindness
                              ? RogueTeam
                              : player->getTeam();
                glm::vec3 shotcolor = coloredShot
                                      ? Team::getRadarColor(myTeam)
                                      : glm::vec3(1.0f, 1.0f, 1.0f);
                RADARSHADER.setColorHeight(shotcolor, muzzleHeight);
                shot->radarRender();
            }
        }

        glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
        RADARSHADER.setRadarKind(RADARSHADER.flag);
        RADARSHADER.setColorProcessing(colorScale);

        // draw flags not on tanks.
        // draw them in reverse order so that the team flags
        // (which come first), are drawn on top of the normal flags.
        const int maxFlags = world->getMaxFlags();
        const bool drawNormalFlags = BZDB.isTrue("displayRadarFlags");
        const bool hideTeamFlagsOnRadar = BZDB.isTrue(StateDatabase::BZDB_HIDETEAMFLAGSONRADAR);
        const bool hideFlagsOnRadar = BZDB.isTrue(StateDatabase::BZDB_HIDEFLAGSONRADAR);
        GLfloat s = std::max(float(BZDBCache::flagRadius), 3.0f * ps);
        RADARSHADER.setDimsRot(s, s, 0);
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
                if (flag.type->flagTeam != ::NoTeam)
                    continue;
            if (hideFlagsOnRadar)
                if (flag.type)
                    continue;
            // Flags change color by height
            const auto flagcolor = flag.type->getRadarColor();
            RADARSHADER.setColorHeight(flagcolor, muzzleHeight);
            drawFlag(flag.position);
        }
        RADARSHADER.setColorProcessing(noChange);
        // draw antidote flag
        const float* antidotePos =
            LocalPlayer::getMyTank()->getAntidoteLocation();
        if (antidotePos)
        {
            RADARSHADER.setColorHeight(glm::vec3(1.0f, 1.0f, 0.0f), 0.0f);
            drawFlag(antidotePos);
        }

        if (!observer)
        {
            RADARSHADER.setRadarKind(RADARSHADER.solidColor);

            // my tank
            drawTank(myPos, myAngle, tankSize, flagSize, myTank,
                     useTankDimensions);

            // re-setup the blending function
            // (was changed by drawing jump jets)
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        // draw these markers above all others always centered
        glPopMatrix();

        // north marker
        RADARSHADER.setRadarKind(RADARSHADER.solidColor);
        GLfloat ns = 0.05f * radarRange, ny = 0.9f * radarRange;
        RADARSHADER.setObjColor(glm::vec3(1.0f));
        RADARSHADER.setOffset(0.0f, ny, 0.0f);
        RADARSHADER.setDimsRot(ns, ns, 0.0f);
        DRAWER.north();

        // always up
        glPopMatrix();

        // forward tick
        RADARSHADER.setOffset(0.0f, radarRange - 4.0f * ps, 0.0f);
        RADARSHADER.setDimsRot(0.0f, 3.0 * ps, 0.0f);
        DRAWER.asimmetricLineY();

        glPopMatrix();

        if (dimming > 0.0f)
        {
            if (!smooth)
                glEnable(GL_BLEND);
            RADARSHADER.setRadarKind(RADARSHADER.dimming);
            // darken the entire radar if we're dimmed
            // we're drawing positively, so dimming is actually an opacity
            RADARSHADER.setAlphaDim(1.0f - dimming);
            DRAWER.simmetricRect();
        }
        glDisable(GL_BLEND);
        glDisable(GL_LINE_SMOOTH);
        glDisable(GL_POINT_SMOOTH);
    }

    RADARSHADER.pop();

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

    const float vfz = RENDERER.getViewFrustum().getEye()[2];
    RADARSHADER.setZTank(vfz);

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
    RADARSHADER.setRadarKind(RADARSHADER.solidColor);
    const ObstacleList& walls = OBSTACLEMGR.getWalls();
    int count = walls.size();
    RADARSHADER.setObjColor(glm::vec3(0.25f, 0.5f, 0.5f));
    for (int i = 0; i < count; i++)
    {
        const WallObstacle& wall = *((const WallObstacle*) walls[i]);
        const float wid = wall.getBreadth();
        const float* pos = wall.getPosition();
        const float  rot = wall.getRotation();
        RADARSHADER.setOffset(pos[0], pos[1], 0.0f);
        RADARSHADER.setDimsRot(wid, wid, rot);
        DRAWER.simmetricLineY();
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

    RADARSHADER.setRadarKind(RADARSHADER.fastRadar);

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

//  if (!BZDB.isTrue("visualRadar")) {
    ViewFrustum radarClipper;
    radarClipper.setOrthoPlanes(RENDERER.getViewFrustum(), _range, _range);
    RENDERER.getSceneDatabase()->renderRadarNodes(radarClipper);
//  } else {
//    RENDERER.getSceneDatabase()->renderRadarNodes(RENDERER.getViewFrustum());
//  }

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
    const bool enhanced = (BZDBCache::radarStyle > 0);

    RADARSHADER.setRadarKind(RADARSHADER.enhancedRadar);
    if (enhanced)
        RADARSHADER.setColorProcessing(colorTrans);
    else
        RADARSHADER.setColorProcessing(transScale);

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
    for (auto &item : boxVBO)
        item.draw(GL_TRIANGLE_STRIP);

    // draw pyramid buildings
    for (auto &item : pyrVBO)
        item.draw(GL_TRIANGLE_STRIP);

    // draw mesh obstacles
    if (smooth)
        glEnable(GL_POLYGON_SMOOTH);
    if (!enhanced)
        glDisable(GL_CULL_FACE);
    if (enhanced)
        for (auto &item : meshVBOEnhanced)
            item.draw(GL_TRIANGLE_STRIP);
    else
        for (auto &item : meshVBO)
            item.draw(GL_TRIANGLE_STRIP);

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
        for (auto &item : boxOutVBO)
            item.draw(GL_LINE_LOOP);

        for (auto &item : pyrOutVBO)
            item.draw(GL_LINE_LOOP);
    }

    return;
}


void RadarRenderer::renderBasesAndTeles()
{
    int i;

    // draw team bases
    if (world->allowTeamFlags())
    {
        RADARSHADER.setRadarKind(RADARSHADER.solidColor);
        for (i = 1; i < NumTeams; i++)
        {
            auto c = Team::getRadarColor(TeamColor(i));
            for (int j = 0;; j++)
            {
                const float *base = world->getBase(i, j);
                if (base == NULL)
                    break;
                RADARSHADER.setObjColor(c);
                RADARSHADER.setOffset(base[0], base[1], 0.0f);
                RADARSHADER.setDimsRot(base[4], base[5], base[3]);
                DRAWER.simmetricSquareLoop();
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
    if (count)
        RADARSHADER.setRadarKind(RADARSHADER.tele);
    for (i = 0; i < count; i++)
    {
        const Teleporter & tele = *((const Teleporter *) teleporters[i]);
        const float bh = tele.getHeight ();
        const float *pos = tele.getPosition ();
        const float rot = tele.getRotation();
        const float tw = tele.getBreadth();
        const float wx = tele.getWidth();
        RADARSHADER.setTeleData(pos[2], bh);
        RADARSHADER.setOffset(pos[0], pos[1], 0.0f);
        if (tele.isHorizontal ())
        {
            RADARSHADER.setDimsRot(wx, tw, rot);
            DRAWER.simmetricSquareLoop();
        }
        else
        {
            RADARSHADER.setDimsRot(tw, tw, rot);
            DRAWER.simmetricLineY();
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
