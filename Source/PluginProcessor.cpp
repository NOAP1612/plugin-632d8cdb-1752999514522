#include "PluginProcessor.h"
#include "PluginEditor.h"

AnalogDelayAudioProcessor::AnalogDelayAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
#endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
                       ),
#endif
    parameters(*this, nullptr, juce::Identifier("AnalogDelayParams"),
    {
        std::make_unique<juce::AudioParameterChoice>("sync", "Sync to Tempo", juce::StringArray { "Free", "Sync" }, 1),
        std::make_unique<juce::AudioParameterFloat>("delayTime", "Delay Time (ms)", juce::NormalisableRange<float>(10.0f, 2000.0f, 0.1f, 0.5f), 400.0f),
        std::make_unique<juce::AudioParameterChoice>("noteDivision", "Note Division", juce::StringArray { "1/1", "1/2", "1/4", "1/8", "1/16", "1/8T", "1/16T", "1/4T" }, 2),
        std::make_unique<juce::AudioParameterFloat>("feedback", "Feedback", juce::NormalisableRange<float>(0.0f, 0.95f, 0.01f), 0.35f),
        std::make_unique<juce::AudioParameterFloat>("mix", "Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f)
    })
{
    syncParam = parameters.getRawParameterValue("sync");
    delayTimeParam = parameters.getRawParameterValue("delayTime");
    noteDivisionParam = parameters.getRawParameterValue("noteDivision");
    feedbackParam = parameters.getRawParameterValue("feedback");
    mixParam = parameters.getRawParameterValue("mix");

    parameters.addParameterListener("sync", this);
    parameters.addParameterListener("delayTime", this);
    parameters.addParameterListener("noteDivision", this);
    parameters.addParameterListener("feedback", this);
    parameters.addParameterListener("mix", this);
}

AnalogDelayAudioProcessor::~AnalogDelayAudioProcessor()
{
    parameters.removeParameterListener("sync", this);
    parameters.removeParameterListener("delayTime", this);
    parameters.removeParameterListener("noteDivision", this);
    parameters.removeParameterListener("feedback", this);
    parameters.removeParameterListener("mix", this);
}

const juce::String AnalogDelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AnalogDelayAudioProcessor::acceptsMidi() const
{
   return false;
}

bool AnalogDelayAudioProcessor::producesMidi() const
{
    return false;
}

bool AnalogDelayAudioProcessor::isMidiEffect() const
{
    return false;
}

double AnalogDelayAudioProcessor::getTailLengthSeconds() const
{
    return 2.0;
}

int AnalogDelayAudioProcessor::getNumPrograms()
{
    return 1;
}

int AnalogDelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AnalogDelayAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AnalogDelayAudioProcessor::getProgramName (int index)
{
    return {};
}

void AnalogDelayAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void AnalogDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    const int maxDelayMs = 2000;
    const int maxDelaySamples = static_cast<int>(sampleRate * maxDelayMs / 1000.0) + 1;

    for (int ch = 0; ch < 2; ++ch)
    {
        delayBuffer[ch].setSize(1, maxDelaySamples);
        delayBuffer[ch].clear();
        delayWritePosition[ch] = 0;
    }

    updateParameters();
}

void AnalogDelayAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AnalogDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void AnalogDelayAudioProcessor::updateParameters()
{
    syncEnabled = (*syncParam > 0.5f);

    if (syncEnabled)
    {
        // Host BPM
        auto playHead = getPlayHead();
        juce::AudioPlayHead::CurrentPositionInfo posInfo;
        float bpm = 120.0f;
        if (playHead != nullptr && playHead->getCurrentPosition(posInfo))
            bpm = posInfo.bpm > 0.0 ? posInfo.bpm : 120.0f;

        static const float noteMultipliers[] = {
            4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.3333f, 0.1667f, 0.25f * 0.6667f
        };
        int idx = static_cast<int>(*noteDivisionParam);
        float multiplier = noteMultipliers[juce::jlimit(0, 7, idx)];
        float quarterNoteMs = 60000.0f / bpm;
        targetDelayMs = quarterNoteMs * multiplier;
    }
    else
    {
        targetDelayMs = *delayTimeParam;
    }

    feedback = *feedbackParam;
    mix = *mixParam;
}

void AnalogDelayAudioProcessor::parameterChanged(const juce::String& paramID, float newValue)
{
    juce::ignoreUnused(paramID, newValue);
    updateParameters();
}

void AnalogDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int numChannels = juce::jmin(2, buffer.getNumChannels());
    const int numSamples = buffer.getNumSamples();

    updateParameters();

    int delaySamples = static_cast<int>(currentSampleRate * targetDelayMs / 1000.0f);
    delaySamples = juce::jlimit(1, delayBuffer[0].getNumSamples() - 1, delaySamples);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* channelData = buffer.getWritePointer(ch);
        auto* delayData = delayBuffer[ch].getWritePointer(0);
        int delayBufferSize = delayBuffer[ch].getNumSamples();
        int writePos = delayWritePosition[ch];

        for (int i = 0; i < numSamples; ++i)
        {
            int readPos = (writePos - delaySamples + delayBufferSize) % delayBufferSize;

            // Simple linear interpolation for fractional delay (not implemented here for simplicity)
            float delayedSample = delayData[readPos];

            // Analog-style: apply simple lowpass to feedback path
            float lastOut = delayedSample * 0.7f + lastDelayOutput[ch] * 0.3f;
            lastDelayOutput[ch] = lastOut;

            float in = channelData[i];
            float fb = lastOut * feedback;
            float out = in * (1.0f - mix) + lastOut * mix;

            delayData[writePos] = in + fb;

            channelData[i] = out;

            writePos = (writePos + 1) % delayBufferSize;
        }
        delayWritePosition[ch] = writePos;
    }
}

bool AnalogDelayAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AnalogDelayAudioProcessor::createEditor()
{
    return new AnalogDelayAudioProcessorEditor (*this, parameters);
}

void AnalogDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void AnalogDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AnalogDelayAudioProcessor();
}
```

```cpp