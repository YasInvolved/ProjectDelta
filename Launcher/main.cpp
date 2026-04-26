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

#include <delta/core/engine.h>
#include <trigger/trigger.h>

#include <string>
#include <iostream>
#include <filesystem>
#include <cassert>

#ifdef _WIN32
    #include <Windows.h>
    using DynamicLibrary = HMODULE;

    #define LOAD_LIB(path) LoadLibraryA(path)
    #define LOAD_SYMB(name, lib) GetProcAddress(lib, name)
    #define UNLOAD_LIB(lib) FreeLibrary(lib)
#else
    #include <dlfcn.h>
    using DynamicLibrary = void*;

    #define LOAD_LIB(path) dlopen(path, RTLD_LAZY)
    #define LOAD_SYMB(name, lib) dlsym(lib, name)
    #define UNLOAD_LIB(lib) dlclose(lib)
#endif

namespace fs = std::filesystem;

using InitFunc = delta::Engine::GameInitFunc;
using UpdateFunc = delta::Engine::GameUpdateFunc;
using ShutdownFunc = delta::Engine::GameShutdownFunc;

template <typename T>
inline T loadSymbol(const char* symName, DynamicLibrary lib)
{
    return reinterpret_cast<T>(
        reinterpret_cast<void*>(LOAD_SYMB(symName, lib))
    );
}

struct Game
{
#ifdef _WIN32
    static constexpr const char MODULE_SHADOW_COPY[] = "running_game.dll";
#else
    static constexpr const char MODULE_SHADOW_COPY[] = "running_game.so";
#endif

    Game(const char* _modulePath)
        : modulePath(_modulePath),
        module(0),
        initFn(nullptr),
        updateFn(nullptr),
        shutdownFn(nullptr)
    { }

    const char* modulePath;
    DynamicLibrary module;
    InitFunc initFn;
    UpdateFunc updateFn;
    ShutdownFunc shutdownFn;

    bool load()
    {
        try
        {
            if (!fs::exists(modulePath))
            {
                std::cout << "[DeltaLauncher] Game module doesn't exist at specified path.\n";
                return false;
            }

            fs::copy(modulePath, MODULE_SHADOW_COPY, fs::copy_options::overwrite_existing);
            std::cout << "[DeltaLauncher] Game module's shadow copy successfully created.\n";
        }
        catch (const fs::filesystem_error& e)
        {
            std::cout << "[DeltaLauncher] Failed to make a shadow copy of the game module.\n";
            return false;
        }

        module = LOAD_LIB(modulePath);
        if (!module)
        {
            std::cout << "[DeltaLauncher] Failed to load library.\n";
            return false;
        }

        initFn = loadSymbol<decltype(initFn)>("Game_OnInit", module);
        updateFn = loadSymbol<decltype(updateFn)>("Game_OnUpdate", module);
        shutdownFn = loadSymbol<decltype(shutdownFn)>("Game_OnShutdown", module);

        if (!initFn || !updateFn || !shutdownFn)
        {
            std::cout << "[DeltaLauncher] Failed to load symbols.\n";
            return false;
        }

        return true;
    }

    void unload()
    {
        UNLOAD_LIB(module);
        module = 0;
        initFn = nullptr;
        updateFn = nullptr;
        shutdownFn = nullptr;
    }

    bool reload()
    {
        unload();
        return load();
    }
};

static delta::Engine::Context g_context;
static trigger::SignalSocket g_reloadSocket;

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

    const char* libPath = argv[1];
    Game game(libPath);
    if (!game.load())
    {
        std::cout << "[DeltaLauncher] Failed to load the game.\n";
        return -1;
    }

    game.initFn(&g_context);
    while (g_context.isRunning)
    {
        if (trigger::CheckForSignal(g_reloadSocket))
        {
            std::cout << "[DeltaLauncher] Reload signal received. Performing a hot-reload!\n";
            if (!game.reload())
            {
                std::cout << "[DeltaLauncher] Failed to hot-reload the game.\n";
                break;
            }
            continue;
        }

        game.updateFn(&g_context);
    }

    game.shutdownFn(&g_context);
    delta::Engine::Shutdown(g_context);

    return 0;
}
