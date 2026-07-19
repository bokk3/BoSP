juce_add_plugin(BoDSPFilter
	COMPANY_NAME "BoDSP"
	IS_SYNTH FALSE
	NEEDS_MIDI_INPUT FALSE
	NEEDS_MIDI_OUTPUT FALSE
	IS_MIDI_EFFECT FALSE
	EDITOR_WANTS_KEYBOARD_FOCUS FALSE

	COPY_PLUGIN_AFTER_BUILD FALSE

	PLUGIN_MANUFACTURER_CODE BoDS
	PLUGIN_CODE Filt

	FORMATS VST3 Standalone

	PRODUCT_NAME "BoDSP Filter"
)

target_sources(BoDSPFilter PRIVATE
	Source/PluginProcessor.cpp
	Source/PluginProcessor.h
	Source/PluginEditor.cpp
	Source/PluginEditor.h
)

target_link_libraries(BoDSPFilter PRIVATE
	juce::juce_audio_utils
)

target_compile_definitions(BoDSPFilter PRIVATE
	JUCE_WEB_BROWSER=0
	JUCE_USE_CURL=0
	JUCE_VST3_CAN_REPLACE_VST2=0
)
