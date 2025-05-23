#pragma once

#include "Light.h"
#include <DirectXMath.h>

using namespace DirectX;

// b0
struct CB_MVP
{
    XMMATRIX model;
    XMMATRIX view;
    XMMATRIX projection;
    XMMATRIX modelInvTranspose;
};

// b1
struct CB_Lighting
{
    XMFLOAT3 cameraWorld;
    float padding; // align 16 bytes
    Light lights[MAX_LIGHTS];
};


// b2
struct CB_Material
{
    XMFLOAT3 ambient;
    float padding0;

    XMFLOAT3 diffuse;
    float padding1;

    XMFLOAT3 specular;
    float alpha;

    int useTexture;        // bool 이지만 4byte로 맞춰야함
    XMFLOAT3 padding3;
};


// b3
struct CB_Global {
    float time;
    float padding[3]; // 16바이트 정렬
};