#include "GameServer.h"

#include <iostream>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

bool GameServer::Start(int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return false;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        return false;
    }

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed\n";
        return false;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed\n";
        return false;
    }

    running = true;
    acceptThread = std::thread(&GameServer::AcceptLoop, this);
    gameLoopThread = std::thread(&GameServer::GameLoop, this);

    std::cout << "Server started on port " << port << "\n";
    return true;
}

void GameServer::Stop() {
    running = false;

    closesocket(serverSocket);
    if (acceptThread.joinable()) acceptThread.join();
    if (gameLoopThread.joinable()) gameLoopThread.join();

    WSACleanup();
    std::cout << "Server stopped\n";
}

void GameServer::AcceptLoop() {
    while (running) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            if (!running) break;
            std::cerr << "Accept failed\n";
            continue;
        }

        std::cout << "Client connected: " << clientSocket << "\n";
        std::thread(&GameServer::HandleClient, this, clientSocket).detach();
    }
}

void GameServer::HandleClient(SOCKET clientSocket) {
    clientManager.AddClient(clientSocket);  // 클라이언트 등록

    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];

    while (running) {
        int received = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (received <= 0) {
            std::cout << "Client disconnected: " << clientSocket << "\n";
            break;
        }

        buffer[received] = '\0';

        // 예: 방 번호를 1로 고정, 실제로는 메시지 파싱해야 함
        GameRoom* room = roomManager.GetRoom(1);
        if (!room) {
            room = &roomManager.CreateRoom(1);
        }

        room->AddPlayer(clientSocket);
        room->Broadcast(buffer, received, clientSocket);
    }

    roomManager.RemovePlayerFromAllRooms(clientSocket);
    clientManager.RemoveClient(clientSocket);
    closesocket(clientSocket);
}

void GameServer::GameLoop() {
    using namespace std::chrono;
    auto prev = high_resolution_clock::now();
    constexpr double tickRate = 1.0 / 60.0;  // 60 FPS 고정 틱

    while (running) {
        auto now = high_resolution_clock::now();
        duration<double> elapsed = now - prev;

        if (elapsed.count() >= tickRate) {
            prev = now;

            // 여기에 게임 처리 로직 (예: 위치 갱신, 충돌 처리 등)
            auto rooms = roomManager.GetAllRooms();
            for (auto room : rooms) {
                const auto& players = room->GetPlayers();
                // 각 플레이어의 위치 등을 갱신 또는 브로드캐스트 가능
            }
        }

        std::this_thread::sleep_for(milliseconds(1));  // 과한 CPU 점유 방지
    }
}
