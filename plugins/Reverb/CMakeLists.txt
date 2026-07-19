juce_add_plugin(BoDSPReverb
	COMPANY_NAME "BoDSP"
	IS_SYNTH FALSE
	NEEDS_MIDI_INPUT FALSE
	NEEDS_MIDI_OUTPUT FALSE
	IS_MIDI_EFFECT FALSE
	EDITOR_WANTS_KEYBOARD_FOCUS FALSE

	COPY_PLUGIN_AFTER_BUILD FALSE

	PLUGIN_MANUFACTURER_CODE BoDS
	PLUGIN_CODE Revb

	FORMATS VST3 Standalone

	PRODUCT_NAME "BoDSP Reverb"
)

target_sources(BoDSPReverb PRIVATE
	Source/PluginProcessor.cpp
	Source/PluginProcessor.h
	Source/PluginEditor.cpp
	Source/PluginEditor.h
)

target_link_libraries(BoDSPReverb PRIVATE
	juce::juce_audio_utils
)

target_compile_definitions(BoDSPReverb PRIVATE
	JUCE_WEB_BROWSER=0
	JUCE_USE_CURL=0
	JUCE_VST3_CAN_REPLACE_VST2=0
)
