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

#version 110

uniform float   zTank;
uniform int     radarKind;
uniform bool    tsProc;
uniform bool    csProc;
uniform float   radarRange;
uniform vec4    noisePattern;
uniform float   viewWidth;
uniform vec4    baseData[2];
uniform vec2    teleData;
uniform int     shotType;
uniform vec4    segmentedData[2];
uniform vec4    colorHeight;
uniform vec3    dimsRot;
uniform vec3    offset;

const int enhancedRadar = 0;
const int fastRadar     = 1;
const int emulFixed     = 2;
const int jammed        = 3;
const int viewLine      = 4;
const int flag          = 5;
const int dimming       = 6;
const int tele          = 7;
const int shot          = 8;
const int solidColor    = 10;

const int segmentedType = 0;
const int sizedBullet   = 1;
const int missileBullet = 2;
const int noType        = 3;

void boxPyrMesh()
{
    vec4 vertex = gl_ModelViewMatrix * gl_Vertex;

    const float colorFactor = 40.0;

    float scale;
    float z      = vertex.z;
    float bh     = gl_Color.a + z;
    float atten  = zTank - bh;

    if (atten < 0.0)
        atten = z - zTank;
    if (atten < 0.0)
        atten = 0.0;

    scale = 1.0 - atten / colorFactor;

    vec3 color = gl_Color.rgb;

    if (csProc)
        // Don't fade all the way
        color *= max (0.35, scale);
    gl_FrontColor = vec4(color, tsProc ? max (0.5, scale) : 1.0);

    vertex.z    = 0.0;
    gl_Position = gl_ProjectionMatrix * vertex;
}

void BoxPyrMeshFast()
{
    vec4 vertex    = gl_ModelViewMatrix * gl_Vertex;
    const float hf = 128.0; // height factor, goes from 0.0 to 1.0 in texcoords
    float g        = 0.5 + (vertex.z - zTank) / hf;
    gl_TexCoord[0] = vec4(g, 0.0, 0.0, 1.0);
    gl_Position    = gl_ProjectionMatrix * vertex;
}

void fixedPipeline()
{
    vec4 vertex            = gl_ModelViewMatrix * gl_Vertex;
    gl_FrontColor          = gl_Color;
    gl_BackColor           = gl_Color;
    gl_FrontSecondaryColor = gl_SecondaryColor;
    gl_BackSecondaryColor  = gl_SecondaryColor;
    gl_ClipVertex          = vertex;
    gl_Position            = gl_ProjectionMatrix * vertex;
    gl_TexCoord[0]         = gl_TextureMatrix[0] * gl_MultiTexCoord0;
}

void jammedRadar()
{
    vec4 vertex    = gl_Vertex;
    vec4 texCoord  = gl_MultiTexCoord0;

    vertex.xy      = vertex.xy * radarRange;
    gl_Position    = gl_ProjectionMatrix * vertex;

    texCoord.st    = texCoord.st * noisePattern.pq + noisePattern.st;
    gl_TexCoord[0] = texCoord;
}

void viewLineWidth()
{
    vec4 vertex    = gl_Vertex;
    vertex.x      *= viewWidth;
    vertex.y      *= radarRange;
    gl_Position    = gl_ProjectionMatrix * vertex;
}

void drawTele()
{
    vec2 dims   = dimsRot.xy;
    float rot   = dimsRot.z;

    vec4 vertex = gl_Vertex;
    vertex.xy   = vertex.xy * dims;
    vec2 tmp    = vertex.xy;
    vertex.xy  *= cos(rot);
    tmp        *= sin(rot);
    vertex.x   -= tmp.y;
    vertex.y   += tmp.x;
    vertex.xy  += offset.xy;
    vertex      = gl_ModelViewMatrix * vertex;

    const float colorFactor = 40.0;

    float scale;
    float z      = teleData.x;
    float bh     = teleData.y + z;
    float atten  = zTank - bh;

    if (atten < 0.0)
        atten = z - zTank;
    if (atten < 0.0)
        atten = 0.0;

    scale = 1.0 - atten / colorFactor;

    vec3 color = vec3(1.0, 1.0, 0.25);

    // Don't fade all the way
    color *= max (0.35, scale);
    gl_FrontColor = vec4(color, max (0.5, scale));

    vertex.z    = 0.0;
    gl_Position = gl_ProjectionMatrix * vertex;
}

void drawSegmentedShot()
{
    vec2  orig = segmentedData[0].xy;
    float z    = segmentedData[0].z;
    vec2  dir  = segmentedData[1].xy;

    vec4 vertex = gl_Vertex;

    vertex.xy *= dir;
    vertex.xy += orig;

    vertex = gl_ModelViewMatrix * vertex;

    const float colorFactor = 40.0;

    float scale;
    float bh     = colorHeight.a + z;
    float atten  = zTank - bh;

    if (atten < 0.0)
        atten = z - zTank;
    if (atten < 0.0)
        atten = 0.0;

    scale = 1.0 - atten / colorFactor;

    vec3 color = colorHeight.rgb;

    // Don't fade all the way
    color *= max (0.35, scale);

    gl_FrontColor = vec4(color, 1.0);

    gl_Position = gl_ProjectionMatrix * vertex;

    float size = segmentedData[1].z;
    if (size > 0.0)
    {
        float length = segmentedData[1].w;
        if (length <= 0.0)
            size = 1.0;
    }
    gl_PointSize = size;
}

void drawSizedBullet()
{
    vec2 orig     = segmentedData[0].xy;
    vec4 vertex   = vec4(orig, 0.0, 1.0);
    vertex        = gl_ModelViewMatrix * vertex;
    gl_Position   = gl_ProjectionMatrix * vertex;

    gl_FrontColor = vec4(0.75, 0.75, 0.75, 1.0);

    float size    = segmentedData[1].z;
    gl_PointSize  = size;
}

void drawMissileBullet()
{
    vec2 orig     = segmentedData[0].xy;
    vec4 vertex   = vec4(orig, 0.0, 1.0);
    vertex        = gl_ModelViewMatrix * vertex;
    gl_Position   = gl_ProjectionMatrix * vertex;

    gl_FrontColor = vec4(1.0, 0.75, 0.75, 1.0);

    float size    = segmentedData[1].z;
    gl_PointSize  = size;
}

void drawShotNoType()
{
    vec4 vertex = gl_ModelViewMatrix * gl_Vertex;

    const float colorFactor = 40.0;

    float scale;
    float z      = vertex.z;
    float bh     = colorHeight.a + z;
    float atten  = zTank - bh;

    if (atten < 0.0)
        atten = z - zTank;
    if (atten < 0.0)
        atten = 0.0;

    scale = 1.0 - atten / colorFactor;

    vec3 color = colorHeight.rgb;

    // Don't fade all the way
    color *= max (0.35, scale);
    gl_FrontColor = vec4(color, 1.0);

    vertex.z    = 0.0;
    gl_Position = gl_ProjectionMatrix * vertex;

    gl_PointSize = 1.0;
}

void drawShot()
{
    if (shotType == segmentedType)
        drawSegmentedShot();
    else if (shotType == sizedBullet)
        drawSizedBullet();
    else if (shotType == missileBullet)
        drawMissileBullet();
    else
        drawShotNoType();
}

void drawSolidColor()
{
    vec2 dims   = dimsRot.xy;
    float rot   = dimsRot.z;

    vec4 vertex = gl_Vertex;
    vertex.xy  *= dims;

    vec2 tmp    = vertex.xy;
    vertex.xy  *= cos(rot);
    tmp        *= sin(rot);
    vertex.x   -= tmp.y;
    vertex.y   += tmp.x;

    vertex.xy  += offset.xy;

    vertex      = gl_ModelViewMatrix * vertex;
    gl_Position = gl_ProjectionMatrix * vertex;
}

void drawFlag()
{
    vec2 dims   = dimsRot.xy;

    vec4 vertex = gl_Vertex;
    vertex.xy  *= dims;

    vertex.xy  += offset.xy;

    vertex      = gl_ModelViewMatrix * vertex;

    vec3 color = colorHeight.rgb;

    if (csProc)
    {
        const float colorFactor = 40.0;

        float scale;
        float z      = offset.z;
        float bh     = colorHeight.a + z;
        float atten  = zTank - bh;

        if (atten < 0.0)
            atten = z - zTank;
        if (atten < 0.0)
            atten = 0.0;

        scale = 1.0 - atten / colorFactor;

        // Don't fade all the way
        color *= max (0.35, scale);
    }
    gl_FrontColor = vec4(color, 1.0);

    gl_Position = gl_ProjectionMatrix * vertex;
}

void dim()
{
    vec4 vertex = gl_Vertex;
    vertex.xy  *= radarRange;
    vertex      = gl_ModelViewMatrix * vertex;
    gl_Position = gl_ProjectionMatrix * vertex;
}

void main(void)
{
    if (radarKind == fastRadar)
        BoxPyrMeshFast();
    else if (radarKind == enhancedRadar)
        boxPyrMesh();
    else if (radarKind == emulFixed)
        fixedPipeline();
    else if (radarKind == jammed)
        jammedRadar();
    else if (radarKind == viewLine)
        viewLineWidth();
    else if (radarKind == flag)
        drawFlag();
    else if (radarKind == dimming)
        dim();
    else if (radarKind == tele)
        drawTele();
    else if (radarKind == shot)
        drawShot();
    else if (radarKind == solidColor)
        drawSolidColor();
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4 ft=cpp
