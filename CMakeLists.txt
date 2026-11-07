cmake_minimum_required(VERSION 3.22)
project(JohnMayerGoat VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(JohnMayerGoat
    PRODUCT_NAME "JohnMayerGoat"
    COMPANY_NAME "CS IA"
    BUNDLE_ID "com.csia.johnmayergoat"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "JohnMayerGoat needs audio input to process your guitar through the interface."
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE
)

juce_generate_juce_header(JohnMayerGoat)

target_sources(JohnMayerGoat PRIVATE
    src/Main.cpp
    src/MainComponent.cpp
    src/dsp/Effect.cpp
    src/dsp/EffectChain.cpp
    src/dsp/AudioEngine.cpp
    src/dsp/DigitalPreamp.cpp
    src/dsp/amps/ToneStack.cpp
    src/dsp/amps/PowerAmp.cpp
    src/dsp/amps/DeluxeReverbModel.cpp
    src/dsp/amps/SteelStringModel.cpp
    src/dsp/TubeScreamer.cpp
    src/dsp/Bluesbreaker.cpp
    src/dsp/RatDistortion.cpp
    src/dsp/Shredmaster.cpp
    src/dsp/Chorus.cpp
    src/dsp/TapeDelay.cpp
    src/dsp/SlapbackDelay.cpp
    src/dsp/CabinetSim.cpp
    src/dsp/GoldenReverb.cpp
    src/gui/KnobLookAndFeel.cpp
    src/gui/PedalComponent.cpp
    src/gui/PedalboardComponent.cpp
    src/gui/AmpPageComponent.cpp
    src/gui/IOPageComponent.cpp
    src/presets/PresetManager.cpp
)

target_include_directories(JohnMayerGoat PRIVATE src)

target_compile_definitions(JohnMayerGoat PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:JohnMayerGoat,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:JohnMayerGoat,JUCE_VERSION>"
)

target_link_libraries(JohnMayerGoat PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
        juce::juce_recommended_config_flags
)
