cmake_minimum_required(VERSION 3.22)

project(WilboRig VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# JUCE: usa uma pasta local se voce passar -DWILBO_JUCE_DIR=caminho,
# senao baixa automaticamente do GitHub (precisa de internet na 1a vez).
# ---------------------------------------------------------------------------
set(WILBO_JUCE_DIR "" CACHE PATH "Pasta local do JUCE (opcional)")

if(WILBO_JUCE_DIR)
    add_subdirectory(${WILBO_JUCE_DIR} juce EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------------------
# O app: gera o programa Standalone (.exe) e tambem um plugin VST3
# ---------------------------------------------------------------------------
juce_add_plugin(WilboRig
    COMPANY_NAME "Wilbo Studio"
    PRODUCT_NAME "Wilbo Rig"
    PLUGIN_MANUFACTURER_CODE Wilb
    PLUGIN_CODE WRig
    FORMATS Standalone VST3
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

set(WILBO_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Parameters.cpp
    Source/DSP/Tuner.cpp
    Source/DSP/Wah.cpp
    Source/DSP/Drive.cpp
    Source/DSP/Amp.cpp
    Source/DSP/Cabinet.cpp
    Source/DSP/Delay.cpp
    Source/UI/LookAndFeel.cpp
    Source/UI/PedalComponent.cpp
    Source/UI/TunerDisplay.cpp)

target_sources(WilboRig PRIVATE ${WILBO_SOURCES})
target_include_directories(WilboRig PRIVATE Source)

target_compile_definitions(WilboRig PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(WilboRig
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Testes automaticos (opcional): cmake -DWILBO_BUILD_TESTS=ON
# ---------------------------------------------------------------------------
option(WILBO_BUILD_TESTS "Compilar testes offline" OFF)

if(WILBO_BUILD_TESTS)
    juce_add_console_app(WilboTests PRODUCT_NAME "WilboTests")
    target_sources(WilboTests PRIVATE Tests/TestMain.cpp ${WILBO_SOURCES})
    target_include_directories(WilboTests PRIVATE Source)
    target_compile_definitions(WilboTests PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(WilboTests PRIVATE
        juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags)
endif()
