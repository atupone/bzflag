/* bzflag
 * Copyright (c) 2019-2019 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#pragma once

// inherits from
#include "VBO_Handler.h"

// system interface headers
#include <vector>

// common interface headers
#include "bzfgl.h"

// This class handle the vertex GL buffer and additional
// texCoord/Normal/Color GL buffers
// Vertex Buffer is designed to have 3 coords
// Normals the same
// TexCoords 2 coords
// Colors 4 coords
class VBO_Vertex: public VBO_Handler
{
public:
    // Ctor: Set the availability of the extra buffers
    VBO_Vertex(
        bool handleTexture,
        bool handleNormal,
        bool handleColor);
    ~VBO_Vertex() = default;

    // This should be called when there is not enough space on the current
    // VBO, so try to double the size of it.
    // It start from 256
    void resize() override;

    // Init will be called when a GL Context is ready
    // It creates all the GL buffers requested,
    void init() override;
    // Destroy will delete the GL Buffers
    void destroy() override;
    // Name of the VBO
    std::string vboName() override;

    // These are for the case the caller provides a pointer to the first element
    void vertexData(int index,  int size, const GLfloat vertices[]);
    void textureData(int index, int size, const GLfloat textures[]);
    void normalData(int index,  int size, const GLfloat normals[]);
    void colorData(int index,   int size, const GLfloat colors[]);

    // This method bind all the buffers, to be ready for drawing
    void enableArrays();
    // This method will select which buffer should be enabled
    // Vertex is always enabled
    void enableArrays(bool texture, bool normal, bool color);
    // This method will enable the vertex buffer only (used for shadows
    void enableVertexOnly();

private:
    // These disable the usage of TexCoord/Normals/Colors,
    // but do not unbind the related buffer
    static void disableTextures();
    static void disableNormals();
    static void disableColors();

    // The binded vertex/texture/normals/colors arrays by buffer
    static GLuint actVerts;
    static GLuint actTxcds;
    static GLuint actNorms;
    static GLuint actColrs;

    // Last bounded buffer
    static GLuint actBounded;

    // Taken from the ctor
    bool handleTexture;
    bool handleNormal;
    bool handleColor;

    // The GL buffers created from the class
    // After enableArray these are copied in the static one
    GLuint verts;
    GLuint txcds;
    GLuint norms;
    GLuint colrs;

    // Bind vertex Buffer
    void enableVertex();
    // Bind the relevant Buffer and enable its usage
    void enableTextures();
    void enableNormals();
    void enableColors();

    void bindBuffer(GLuint buffer);
    void deleteBuffer(GLuint buffer);

    // mantain the status of the enabling of the buffers
    static bool textureEnabled;
    static bool normalEnabled;
    static bool colorEnabled;

    // in CPU memory contents of GPU memory
    std::vector<GLfloat> hostedVertices;
    std::vector<GLfloat> hostedTextures;
    std::vector<GLfloat> hostedNormals;
    std::vector<GLfloat> hostedColors;

    bool vbosReady;
};

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
