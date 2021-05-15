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
uniform bool gourad;
uniform bool separateColor;
uniform bool localViewer;
uniform int  fogMode;
uniform bool fogging;
uniform sampler2D ourTexture;
uniform bool      texturing;
uniform bool      replaceTexture;

varying vec3 ecPosition3;
varying vec3 normal;
varying vec3 eye;

float dotC(in vec3 d1, in vec3 d2)
{
    return max(dot(d1, d2), 0.0);
}

void LightD(in gl_LightProducts lightProduct,
            in gl_LightSourceParameters lightSource,
            inout vec4 ambient, inout vec4 diffuse,
            inout vec4 specular)
{
    // Compute vector from surface to light position
    vec3 VPpli = lightSource.position.xyz;

    ambient += lightProduct.ambient;

    // Normalize the vector from surface to light position
    VPpli = normalize(VPpli);

    // normal . light direction
    float nDotVP = dotC(normal, VPpli);

    diffuse += lightProduct.diffuse * nDotVP;

    // direction of maximum highlights
    vec3 halfVector = localViewer ? normalize(VPpli + eye) : lightSource.halfVector.xyz;

    // normal . light half vector
    float nDotHV = dotC(normal, halfVector);

    // power factor
    float pf = pow(nDotHV, max(gl_FrontMaterial.shininess, 0.001));

    specular += nDotVP == 0.0 ? vec4(0.0) : lightProduct.specular * pf;
}

void LightP(in gl_LightProducts lightProduct,
            in gl_LightSourceParameters lightSource,
            inout vec4 ambient, inout vec4 diffuse,
            inout vec4 specular)
{
    float attenuation;    // computed attenuation factor

    // Compute vector from surface to light position
    vec3 VPpli = lightSource.position.xyz;

    VPpli -= ecPosition3;

    // Compute distance between surface and light position
    float d = length(VPpli);

    // Compute attenuation
    attenuation = 1.0 / (
                      lightSource.constantAttenuation +
                      (lightSource.linearAttenuation +
                       lightSource.quadraticAttenuation * d) * d);

    ambient  += lightProduct.ambient * attenuation;

    // Normalize the vector from surface to light position
    VPpli = VPpli / d;

    // normal . light direction
    float nDotVP = dotC(normal, VPpli);

    diffuse += lightProduct.diffuse * nDotVP * attenuation;

    // direction of maximum highlights
    vec3 halfVector = normalize(VPpli + eye);

    // normal . light half vector
    float nDotHV = dotC(normal, halfVector);

    // power factor
    float pf = pow(nDotHV, max(gl_FrontMaterial.shininess, 0.001));

    specular += nDotVP == 0.0 ? vec4(0.0) : lightProduct.specular * pf * attenuation;
}

void main(void)
{
    const int FogExp    = 0;
    const int FogExp2   = 1;
    const int FogLinear = 2;

    vec4 color;
    vec3 secondaryColor = vec3(0.0);

    if (gourad || !lighting)
    {
        color          = gl_Color;
        secondaryColor = gl_SecondaryColor.rgb;
    }
    else
    {
        vec4 ambient  = vec4(0.0);
        vec4 diffuse  = vec4(0.0);
        vec4 specular = vec4(0.0);

        if (lights[0])
            LightD(gl_FrontLightProduct[0], gl_LightSource[0], ambient, diffuse, specular);
        for (int i = 1; i < gl_MaxLights; i++)
            if (lights[i])
                LightP(gl_FrontLightProduct[i], gl_LightSource[i], ambient, diffuse, specular);

        color = gl_FrontLightModelProduct.sceneColor + ambient + diffuse;
        color.a = gl_FrontMaterial.diffuse.a;
        if (separateColor)
            secondaryColor = specular.rgb;
        else
            color         += specular;
        color = clamp(color, 0.0, 1.0);
    }

    if (texturing)
    {
        vec4 texColor = texture2D(ourTexture, gl_TexCoord[0].st);
        if (replaceTexture)
            color  = texColor;
        else
            color *= texColor;
    }
    if (lighting)
        color.rgb = color.rgb + secondaryColor;
    color = clamp(color, 0.0, 1.0);

    if (fogging)
    {
        float fogFactor = 1.0;
        if (fogMode == FogExp)
            fogFactor = exp(-gl_Fog.density * gl_FogFragCoord);
        else if (fogMode == FogExp2)
            fogFactor = exp(-pow((gl_Fog.density * gl_FogFragCoord), 2.0));
        else if (fogMode == FogLinear)
            fogFactor = (gl_Fog.end - gl_FogFragCoord) * gl_Fog.scale;
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        color = mix(gl_Fog.color, color, fogFactor);
    }
    gl_FragColor = color;
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4 ft=cpp
