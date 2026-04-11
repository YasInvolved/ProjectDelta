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

#include <iostream>

#ifdef WIN32
    #include <Windows.h>
    using DynamicLibrary = HMODULE;

    #define LOAD_LIB(path) LoadLibraryA(path)
    #define GET_FUNC(lib, name) GetProcAddress(lib, name)
    #define UNLOAD_LIB(lib) FreeLibrary(lib)
#else
    #error "Platform unsupported"
#endif

using InitFunc = delta::Engine::GameInitFunc;
using UpdateFunc = delta::Engine::GameUpdateFunc;
using ShutdownFunc = delta::Engine::GameShutdownFunc;

static delta::Engine::Context g_context;

template <typename T>
inline T loadGameFunc(DynamicLibrary gameLib, const char* const fn)
{
    return reinterpret_cast<T>(
        reinterpret_cast<void*>(GET_FUNC(gameLib, fn))
    );
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << "Game library was expected as a first argument.\n";
        return -1;
    }

    const char* gameLibName = argv[1];
    delta::Engine::Initialize(g_context);

    DynamicLibrary gameLib = LOAD_LIB(argv[1]);
    if (!gameLib)
    {
        std::cout << "Failed to load game library.\n";
        return -1;
    }

    InitFunc gameInitFn = loadGameFunc<InitFunc>(gameLib, "Game_OnInit");
    UpdateFunc gameUpdateFn = loadGameFunc<UpdateFunc>(gameLib, "Game_OnUpdate");
    ShutdownFunc gameShutdownFn = loadGameFunc<ShutdownFunc>(gameLib, "Game_OnShutdown");

    bool successfullyLoaded = gameInitFn && gameUpdateFn && gameShutdownFn;
    if (!successfullyLoaded)
    {
        std::cout << "Failed to load game functions. Please ensure that they're exposed!\n";
        return -1;
    }

    gameInitFn(&g_context);
    while (g_context.isRunning)
    {
        gameUpdateFn(&g_context);
    }

    gameShutdownFn(&g_context);
    UNLOAD_LIB(gameLib);
    delta::Engine::Shutdown(g_context);

    return 0;
}
