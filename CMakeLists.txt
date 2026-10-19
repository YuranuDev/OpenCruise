# =============================================================================
# Qt + aasdk 車載システム簡易プロトタイプ（x86_64 PC 向け）
#
# 目的:
#   Qt Quick（QML）+ C++ + CMake で車載向けの簡易 UI を構成する。
#   Android Auto への接続処理には aasdk リポジトリの tools/simple サンプル実装
#   （usb_init.cpp）を変更せずに再利用する。
#
# 前提:
#   - Qt 6（Core / Quick）
#   - 接続処理を使う場合は aasdk がビルド済み／インストール済みであること
#     （libusb-1.0、Boost ヘッダ、OpenSSL も必要）
#
# ビルド例:
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build -j"$(nproc)"
#   ./build/headunit
#
# aasdk を使わず UI だけをビルドする場合:
#   cmake -S . -B build -DHEADUNIT_WITH_AASDK=OFF
# =============================================================================
cmake_minimum_required(VERSION 3.16)
project(headunit_prototype VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

# --- 設定オプション ---------------------------------------------------------
option(HEADUNIT_WITH_AASDK "aasdk（tools/simple）を利用した Android Auto 接続処理を組み込む" ON)

set(HEADUNIT_AASDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/aasdk"
    CACHE PATH "aasdk リポジトリのルートディレクトリ")
set(HEADUNIT_AASDK_SIMPLE_DIR "${HEADUNIT_AASDK_DIR}/tools/simple"
    CACHE PATH "aasdk tools/simple のディレクトリ（サンプル実装を再利用する）")

# --- Qt 6 -------------------------------------------------------------------
find_package(Qt6 6.2 REQUIRED COMPONENTS Core Quick)

# --- aasdk の探索 -----------------------------------------------------------
# 本アプリが必要とするのは USB 関連ヘッダ（aasdk/USB/...）と共有ライブラリ libaasdk
# のみである。deb パッケージでもソースビルドでも使えるようヘッダとライブラリを
# 直接探索する（tools/simple の CMakeLists.txt と同じく -laasdk 相当でリンクする）。
set(HEADUNIT_AASDK_ENABLED OFF)
set(HEADUNIT_AASDK_INCLUDE_DIRS "")
set(HEADUNIT_AASDK_LIBRARIES "")

if(HEADUNIT_WITH_AASDK)
    find_path(HEADUNIT_AASDK_INCLUDE_DIR
        NAMES aasdk/USB/USBWrapper.hpp
        HINTS "${HEADUNIT_AASDK_DIR}/include")

    find_library(HEADUNIT_AASDK_LIBRARY
        NAMES aasdk
        HINTS "${HEADUNIT_AASDK_DIR}/build-release/lib"
              "${HEADUNIT_AASDK_DIR}/build-debug/lib")

    if(HEADUNIT_AASDK_INCLUDE_DIR AND HEADUNIT_AASDK_LIBRARY
       AND EXISTS "${HEADUNIT_AASDK_SIMPLE_DIR}/usb_init.cpp")
        set(HEADUNIT_AASDK_INCLUDE_DIRS "${HEADUNIT_AASDK_INCLUDE_DIR}")
        set(HEADUNIT_AASDK_LIBRARIES "${HEADUNIT_AASDK_LIBRARY}")
        set(HEADUNIT_AASDK_ENABLED ON)
        message(STATUS "aasdk を検出しました: ${HEADUNIT_AASDK_LIBRARY}")
        message(STATUS "aasdk ヘッダ: ${HEADUNIT_AASDK_INCLUDE_DIR}")
        message(STATUS "利用するサンプル実装: ${HEADUNIT_AASDK_SIMPLE_DIR}/usb_init.cpp")
    else()
        message(WARNING
            "aasdk を検出できませんでした。Qt UI のみをビルドします。\n"
            "  ヘッダ        : '${HEADUNIT_AASDK_INCLUDE_DIR}'\n"
            "  ライブラリ    : '${HEADUNIT_AASDK_LIBRARY}'\n"
            "  サンプル実装  : '${HEADUNIT_AASDK_SIMPLE_DIR}/usb_init.cpp'\n"
            "  aasdk の場所は -DHEADUNIT_AASDK_DIR=<path> で指定できます。")
    endif()
endif()

# --- 実行ファイル -----------------------------------------------------------
add_executable(headunit
    src/main.cpp
    src/AndroidAutoController.h
    src/AndroidAutoController.cpp
    resources.qrc
)

target_include_directories(headunit PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(headunit PRIVATE Qt6::Core Qt6::Quick)

if(HEADUNIT_AASDK_ENABLED)
    # tools/simple のサンプル実装が依存する libusb-1.0 と Boost ヘッダを要求する。
    # libusb-1.0 の Find モジュールは aasdk リポジトリ同梱のものを再利用する。
    list(APPEND CMAKE_MODULE_PATH "${HEADUNIT_AASDK_DIR}/cmake_modules")
    find_package(libusb-1.0 REQUIRED)
    find_package(Boost REQUIRED)

    # サンプル実装（usb_init.cpp）を変更せずそのままコンパイル対象へ加える。
    target_compile_definitions(headunit PRIVATE HEADUNIT_HAS_AASDK=1)
    target_include_directories(headunit PRIVATE
        "${HEADUNIT_AASDK_SIMPLE_DIR}"
        ${HEADUNIT_AASDK_INCLUDE_DIRS}
        ${LIBUSB_1_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS})
    target_sources(headunit PRIVATE "${HEADUNIT_AASDK_SIMPLE_DIR}/usb_init.cpp")
    target_link_libraries(headunit PRIVATE
        ${HEADUNIT_AASDK_LIBRARIES}
        ${LIBUSB_1_LIBRARIES})
else()
    message(STATUS "Android Auto 接続処理なし（UI のみ）でビルドします。")
endif()
