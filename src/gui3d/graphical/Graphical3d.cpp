#include "Graphical3d.hpp"
#include "config.hpp"
#include "world.hpp"
#include <raylib.h>

Graphical3D::Graphical3D(Network::NetworkClient &networkClient) : m_networkClient(networkClient)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT, Config::WINDOW_TITLE);
    SetTargetFPS(Config::TARGET_FPS);
    InitAudioDevice();

    m_font = LoadFontEx(Config::FONT_PATH, 32, nullptr, 0);
    if (m_font.texture.id != 0) {
        SetTextureFilter(m_font.texture, TEXTURE_FILTER_BILINEAR);
        m_fontLoaded = true;
    }

    m_tileModel = LoadModel("assets/map/block-grass-large.obj");
    m_tileModelLoaded = (m_tileModel.meshCount > 0);

    m_playerModel = LoadModel("assets/character/animal-koala.obj");
    m_playerModelLoaded = (m_playerModel.meshCount > 0);

    m_music = LoadMusicStream("assets/musique zappy.ogg");
    if (m_music.stream.buffer != nullptr) {
        m_musicLoaded = true;
        PlayMusicStream(m_music);
    }

    initCamera();
    pushLog("CLIENT STARTED");
}

Graphical3D::~Graphical3D()
{
    if (m_tileModelLoaded)
        UnloadModel(m_tileModel);
    if (m_playerModelLoaded)
        UnloadModel(m_playerModel);
    if (m_fontLoaded)
        UnloadFont(m_font);
    if (m_musicLoaded) {
        StopMusicStream(m_music);
        UnloadMusicStream(m_music);
    }
    CloseAudioDevice();
    CloseWindow();
}

void Graphical3D::run()
{
    while (m_running && !WindowShouldClose()) {
        handleInput();
        update(GetFrameTime());
        render();
    }
}