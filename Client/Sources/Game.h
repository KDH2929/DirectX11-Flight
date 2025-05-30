#pragma once

#include "NetworkManager.h"
#include "PhysicsManager.h"
#include "Renderer.h"
#include "ModelLoader.h"
#include "TextureManager.h"
#include "HeightMapTerrain.h"
#include "PlayerFlight.h"
#include "EnemyFlight.h"
#include "Bullet.h"
#include "Skybox.h"
#include "UI2D.h"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <windows.h>


class Game {
public:
    bool Init(HINSTANCE hInstance, int nCmdShow);
    int Run();          // 메시지 루프 + 렌더링 루프
    void Cleanup();

private:
    HWND hwnd;
    
    // 윈도우 관련 정보
    int windowWidth = 1280;
    int windowHeight = 720;
    int windowPosX = 100;
    int windowPosY = 100;
    LPCWSTR windowClassName = L"D3DWindowClass";
    LPCWSTR windowTitle = L"Direct3D Game Window";

    LARGE_INTEGER prevTime;     // 이전 프레임의 시간
    LARGE_INTEGER frequency;    // 성능 카운터의 주파수
    float deltaTime = 0.0f;         // 프레임 간 시간 간격
    float totalTime = 0.0f;

    NetworkManager network;
    PhysicsManager physicsManager;
    Renderer renderer;
    ModelLoader modelLoader;
    TextureManager textureManager;

    // Meshes
    std::shared_ptr<Mesh> flight1Mesh;
    std::shared_ptr<Texture> flight1Texture;
    std::shared_ptr<Flight> playerFlight1;

    // EnemyFlight
    std::vector<std::shared_ptr<EnemyFlight>> enemyFlights;

    std::vector<XMFLOAT3> enemyPositions = {
        { -60.0f, 50.0f, 100.0f },
        { -30.0f, 50.0f, 120.0f },
        {   0.0f, 50.0f, 130.0f },
        {  30.0f, 50.0f, 120.0f },
        {  60.0f, 50.0f, 100.0f },
    };

    std::shared_ptr<HeightMapTerrain> terrain;
    std::shared_ptr<Skybox> skybox;

    std::shared_ptr<Mesh> bulletMesh;
    std::shared_ptr<Texture> bulletTexture;

    std::shared_ptr<UI2D> crosshair;

    void LoadModel();
    void LoadTexture();

    bool InitWindow(HINSTANCE hInstance, int nCmdShow);
    bool InitImGui();
    void Update(float deltaTime);
    void Render();
    void HandleInput(float deltaTime);      // 서버에 키보드와 마우스 입력값 전달
    void ProcessNetwork();                   // 서버가 보낸 패킷 처리

    void SetupCollisionResponse();
};
