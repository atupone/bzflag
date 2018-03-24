/* bzflag
 * Copyright (c) 2019-2026 Tim Riker
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
#include <string>

// common interface headers
#include "bzfgl.h"

// This class handles the vertex GL buffer and additional
// texCoord/Normal/Color GL buffers with support for static and dynamic streaming.
class VBO_Vertex : public VBO_Handler
{
public:
    enum Usage { Static, Dynamic };

    // Ctor: Set feature availability and usage mode
    VBO_Vertex(
        bool handleTexture,
        bool handleNormal,
        bool handleColor,
        bool interlaced,
        Usage usage = Static);

    ~VBO_Vertex() override = default;

    // Resizes the VBO capacity (doubles size starting from min 256)
    void resize() override;

    // Initializes GL buffer objects when context is ready
    void init() override;

    // Destroys GL buffer objects
    void destroy() override;

    // Returns structural name of the VBO layout
    std::string vboName() override;

    // CPU-side attribute data updates (sets dirty flag without immediate GPU copies)
    void vertexData(int index,  int size, const GLfloat vertices[]);
    void textureData(int index, int size, const GLfloat textures[]);
    void normalData(int index,  int size, const GLfloat normals[]);
    void colorData(int index,   int size, const GLfloat colors[]);

    // Flushes a specific slice/chunk range to GPU VRAM
    void flushRange(int index, int size);

    // Syncs interlaced CPU data to GPU in a single packed pass; No-op for non-interlaced
    inline void syncInterlacedRange(int index, int size)
    {
        if (interlaced)
            flushRange(index, size);
    }

    // Discards GPU memory allocation without copying CPU data
    void orphan();

    void enableArrays(bool texture, bool normal, bool color);

private:
    // These disable the usage of TexCoord/Normals/Colors,
    // but do not unbind the related buffer
    static void disableTextures();
    static void disableNormals();
    static void disableColors();

    // Low-level buffer transfer helper handling orphaning and partial writes
    void uploadBufferData(
        GLuint buffer,
        GLintptr offset,
        GLsizeiptr updateSize,
        const void* updateData);

    // Track active state across draw calls
    static GLuint actVerts;
    static GLuint actTxcds;
    static GLuint actNorms;
    static GLuint actColrs;
    static GLuint actIntrs;

    // Last bounded buffer
    static GLuint actBounded;

    // Feature availability
    bool handleTexture;
    bool handleNormal;
    bool handleColor;

    // GL buffer handles
    GLuint verts{0};
    GLuint txcds{0};
    GLuint norms{0};
    GLuint colrs{0};
    GLuint intrs{0};

    // Binding helpers
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
    std::vector<GLfloat> hostedInterlaced;

    bool vbosReady{false};

    // Interlacing stride configuration
    bool interlaced;
    int  interlacedSize;
    int  strideTexture;
    int  strideNormal;
    int  strideColor;

    // Optimization attributes for static vs dynamic streaming
    Usage  usageMode;
    GLenum glUsageHint;
};


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
