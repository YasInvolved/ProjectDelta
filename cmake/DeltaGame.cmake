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

function(setup_game GAME_TARGET)
   set(GAME_BIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/bin/$<CONFIG>")

   set_target_properties(${GAME_TARGET} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${GAME_BIN_DIR}"
      LIBRARY_OUTPUT_DIRECTORY "${GAME_BIN_DIR}"
      ARCHIVE_OUTPUT_DIRECTORY "${GAME_BIN_DIR}"
      PREFIX ""
   )

   target_link_libraries(${GAME_TARGET} PRIVATE ProjectDelta)

   set(GAME_LIBRARY "$<TARGET_FILE:${GAME_TARGET}>") 
   set(GAME_ENGINE_LIBRARY "$<TARGET_FILE_DIR:${GAME_TARGET}>/$<TARGET_FILE_NAME:ProjectDelta>")
   set(GAME_LAUNCHER_EXECUTABLE "$<TARGET_FILE_DIR:${GAME_TARGET}>/$<TARGET_FILE_NAME:DeltaLauncher>")

   add_custom_command(TARGET ${GAME_TARGET} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:ProjectDelta>" ${GAME_ENGINE_LIBRARY}
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:DeltaLauncher>" ${GAME_LAUNCHER_EXECUTABLE}
      COMMENT "Copying engine and game launcher to ${GAME_TARGET}'s bin directory..."
   )

   if (MSVC)
      target_compile_definitions(${GAME_TARGET} PRIVATE DLT_GAME_EXPORT)
      target_compile_options(${GAME_TARGET} PRIVATE /Zi)

      set_target_properties(${GAME_TARGET} PROPERTIES
         VS_DEBUGGER_COMMAND "${GAME_LAUNCHER_EXECUTABLE}"
         VS_DEBUGGER_COMMAND_ARGUMENTS "${GAME_LIBRARY}"
         VS_DEBUGGER_WORKING_DIRECTORY "${GAME_BIN_DIR}"
      )
   endif()

   add_custom_target("Run${GAME_TARGET}"
      COMMAND ${CMAKE_COMMAND} -E echo "Launching game"
      COMMAND ./$<TARGET_FILE_NAME:DeltaLauncher> $<TARGET_FILE:${GAME_TARGET}>
      DEPENDS ${GAME_TARGET} ProjectDelta DeltaLauncher
      WORKING_DIRECTORY ${GAME_BIN_DIR}
   )
endfunction()
