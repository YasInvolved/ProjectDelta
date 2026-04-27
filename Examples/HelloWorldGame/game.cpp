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
#include <delta/platform/os.h>

#include <iostream>

#define STDOUT_BOOL_FORMAT(x) (x ? "true" : "false")

#ifdef WIN32
    #ifdef DLT_GAME_EXPORT
        #define GAME_API __declspec(dllexport)
    #else
        #define GAME_API __declspec(dllimport)
    #endif
#else
    #define GAME_API
#endif

using OSInfo = delta::platform::OSInfo;

extern "C"
{
    void GAME_API Game_OnInit(delta::Engine::Context* context)
    {
        const OSInfo* info = delta::platform::getOSInfo();

        std::cout << "Initializing game\n";
        std::cout << "System Info:\n";
        std::cout <<
            "\tOS Page Size: " << info->osPageSize << " bytes\n" <<
            "\tCPU Architecture: " << info->cpuArchitecture << "\n" <<
            "\tCPU Manufacturer: " << info->cpuManufacturerId << "\n" <<
            "\tCPU Model: " << info->cpuBrandString << "\n" <<
            "\tAVX2 Available: " << STDOUT_BOOL_FORMAT(info->cpuHasAVX2) << "\n" <<
            "\tAVX512 Foundation Available: " << STDOUT_BOOL_FORMAT(info->cpuHasAVX512f) << "\n" <<
            "\tAVX512 Conflict Detection Available: " << STDOUT_BOOL_FORMAT(info->cpuHasAVX512cd) << "\n" <<
            "\tAVX512 Exponential and Reciprocal Available: " << STDOUT_BOOL_FORMAT(info->cpuHasAVX512er) << "\n" <<
            "\tAVX512 Prefetch Available: " << STDOUT_BOOL_FORMAT(info->cpuHasAVX512pf) << "\n";
    }

    void GAME_API Game_OnUpdate(delta::Engine::Context* context)
    {
        char input;
        std::cout << "\"1\" to continue and \"2\" to exit: ";
        std::cin >> input;

        switch (input)
        {
        case '1':
            break;
        case '2':
            context->isRunning = false;
            break;
        }
    }

    void GAME_API Game_OnShutdown(delta::Engine::Context* context)
    {
        std::cout << "Shutdown\n";
    }
}
