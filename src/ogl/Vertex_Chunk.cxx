/* bzflag
 * Copyright (c) 1993-2026 Tim Riker
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
#include "Vertex_Chunk.h"

// System headers
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// Common headers
#include "VBO_Vertex.h"
#include "OpenGLGState.h"

VBO_Vertex* Vertex_Chunk::staticVBO[VLast] = { nullptr };
VBO_Vertex* Vertex_Chunk::dynamicVBO[VLast] = { nullptr };
unsigned int Vertex_Chunk::staticRefer[VLast];
unsigned int Vertex_Chunk::dynamicRefer[VLast];

Vertex_Chunk::Vertex_Chunk(Component comp, unsigned int size, Usage usage)
    : indexSize(size)
    , myComp(comp)
    , myUsage(usage)
    , vbo((usage == Dynamic) ? dynamicVBO : staticVBO)
    , refer((usage == Dynamic) ? dynamicRefer : staticRefer)
    , myVBO(vbo[comp])
{
    if (!indexSize)
        return;

    if (myVBO)
    {
        // A VBO with same features already exists
        // Incr reference count
        refer[comp]++;
        index = myVBO->alloc(indexSize);
        return;
    }

    // Create a VBO with the specified features
    const bool interlaced = true;
    myVBO = createVBOForComponent(comp, interlaced);

    // register it
    vbo[comp] = myVBO;

    // Init the VBO if GL Context is available
    if (OpenGLGState::haveGLContext())
        myVBO->init();

    // Reset reference count
    refer[comp] = 1;

    // Allocate chunk of memory
    index = myVBO->alloc(indexSize);
}

Vertex_Chunk::~Vertex_Chunk()
{
    if (!myVBO)
        return;

    myVBO->free(index);
    if (!--refer[myComp])
    {
        delete myVBO;
        vbo[myComp] = nullptr;
    }
}

Vertex_Chunk::Vertex_Chunk(Vertex_Chunk&& other) noexcept
    : indexSize(other.indexSize)
    , index{other.index}
    , myComp(other.myComp)
    , myUsage(other.myUsage)
    , vbo(other.vbo)
    , refer(other.refer)
    , myVBO(other.myVBO)
{
    other.myVBO = nullptr;
}

Vertex_Chunk& Vertex_Chunk::operator=(Vertex_Chunk&& other) noexcept
{
    if (this == &other)
        return *this;
    if (myVBO)
    {
        myVBO->free(index);
        if (!--refer[myComp])
        {
            delete myVBO;
            vbo[myComp] = nullptr;
        }
    }
    indexSize   = other.indexSize;
    index       = other.index;
    myComp      = other.myComp;
    myUsage     = other.myUsage;
    vbo         = other.vbo;
    refer       = other.refer;
    myVBO       = other.myVBO;

    other.myVBO = nullptr;
    return *this;
}

VBO_Vertex* Vertex_Chunk::createVBOForComponent(Component comp, bool interlaced)
{
    switch (comp)
    {
        case Component::V:
            return new VBO_Vertex(false, false, false, interlaced, (myUsage == Dynamic) ? VBO_Vertex::Dynamic : VBO_Vertex::Static);
        case Component::VC:
            return new VBO_Vertex(false, false, true, interlaced, (myUsage == Dynamic) ? VBO_Vertex::Dynamic : VBO_Vertex::Static);
        case Component::VN:
            return new VBO_Vertex(false, true, false, interlaced, (myUsage == Dynamic) ? VBO_Vertex::Dynamic : VBO_Vertex::Static);
        case Component::VT:
            return new VBO_Vertex(true, false, false, interlaced, (myUsage == Dynamic) ? VBO_Vertex::Dynamic : VBO_Vertex::Static);
        case Component::VTC:
            return new VBO_Vertex(true, false, true, interlaced, (myUsage == Dynamic) ? VBO_Vertex::Dynamic : VBO_Vertex::Static);
        case Component::VTN:
            return new VBO_Vertex(true, true, false, interlaced, (myUsage == Dynamic) ? VBO_Vertex::Dynamic : VBO_Vertex::Static);
        case Component::VTNC:
            return new VBO_Vertex(true, true, true, interlaced, (myUsage == Dynamic) ? VBO_Vertex::Dynamic : VBO_Vertex::Static);
        default:
            return nullptr;
    };

    return nullptr;
}

void Vertex_Chunk::enableArrays(bool texture, bool normal, bool color)
{
    if (myVBO)
        myVBO->enableArrays(texture, normal, color);
}

// ----------------------------------------------------------------------------
// Data Setter Forwarders
// ----------------------------------------------------------------------------

void Vertex_Chunk::vertexData(const GLfloat vertices[], int size)
{
    if (size < 0 || (unsigned int)size > indexSize)
        size = indexSize;
    if (myVBO)
        myVBO->vertexData(index, size, vertices);
}

void Vertex_Chunk::textureData(const GLfloat textures[], int size)
{
    if (size < 0 || (unsigned int)size > indexSize)
        size = indexSize;
    if (myVBO)
        myVBO->textureData(index, size, textures);
}

void Vertex_Chunk::normalData(const GLfloat normals[], int size)
{
    if (size < 0 || (unsigned int)size > indexSize)
        size = indexSize;
    if (myVBO)
        myVBO->normalData(index, size, normals);
}

void Vertex_Chunk::colorData(const GLfloat colors[], int size)
{
    if (size < 0 || (unsigned int)size > indexSize)
        size = indexSize;
    if (myVBO)
        myVBO->colorData(index, size, colors);
}

void Vertex_Chunk::vertexData(const GLfloat vertices[][3], int size)
{
    vertexData(vertices[0], size);
}

void Vertex_Chunk::textureData(const GLfloat textures[][2], int size)
{
    textureData(textures[0], size);
}

void Vertex_Chunk::normalData(const GLfloat normals[][3], int size)
{
    normalData(normals[0], size);
}

void Vertex_Chunk::colorData(const GLfloat colors[][4], int size)
{
    colorData(colors[0], size);
}

void Vertex_Chunk::vertexData(const glm::vec3 vertices[], int size)
{
    vertexData(&vertices[0][0], size);
}

void Vertex_Chunk::textureData(const glm::vec2 textures[], int size)
{
    textureData(&textures[0][0], size);
}

void Vertex_Chunk::normalData(const glm::vec3 normals[], int size)
{
    normalData(&normals[0][0], size);
}

void Vertex_Chunk::colorData(const glm::vec4 colors[], int size)
{
    colorData(&colors[0][0], size);
}

void Vertex_Chunk::vertexData(const std::vector<glm::vec3>& vertices)
{
    if (vertices.empty())
        vertexData(static_cast<const GLfloat *>(nullptr), 0);
    else
        vertexData(vertices.data(), vertices.size());
}

void Vertex_Chunk::textureData(const std::vector<glm::vec2>& textures)
{
    textureData(textures.data(), textures.size());
}

void Vertex_Chunk::normalData(const std::vector<glm::vec3>& normals)
{
    normalData(normals.data(), normals.size());
}

void Vertex_Chunk::colorData(const std::vector<glm::vec4>& colors)
{
    if (colors.empty())
        colorData(static_cast<const GLfloat *>(nullptr), 0);
    else
        colorData(colors.data(), colors.size());
}

void Vertex_Chunk::syncInterlaced()
{
    if (myVBO)
        myVBO->syncInterlacedRange(index, indexSize);
}

void Vertex_Chunk::orphanDynamicBuffers()
{
    for (int i = 0; i < VLast; ++i)
    {
        if (dynamicVBO[i])
            // Re-allocate the underlying OpenGL buffer to orphan old VRAM
            dynamicVBO[i]->orphan();
    }
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
