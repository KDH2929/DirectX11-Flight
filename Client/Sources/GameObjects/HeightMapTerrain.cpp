#include "HeightMapTerrain.h"
#include "Renderer.h"
#include "DebugManager.h"

#include <fstream>
#include <DirectXTex.h>

HeightMapTerrain::HeightMapTerrain(const std::wstring& path, int width, int height, float scale, float vertexDist)
    : heightMapPath(path), mapWidth(width), mapHeight(height), heightScale(scale), vertexDistance(vertexDist)
{}

bool HeightMapTerrain::Initialize(Renderer* renderer)
{
    GameObject::Initialize(renderer);

    LoadHeightMap();
    CreateMeshFromHeightMap();

    auto device = renderer->GetDevice();

    // Vertex Buffer
    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = UINT(sizeof(MeshVertex) * vertices.size());
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vinitData = {};
    vinitData.pSysMem = vertices.data();

    device->CreateBuffer(&vbd, &vinitData, vertexBuffer.GetAddressOf());

    // Index Buffer
    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = UINT(sizeof(UINT) * indices.size());
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA iinitData = {};
    iinitData.pSysMem = indices.data();

    device->CreateBuffer(&ibd, &iinitData, indexBuffer.GetAddressOf());

    // Shader 설정
    auto shaderManager = renderer->GetShaderManager();
    shaderInfo.vertexShaderName = L"PhongVertexShader";
    shaderInfo.pixelShaderName = L"PhongPixelShader";

    vertexShader = shaderManager->GetVertexShader(shaderInfo.vertexShaderName);
    pixelShader = shaderManager->GetPixelShader(shaderInfo.pixelShaderName);
    inputLayout = shaderManager->GetInputLayout(shaderInfo.vertexShaderName);

    // Rasterizer 설정
    if (!CreateRasterizerState(device)) { 
        return false; 
    }

    // Material 기본값
    materialData.ambient = XMFLOAT3(0.2f, 0.2f, 0.2f);
    materialData.diffuse = XMFLOAT3(0.6f, 0.6f, 0.6f);
    materialData.specular = XMFLOAT3(0.1f, 0.1f, 0.1f);
    materialData.useTexture = 0;


    // 콜라이더 크기 설정
    PxVec3 halfExtents = { 30.0f, 5.0f, 30.0f };  // 초기값

    // 지형 메시의 폭과 높이를 고려하여 콜라이더 크기 조정
    float terrainWidth = static_cast<float>(mapWidth) * vertexDistance;
    float terrainHeight = static_cast<float>(mapHeight) * vertexDistance;

    halfExtents.x = terrainWidth / 2.0f;
    halfExtents.z = terrainHeight / 2.0f;

    XMFLOAT3 terrainPos = GetPosition();
    PxVec3 colliderCenter = PxVec3(
        terrainPos.x + halfExtents.x,
        terrainPos.y - halfExtents.y,
        terrainPos.z + halfExtents.z
    );

    // 콜라이더 생성
    PxRigidStatic* actor = PhysicsManager::GetInstance()->CreateStaticBox(
        colliderCenter,
        halfExtents,
        CollisionLayer::Default
    );

    BindPhysicsActor(actor);


    return true;
}

void HeightMapTerrain::Update(float deltaTime)
{
    UpdateWorldMatrix();
}

void HeightMapTerrain::Render(Renderer* renderer)
{
    DrawPhysicsColliderDebug();

    auto context = renderer->GetDeviceContext();

    // 셰이더 & 입력 레이아웃
    context->IASetInputLayout(inputLayout.Get());
    context->VSSetShader(vertexShader.Get(), nullptr, 0);
    context->PSSetShader(pixelShader.Get(), nullptr, 0);
    context->RSSetState(rasterizerState.Get());

    ID3D11DepthStencilState* prevDepthState = nullptr;
    UINT stencilRef = 0;
    context->OMGetDepthStencilState(&prevDepthState, &stencilRef);
    context->OMSetDepthStencilState(depthStencilState.Get(), 0);


    // 정점/인덱스 버퍼 설정
    UINT stride = sizeof(MeshVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // CB_MVP (b0) 구성
    CB_MVP mvpData;
    XMMATRIX view = renderer->GetCamera()->GetViewMatrix();
    XMMATRIX proj = renderer->GetCamera()->GetProjectionMatrix();

    mvpData.model = XMMatrixTranspose(worldMatrix);
    mvpData.view = XMMatrixTranspose(view);
    mvpData.projection = XMMatrixTranspose(proj);
    mvpData.modelInvTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix));

    context->UpdateSubresource(constantMVPBuffer.Get(), 0, nullptr, &mvpData, 0, 0);
    context->VSSetConstantBuffers(0, 1, constantMVPBuffer.GetAddressOf());  // b0

    // 조명 (b1)
    ID3D11Buffer* lightCB = renderer->GetLightingConstantBuffer();
    context->PSSetConstantBuffers(1, 1, &lightCB);

    // 머티리얼 (b2)
    context->UpdateSubresource(constantMaterialBuffer.Get(), 0, nullptr, &materialData, 0, 0);
    context->PSSetConstantBuffers(2, 1, constantMaterialBuffer.GetAddressOf());

    // 드로우
    context->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0);


    context->OMSetDepthStencilState(prevDepthState, stencilRef);
    if (prevDepthState) prevDepthState->Release();
}

void HeightMapTerrain::SetVertexDistance(float vertexDist)
{
    vertexDistance = vertexDist;
}

bool HeightMapTerrain::CreateRasterizerState(ID3D11Device* device)
{
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.FillMode = D3D11_FILL_SOLID;
    // rsDesc.FillMode = D3D11_FILL_WIREFRAME;
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthClipEnable = TRUE;

    return SUCCEEDED(device->CreateRasterizerState(&rsDesc, rasterizerState.GetAddressOf()));
}

void HeightMapTerrain::LoadHeightMap()
{

    ScratchImage image;
    HRESULT hr = LoadFromWICFile(heightMapPath.c_str(), WIC_FLAGS_NONE, nullptr, image);
    if (FAILED(hr))
    {
        // 로드 실패 시 그냥 리턴
        return;
    }

    const Image* img = image.GetImage(0, 0, 0);
    mapWidth = static_cast<int>(img->width);
    mapHeight = static_cast<int>(img->height);

    heightData.clear();
    heightData.reserve(mapWidth * mapHeight);

    // DirectXTex 기본으로 R8G8B8A8_UNORM으로 올라온다고 가정
    const int bytesPerPixel = 4;
    for (int y = 0; y < mapHeight; ++y)
    {
        // 한 행(row)의 메모리 시작 주소
        uint8_t* row = reinterpret_cast<uint8_t*>(img->pixels + img->rowPitch * y);

        for (int x = 0; x < mapWidth; ++x)
        {
            // x번째 픽셀의 Red 채널(또는 그냥 그레이스케일로 가정했을 때 한 채널)을 읽는다
            uint8_t gray = row[x * bytesPerPixel + 0];
            heightData.push_back(gray);
        }
    }
}

void HeightMapTerrain::CreateMeshFromHeightMap()
{
    vertices.clear();
    indices.clear();

    // vertices 생성
    for (int z = 0; z < mapHeight; ++z)
    {
        for (int x = 0; x < mapWidth; ++x)
        {
            if (!heightData.empty()) {
                // 높이 계산
                float height = heightData[z * mapWidth + x] / 255.0f * heightScale;

                float posX = (float)x * vertexDistance;
                float posZ = (float)z * vertexDistance;
                XMFLOAT3 pos = XMFLOAT3(posX, height, posZ);

                // 노멀 계산
                XMFLOAT3 normal = CalculateNormal(x, z);

                // UV 계산
                XMFLOAT2 uv = XMFLOAT2((float)x / mapWidth, (float)z / mapHeight);
                vertices.push_back({ pos, normal, uv });
            }
        }
    }

    // indices 생성
    for (int z = 0; z < mapHeight - 1; ++z)
    {
        for (int x = 0; x < mapWidth - 1; ++x)
        {
            int start = z * mapWidth + x;

            // 인덱스 범위 계산
            if (start + mapWidth + 1 < vertices.size()) {
                indices.push_back(start);
                indices.push_back(start + mapWidth);
                indices.push_back(start + 1);

                indices.push_back(start + 1);
                indices.push_back(start + mapWidth);
                indices.push_back(start + mapWidth + 1);
            }
        }
    }
}

XMFLOAT3 HeightMapTerrain::CalculateNormal(int x, int z)
{
    // 중심 높이
    float centerY = heightData[z * mapWidth + x] / 255.0f * heightScale;

    // 월드 좌표 보간을 위해 정점 간 거리 사용
    float vx = static_cast<float>(x) * vertexDistance;
    float vz = static_cast<float>(z) * vertexDistance;

    XMFLOAT3 pCenter = { vx, centerY, vz };

    // 오른쪽 (x+1, z)
    float rightY = (x < mapWidth - 1) ? heightData[z * mapWidth + (x + 1)] / 255.0f * heightScale : centerY;
    XMFLOAT3 pRight = { (x + 1) * vertexDistance, rightY, vz };

    // 위쪽 (x, z+1)
    float upY = (z < mapHeight - 1) ? heightData[(z + 1) * mapWidth + x] / 255.0f * heightScale : centerY;
    XMFLOAT3 pUp = { vx, upY, (z + 1) * vertexDistance };

    // 두 벡터 생성: 오른쪽, 위쪽
    XMVECTOR vecRight = XMVectorSubtract(XMLoadFloat3(&pRight), XMLoadFloat3(&pCenter));
    XMVECTOR vecUp = XMVectorSubtract(XMLoadFloat3(&pUp), XMLoadFloat3(&pCenter));

    // 외적 -> 노멀 벡터
    XMVECTOR normal = XMVector3Cross(vecUp, vecRight);
    normal = XMVector3Normalize(normal);

    XMFLOAT3 normalOut;
    XMStoreFloat3(&normalOut, normal);
    return normalOut;
}