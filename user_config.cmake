# =============================================================================
# user_config.cmake - Add your custom source files here
# =============================================================================
#
# This file is included by the generated CMakeLists.txt and allows you to
# add extra source files to the project without modifying generated files
# (which may be overwritten).
#
# To add your own sources, append them to LV_EDITOR_PROJECT_SOURCES:
#
#   list(APPEND LV_EDITOR_PROJECT_SOURCES
#       ${CMAKE_CURRENT_LIST_DIR}/src/my_widget.c
#       ${CMAKE_CURRENT_LIST_DIR}/src/my_screen.c
#   )
#
# Tip:
#   - Use ${CMAKE_CURRENT_LIST_DIR} to get paths relative to this file
#
# =============================================================================

# 新架构的最小入口。旧 UI_src 与模拟业务桩不再参与预览构建。
list(APPEND LV_EDITOR_PROJECT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../UI_next/app/ui_app.c
)

# 头文件搜索路径
# 编辑器预览构建(Emscripten)的 lvgl.h 位于
# ${LIBS_INCLUDE_DIR}/lvgl/lvgl.h, 而 UI_next 中统一写 #include "lvgl.h";
# 编辑器只把 ${LIBS_INCLUDE_DIR} 作为系统路径, 这里补一层 lvgl 子目录。
# 桌面 sim 不定义 LVED_USER_SRC_DIR, 由 FetchContent 的 lvgl target 提供头路径。
if(DEFINED LVED_USER_SRC_DIR AND DEFINED LIBS_INCLUDE_DIR
   AND EXISTS "${LIBS_INCLUDE_DIR}/lvgl/lvgl.h")
    list(APPEND LV_EDITOR_COMPONENT_INCLUDE_DIRS "${LIBS_INCLUDE_DIR}/lvgl")
endif()
