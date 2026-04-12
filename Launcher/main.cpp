/*
 * Copyright 2026 Jakub Bączyk
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <delta/engine.h>
#include <trigger/trigger.h>

#include <string>
#include <iostream>
#include <filesystem>
#include <cassert>

#ifdef _WIN32
    #include <Windows.h>
    using DynamicLibrary = HMODULE;

    #define LOAD_LIB(path) LoadLibraryA(path)
    #define GET_FUNC(lib, name) GetProcAddress(lib, name)
    #define UNLOAD_LIB(lib) FreeLibrary(lib)
#else
    #error "Platform unsupported"
#endif

namespace fs = std::filesystem;

using InitFunc = delta::Engine::GameInitFunc;
using UpdateFunc = delta::Engine::GameUpdateFunc;
using ShutdownFunc = delta::Engine::GameShutdownFunc;

struct Game
{
    static constexpr const char* INVALID_PATH = reinterpret_cast<const char*>(~(0ull));
    static constexpr DynamicLibrary INVALID_LIB_HANDLE = reinterpret_cast<DynamicLibrary>(~(0ull));

    Game() : libPath(INVALID_PATH), lib(INVALID_LIB_HANDLE), initFn(nullptr), updateFn(nullptr), shutdownFn(nullptr)
    { }

    Game(const char* _libPath) : libPath(_libPath), lib(INVALID_LIB_HANDLE), initFn(nullptr), updateFn(nullptr), shutdownFn(nullptr)
    { }

    const char* libPath;
    DynamicLibrary lib;
    InitFunc initFn;
    UpdateFunc updateFn;
    ShutdownFunc shutdownFn;
};

static delta::Engine::Context g_context;
static trigger::SignalSocket g_reloadSocket;

template <typename T>
inline T loadGameFunc(DynamicLibrary gameLib, const char* const fn)
{
    return reinterpret_cast<T>(
        reinterpret_cast<void*>(GET_FUNC(gameLib, fn))
    );
}

static bool loadGame(Game& game)
{
#ifdef _WIN32
    constexpr const char shadowLib[] = "game_running.dll";
#else
    constexpr const char shadowLib[] = "game_running.so";
#endif

    assert(game.libPath != Game::INVALID_PATH);

    try
    {
        if (!fs::exists(game.libPath))
        {
            std::cout << "[DeltaLauncher] Game library doesn't exist!\n";
            return false;
        }

        fs::copy(game.libPath, shadowLib, fs::copy_options::overwrite_existing);
        std::cout << "[DeltaLauncher] Game's library shadow copy created successfully.\n";
    }
    catch (const fs::filesystem_error& e)
    {
        std::cout << "[DeltaLauncher] An error occured during Game's library shadow copy creation!\n";
        return false;
    }

    game.lib = LOAD_LIB(shadowLib);
    if (!game.lib || game.lib == Game::INVALID_LIB_HANDLE)
        return false;

    game.initFn = loadGameFunc<decltype(game.initFn)>(game.lib, "Game_OnInit");
    game.updateFn = loadGameFunc<decltype(game.updateFn)>(game.lib, "Game_OnUpdate");
    game.shutdownFn = loadGameFunc<decltype(game.shutdownFn)>(game.lib, "Game_OnShutdown");

    if (!game.initFn || !game.updateFn || !game.shutdownFn)
        return false;

    return true;
}

static void reloadGame(Game& game)
{
    game.shutdownFn(&g_context);
    UNLOAD_LIB(game.lib);

    if (!loadGame(game))
    {
        std::cout << "[DeltaLauncher] Failed to reload the game.\n";
        game.lib = Game::INVALID_LIB_HANDLE;
        game.shutdownFn = nullptr;
        game.updateFn = nullptr;
        game.initFn = nullptr;
        return;
    }

    game.initFn(&g_context);
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << "[DeltaLauncher] Game library was expected as a first argument.\n";
        return -1;
    }

    g_reloadSocket = trigger::StartServer(6767);
    if (g_reloadSocket == trigger::INVALID_SIGNAL_SOCKET)
    {
        std::cout << "[DeltaLauncher] Failed to open a socket for listening!\n";
        return -1;
    }

    
    delta::Engine::Initialize(g_context);

    Game game(argv[1]);
    if (!loadGame(game))
    {
        std::cout << "[DeltaLauncher] Failed to load the game library.\n";
        return -1;
    }

    game.initFn(&g_context);
    while (g_context.isRunning)
    {
        if (trigger::CheckForSignal(g_reloadSocket))
        {
            std::cout << "[DeltaLauncher] Reload signal received. Performing a hot-reload!\n";
            reloadGame(game);
            continue;
        }

        game.updateFn(&g_context);
    }

    if (game.lib != Game::INVALID_LIB_HANDLE)
    {
        game.shutdownFn(&g_context);
        UNLOAD_LIB(game.lib);
    }

    delta::Engine::Shutdown(g_context);

    return 0;
}
