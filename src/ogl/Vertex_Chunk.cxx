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

// interface header
#include "Vertex_Chunk.h"

// System headers
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// Common headers
#include "VBO_Vertex.h"
#include "OpenGLGState.h"

VBO_Vertex *Vertex_Chunk::vbo[VLast] = { nullptr };
unsigned int Vertex_Chunk::refer[VLast];

Vertex_Chunk::Vertex_Chunk()
    : myVBO(nullptr)
    , indexSize(0)
    , myComp(V)
{
}

Vertex_Chunk::Vertex_Chunk(Component comp, unsigned int size)
    : myVBO(vbo[comp])
    , indexSize(size)
    , myComp(comp)
{
    if (!size)
    {
        // Do nothing if size 0
        myVBO = nullptr;
        return;
    }
    if (myVBO)
    {
        // A VBO with same features already exist
        // Incr reference count
        refer[comp]++;
        index = myVBO->alloc(size);
        return;
    }
    // Create a VBO with the specified features
    switch (comp)
    {
    case Component::V:
        myVBO = new VBO_Vertex(false, false, false);
        break;
    case Component::VC:
        myVBO = new VBO_Vertex(false, false, true);
        break;
    case Component::VN:
        myVBO = new VBO_Vertex(false, true, false);
        break;
    case Component::VT:
        myVBO = new VBO_Vertex(true, false, false);
        break;
    case Component::VTC:
        myVBO = new VBO_Vertex(true, false, true);
        break;
    case Component::VTN:
        myVBO = new VBO_Vertex(true, true, false);
        break;
    case Component::VTNC:
        myVBO = new VBO_Vertex(true, true, true);
        break;
    default:
        break;
    };
    // register it
    vbo[comp] = myVBO;
    // Init the VBO if GL Context is available
    if (OpenGLGState::haveGLContext())
        myVBO->init();
    // Reset reference count;
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

Vertex_Chunk::Vertex_Chunk(Vertex_Chunk&& other)
    : myVBO(other.myVBO)
    , index(other.index)
    , indexSize(other.indexSize)
    , myComp(other.myComp)
{
    other.myVBO = nullptr;
}

Vertex_Chunk& Vertex_Chunk::operator=(Vertex_Chunk&& other)
{
    if (this == &other)
        return *this;
    if (indexSize)
    {
        myVBO->free(index);
        if (!--refer[myComp])
        {
            delete myVBO;
            vbo[myComp] = nullptr;
        }
    }
    myVBO       = other.myVBO;
    index       = other.index;
    indexSize   = other.indexSize;
    myComp      = other.myComp;
    other.myVBO = nullptr;
    return *this;
}

void Vertex_Chunk::enableVertexOnly()
{
    if (myVBO)
        myVBO->enableVertexOnly();
}

void Vertex_Chunk::enableArrays()
{
    if (myVBO)
        myVBO->enableArrays();
}

void Vertex_Chunk::enableArrays(bool texture, bool normal, bool color)
{
    if (myVBO)
        myVBO->enableArrays(texture, normal, color);
}

void Vertex_Chunk::glDrawArrays(GLenum mode)
{
    ::glDrawArrays(mode, index, indexSize);
}

void Vertex_Chunk::glDrawArrays(GLenum mode, unsigned int size, unsigned int offset)
{
    ::glDrawArrays(mode, index + offset, size);
}

void Vertex_Chunk::vertexData(const GLfloat vertices[], int size)
{
    if (size < 0)
        size = indexSize;
    else if ((unsigned int)size > indexSize)
        size = indexSize;
    if (myVBO)
        myVBO->vertexData(index, size, vertices);
}

void Vertex_Chunk::textureData(const GLfloat textures[], int size)
{
    if (size < 0)
        size = indexSize;
    else if ((unsigned int)size > indexSize)
        size = indexSize;
    if (myVBO)
        myVBO->textureData(index, size, textures);
}

void Vertex_Chunk::normalData(const GLfloat normals[], int size)
{
    if (size < 0)
        size = indexSize;
    else if ((unsigned int)size > indexSize)
        size = indexSize;
    if (myVBO)
        myVBO->normalData(index, size, normals);
}

void Vertex_Chunk::colorData(const GLfloat colors[], int size)
{
    if (size < 0)
        size = indexSize;
    else if ((unsigned int)size > indexSize)
        size = indexSize;
    if (myVBO)
        myVBO->colorData(index, size, colors);
}

void Vertex_Chunk::vertexData(const GLfloat vertices[][3])
{
    vertexData(vertices[0]);
}

void Vertex_Chunk::textureData(const GLfloat textures[][2])
{
    textureData(textures[0]);
}

void Vertex_Chunk::vertexData(const glm::vec3 vertices[])
{
    vertexData(&vertices[0][0]);
}

void Vertex_Chunk::textureData(const glm::vec2 textures[])
{
    textureData(&textures[0][0]);
}

void Vertex_Chunk::normalData(const glm::vec3 normals[])
{
    normalData(&normals[0][0]);
}

void Vertex_Chunk::colorData(const glm::vec4 colors[])
{
    colorData(&colors[0][0]);
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

void Vertex_Chunk::vertexData(const std::vector<glm::vec3> vertices)
{
    if (vertices.empty())
        vertexData(static_cast<const GLfloat *>(nullptr), 0);
    else
        vertexData(&vertices[0][0], vertices.size());
}

void Vertex_Chunk::textureData(const std::vector<glm::vec2> textures)
{
    textureData(&textures[0][0], textures.size());
}

void Vertex_Chunk::normalData(const std::vector<glm::vec3> normals)
{
    normalData(&normals[0][0], normals.size());
}

void Vertex_Chunk::colorData(const std::vector<glm::vec4> colors)
{
    colorData(&colors[0][0], colors.size());
}

void Vertex_Chunk::draw(GLenum mode)
{
    enableArrays();
    glDrawArrays(mode);
}

void Vertex_Chunk::draw(GLenum mode, bool onlyVertex)
{
    if (onlyVertex)
        enableVertexOnly();
    else
        enableArrays();
    glDrawArrays(mode);
}

void Vertex_Chunk::draw(GLenum mode, bool onlyVertex, int size)
{
    if (onlyVertex)
        enableVertexOnly();
    else
        enableArrays();
    glDrawArrays(mode, size);
}

unsigned int Vertex_Chunk::getIndex() const
{
    return index;
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
