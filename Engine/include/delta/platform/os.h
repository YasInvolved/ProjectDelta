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

#pragma once
#include <delta/platform/os_types.h>

namespace delta::platform
{
    // General
    // TODO: Change names to the adequate ones
    DLT_API const OSInfo* getOSInfo() noexcept;
    DLT_API MemoryStatus getMemoryStatus();

    // Window API
    DLT_API void Window_SetTitle(WindowHandle window, const char* newTitle);
    DLT_API void Window_Show(WindowHandle window);
    DLT_API void Window_Hide(WindowHandle window);
    DLT_API void Window_SetSize(WindowHandle window, uint32_t w, uint32_t h);
    DLT_API void Window_SetPos(WindowHandle window, uint32_t x, uint32_t y);
    DLT_API void Window_ShowCursor(WindowHandle window);
    DLT_API void Window_HideCursor(WindowHandle window);
}
