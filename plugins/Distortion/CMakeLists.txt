juce_add_plugin(BoDSPDistortion
    COMPANY_NAME "BoDSP"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    COPY_PLUGIN_AFTER_BUILD TRUE

    PLUGIN_MANUFACTURER_CODE BoDS
    PLUGIN_CODE Dist

    FORMATS VST3 Standalone

    PRODUCT_NAME "BoDSP Distortion"
)

target_sources(BoDSPDistortion PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_link_libraries(BoDSPDistortion PRIVATE
    juce::juce_audio_utils
)

target_compile_definitions(BoDSPDistortion PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)