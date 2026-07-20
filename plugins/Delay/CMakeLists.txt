juce_add_plugin(BoDSPDelay
	COMPANY_NAME "BoDSP"
	IS_SYNTH FALSE
	NEEDS_MIDI_INPUT FALSE
	NEEDS_MIDI_OUTPUT FALSE
	IS_MIDI_EFFECT FALSE
	EDITOR_WANTS_KEYBOARD_FOCUS FALSE

	COPY_PLUGIN_AFTER_BUILD FALSE

	PLUGIN_MANUFACTURER_CODE BoDS
	PLUGIN_CODE Dely

	FORMATS VST3 Standalone

	PRODUCT_NAME "BoDSP Delay"
)

target_sources(BoDSPDelay PRIVATE
	Source/PluginProcessor.cpp
	Source/PluginProcessor.h
	Source/PluginEditor.cpp
	Source/PluginEditor.h
)

target_link_libraries(BoDSPDelay PRIVATE
	juce::juce_audio_utils
)

target_compile_definitions(BoDSPDelay PRIVATE
	JUCE_WEB_BROWSER=0
	JUCE_USE_CURL=0
	JUCE_VST3_CAN_REPLACE_VST2=0
)

bodsp_setup_release_copy(BoDSPDelay "BoDSP Delay")

