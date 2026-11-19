
cmake_minimum_required(VERSION 3.22)

project(plasma-wttrin VERSION 0.1.0 LANGUAGES CXX)

# --------------------------------------------------
# Compilador
# --------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Plugins dinâmicos
set(BUILD_SHARED_LIBS ON)

# Diretório explícito para bibliotecas / plugins
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# --------------------------------------------------
# KDE Extra CMake Modules
# --------------------------------------------------
find_package(ECM REQUIRED NO_MODULE)

list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)

# Diretório de saída dos plugins compartilhados
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/lib"
    CACHE PATH "Diretório de saída das bibliotecas"
    FORCE
)

# --------------------------------------------------
# Qt 6
# --------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Network
)

# --------------------------------------------------
# KDE Frameworks
# --------------------------------------------------
find_package(KF6CoreAddons REQUIRED)
find_package(KF6I18n REQUIRED)
find_package(KF6UnitConversion REQUIRED)

# --------------------------------------------------
# Plasma Weather
# --------------------------------------------------
find_package(PlasmaWeather REQUIRED)

# --------------------------------------------------
# Diretório de instalação dos plugins
# --------------------------------------------------
set(KDE_INSTALL_PLUGINDIR
    "/usr/lib/qt6/plugins"
    CACHE PATH
    "Qt plugin installation directory"
    FORCE
)

# --------------------------------------------------
# Plugin wttr.in
# --------------------------------------------------
kcoreaddons_add_plugin(plasma-wttrin
    SOURCES
        src/wttrin.cpp
        src/wttrin.h

    INSTALL_NAMESPACE
        plasma/weather_ions
)

target_link_libraries(plasma-wttrin PRIVATE
    Plasma::Weather::Ion
    Plasma::Weather::Data
    Qt6::Core
    Qt6::Network
    KF6::CoreAddons
    KF6::I18n
    KF6::UnitConversion
)
