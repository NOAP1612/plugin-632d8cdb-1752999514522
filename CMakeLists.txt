CMakeLists.txt

cmake_minimum_required(VERSION 3.15)

project(AnalogDelay VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(JUCE)

juce_add_plugin(AnalogDelay
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Juce
    PLUGIN_CODE AnDe
    FORMATS VST3 AU
    PRODUCT_NAME "Analog Delay"
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(AnalogDelay
    PRIVATE
        PluginProcessor.cpp
        PluginEditor.cpp
        PluginProcessor.h
        PluginEditor.h
)

target_link_libraries(AnalogDelay
    PRIVATE
        juce::juce_audio_utils
)
```