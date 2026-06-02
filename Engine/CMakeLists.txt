# Copyright 2026 Jakub Bączyk
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

add_library(ProjectDelta SHARED
   "src/delta/platform/compiler.h"
   "src/delta/core/engine.cpp"
   "include/delta/core/engine.h"
   "include/delta/core/memory.h"
   "include/delta/definitions.h"
   "src/delta/platform/os_internal.h"
   "src/delta/platform/os_win32.cpp"
   "include/delta/platform/os_types.h"
   "include/delta/platform/os.h"
   "include/delta/pch.h"
   "src/delta/pch.h"
   "src/delta/pch.cpp"
   "src/delta/core/EngineTypes.h"
   "src/delta/core/ThreadContext.h"
   "src/delta/core/ThreadContext.cpp"
   "src/delta/core/MemoryConfig.h"
   "src/delta/core/MemoryConfig.cpp"
   "src/delta/core/Worker.h"
   "src/delta/core/Worker.cpp"
 "src/delta/platform/os_internal_types.h")

target_compile_definitions(ProjectDelta
   PRIVATE DLT_EXPORT_SYMBOLS
)

target_precompile_headers(ProjectDelta
   PUBLIC "include/delta/pch.h"
   PRIVATE "src/delta/pch.h"
)

target_include_directories(ProjectDelta
   PUBLIC "include"
   PRIVATE "src"
)

if (MSVC)
   target_compile_options(ProjectDelta PRIVATE /GR-)
else()
   target_compile_options(ProjectDelta PRIVATE -fno-rtti)
endif()
