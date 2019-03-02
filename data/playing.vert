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

uniform bool lighting;
uniform bool lights[gl_MaxLights];
uniform bool rescaleNormal;
uniform bool normalizeNormal;
uniform bool separateColor;
uniform bool sphereMap;
uniform bool localViewer;

uniform int   model;

uniform vec4  rainLineColor[2];
uniform float rainLinealphaMod;

uniform vec4 ringParam2;
uniform vec4 ringParam;

uniform vec4  idlGlobalParam;
uniform vec4  idlLocalParam[2];

vec3 ecPosition3;
vec3 normal;
vec3 eye;

float dotC(in vec3 d1, in vec3 d2)
{
    return max(dot(d1, d2), 0.0);
}

void Light(in gl_LightProducts lightProduct,
           in gl_LightSourceParameters lightSource,
           in bool shine,
           inout vec4 ambient, inout vec4 diffuse,
           inout vec4 specular)
{
    float attenuation;    // computed attenuation factor

    // Compute vector from surface to light position
    vec3 VPpli = lightSource.position.xyz;

    if (lightSource.position.w == 0.0)
        attenuation = 1.0;
    else
    {
        VPpli -= ecPosition3;

        // Compute distance between surface and light position
        float d = length(VPpli);

        // Compute attenuation
        attenuation = 1.0 / (
                          lightSource.constantAttenuation +
                          lightSource.linearAttenuation * d +
                          lightSource.quadraticAttenuation * d * d);
    }

    ambient  += lightProduct.ambient * attenuation;

    // Normalize the vector from surface to light position
    VPpli = normalize(VPpli);

    // normal . light direction
    float nDotVP = dotC(normal, VPpli);

    if (nDotVP == 0.0)
        return;

    diffuse  += lightProduct.diffuse * nDotVP * attenuation;

    if (gl_FrontMaterial.shininess == 0.0)
        return;

    vec3 halfVector;
    // direction of maximum highlights
    if (lightSource.position.w != 0.0 || shine)
        halfVector = normalize(VPpli + eye);
    else
        halfVector = lightSource.halfVector.xyz;

    // normal . light half vector
    float nDotHV = dotC(normal, halfVector);

    // power factor
    float pf = pow(nDotHV, gl_FrontMaterial.shininess);

    specular += lightProduct.specular * pf * attenuation;
}

void setOutput(in vec4 ecPosition, in vec4 texCoord, in vec4 color)
{
    // Transform vertex to clip space
    gl_Position     = gl_ProjectionMatrix * ecPosition;

    // Transform vertex to eye coordinates
    ecPosition3 = vec3(ecPosition) / ecPosition.w;
    gl_FogFragCoord  = abs(ecPosition3.z);

    gl_ClipVertex  = ecPosition;
    gl_TexCoord[0] = texCoord;
    gl_FrontColor          = color;
    gl_FrontSecondaryColor = vec4(0.0, 0.0, 0.0, 1.0);
    gl_BackSecondaryColor  = gl_FrontSecondaryColor;
    gl_BackColor           = gl_FrontColor;

    if (!lighting)
        return;

    vec4 ambient  = vec4(0.0);
    vec4 diffuse  = vec4(0.0);
    vec4 specular = vec4(0.0);


    bool shine = any(
                     greaterThan(
                         vec3(gl_FrontMaterial.specular),
                         vec3(0.0))) && localViewer;

    vec3 u = normalize(ecPosition3);

    eye = shine ? -u : vec3(0.0, 0.0, 1.0);

    for (int i = 0; i < gl_MaxLights; i++)
        if (lights[i])
            Light(gl_FrontLightProduct[i], gl_LightSource[i], shine,
                  ambient, diffuse, specular);

    gl_FrontColor = gl_FrontLightModelProduct.sceneColor + ambient + diffuse;
    gl_FrontColor.a = gl_FrontMaterial.diffuse.a;
    if (separateColor)
        gl_FrontSecondaryColor = specular;
    else
        gl_FrontColor         += specular;
    gl_FrontSecondaryColor.a = 1.0;

    gl_BackSecondaryColor = gl_FrontSecondaryColor;
    gl_BackColor    = gl_FrontColor;
}

void fixedPipeline()
{
    // Transform vertex to eye coordinates
    vec4 ecPosition  = gl_ModelViewMatrix * gl_Vertex;

    normal = gl_NormalMatrix * gl_Normal;
    if (rescaleNormal)
        normal = gl_NormalScale * normal;
    else if (normalizeNormal)
        normal = normalize(normal);

    vec4 texCoord = gl_MultiTexCoord0;
    if (sphereMap)
    {
        vec3 ecPosition3 = vec3(ecPosition) / ecPosition.w;
        vec3 u = normalize(ecPosition3);
        vec3 r = reflect(u, normal);
        r.z   += 1.0;
        float m = 2.0 * length(r);
        vec2 st = r.xy / m + 0.5;
        texCoord = vec4(st, 0.0, 1.0);
    }
    texCoord = gl_TextureMatrix[0] * texCoord;

    setOutput(ecPosition, texCoord, gl_Color);
}

void lineRain()
{
    vec4 color;
    if (gl_Vertex.z > 0.0)
        color = rainLineColor[1];
    else
        color = rainLineColor[0];

    color.a = color.a + rainLinealphaMod;

    gl_FrontColor = color;

    gl_Position = ftransform();
}

void drawRingXY()
{
    float range         = ringParam.x;
    vec4 vertex         = gl_Vertex;
    vec4 texCoord       = gl_MultiTexCoord0;

    texCoord.t = ringParam.z;
    if (vertex.z > 0.0)
    {
        range     += ringParam.y;
        vertex.z   = ringParam2.y;
        texCoord.t = ringParam.w;
    }

    vertex.xy = range * vertex.xy;

    // Transform vertex to eye coordinates
    vec4 ecPosition  = gl_ModelViewMatrix * vertex;

    normal = gl_NormalMatrix * vec3(0.0, 0.0, 1.0);
    setOutput(ecPosition, texCoord, gl_Color);
}

void drawRingYZ()
{
    float range         = ringParam.x;
    vec4 vertex         = gl_Vertex;
    vec4 texCoord       = gl_MultiTexCoord0;

    texCoord.t = ringParam.z;
    if (vertex.x > 0.0)
    {
        range     += ringParam.y;
        vertex.x   = ringParam2.y;
        texCoord.t = ringParam.w;
    }

    vertex.yz = range * vertex.yz;

    vertex.z = max(vertex.z, -ringParam2.z);

    // Transform vertex to eye coordinates
    vec4 ecPosition  = gl_ModelViewMatrix * vertex;

    normal = gl_NormalMatrix * vec3(0.0, 0.0, 1.0);
    setOutput(ecPosition, texCoord, gl_Color);
}

void drawIDL()
{
    const vec4 innerColor = vec4(1.0, 1.0, 1.0, 0.75);
    const vec4 outerColor = vec4(1.0, 1.0, 1.0, 0.00);
    bool colorOverride    = idlGlobalParam.w > 0.0;
    vec3 origin           = idlGlobalParam.xyz;
    bool proj             = gl_Vertex.y > 0.0;
    float dist            = idlLocalParam[0].w;
    vec3 cross[2];

    cross[0] = idlLocalParam[0].xyz;
    cross[1] = idlLocalParam[1].xyz;

    vec4 vertex;
    vertex.xyz = (gl_Vertex.x == 0.0) ? cross[0] : cross[1];
    if (proj)
        vertex.xyz = mix(origin, vertex.xyz, dist);

    vertex.w      = 1.0;
    gl_Position   = gl_ModelViewProjectionMatrix * vertex;
    gl_FrontColor = colorOverride ? gl_Color : proj ? outerColor : innerColor;
}

void main(void)
{
    const int ModelFixedPipe = 0;
    const int ModelLineRain  = 1;
    const int ModelRingXY    = 2;
    const int ModelRingYZ    = 3;
    const int ModelIDL       = 4;

    if (model == ModelFixedPipe)
        fixedPipeline();
    else if (model == ModelLineRain)
        lineRain();
    else if (model == ModelRingXY)
        drawRingXY();
    else if (model == ModelRingYZ)
        drawRingYZ();
    else if (model == ModelIDL)
        drawIDL();
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4 ft=cpp
