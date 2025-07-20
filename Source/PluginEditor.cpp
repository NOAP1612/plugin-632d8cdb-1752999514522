#include "PluginProcessor.h"
#include "PluginEditor.h"

AnalogDelayAudioProcessorEditor::AnalogDelayAudioProcessorEditor (AnalogDelayAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p), audioProcessor (p), valueTreeState(vts)
{
    setSize (420, 250);

    // Sync Toggle
    syncLabel.setText("Sync", juce::dontSendNotification);
    syncLabel.attachToComponent(&syncToggle, false);
    addAndMakeVisible(syncLabel);

    syncToggle.addItem("Free", 1);
    syncToggle.addItem("Sync", 2);
    syncToggle.setJustificationType(juce::Justification::centred);
    syncToggleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        valueTreeState, "sync", syncToggle);
    addAndMakeVisible(syncToggle);

    // Delay Time Slider
    delayTimeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    delayTimeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    delayTimeSlider.setRange(10.0, 2000.0, 0.1);
    delayTimeSlider.setSkewFactorFromMidPoint(250.0);
    delayTimeLabel.setText("Delay (ms)", juce::dontSendNotification);
    delayTimeLabel.attachToComponent(&delayTimeSlider, false);
    delayTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        valueTreeState, "delayTime", delayTimeSlider);
    addAndMakeVisible(delayTimeSlider);
    addAndMakeVisible(delayTimeLabel);

    // Note Division ComboBox
    noteDivisionLabel.setText("Note", juce::dontSendNotification);
    noteDivisionLabel.attachToComponent(&noteDivisionBox, false);
    noteDivisionBox.addItem("1/1", 1);
    noteDivisionBox.addItem("1/2", 2);
    noteDivisionBox.addItem("1/4", 3);
    noteDivisionBox.addItem("1/8", 4);
    noteDivisionBox.addItem("1/16", 5);
    noteDivisionBox.addItem("1/8T", 6);
    noteDivisionBox.addItem("1/16T", 7);
    noteDivisionBox.addItem("1/4T", 8);
    noteDivisionBox.setJustificationType(juce::Justification::centred);
    noteDivisionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        valueTreeState, "noteDivision", noteDivisionBox);
    addAndMakeVisible(noteDivisionBox);
    addAndMakeVisible(noteDivisionLabel);

    // Feedback Slider
    feedbackSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    feedbackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    feedbackSlider.setRange(0.0, 0.95, 0.01);
    feedbackLabel.setText("Feedback", juce::dontSendNotification);
    feedbackLabel.attachToComponent(&feedbackSlider, false);
    feedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        valueTreeState, "feedback", feedbackSlider);
    addAndMakeVisible(feedbackSlider);
    addAndMakeVisible(feedbackLabel);

    // Mix Slider
    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    mixSlider.setRange(0.0, 1.0, 0.01);
    mixLabel.setText("Mix", juce::dontSendNotification);
    mixLabel.attachToComponent(&mixSlider, false);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        valueTreeState, "mix", mixSlider);
    addAndMakeVisible(mixSlider);
    addAndMakeVisible(mixLabel);

    // Show/hide controls depending on sync
    updateSyncState();
    syncToggle.onChange = [this] { updateSyncState(); };
}

AnalogDelayAudioProcessorEditor::~AnalogDelayAudioProcessorEditor()
{
}

void AnalogDelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    juce::Colour accent = juce::Colour(0xffe6b800);
    g.setColour(accent);
    g.setFont(28.0f);
    g.drawFittedText("Analog Delay", getLocalBounds().removeFromTop(40), juce::Justification::centred, 1);

    g.setColour(juce::Colours::grey);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(8.0f), 10.0f, 2.0f);
}

void AnalogDelayAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    auto top = area.removeFromTop(48);

    syncToggle.setBounds(top.removeFromLeft(100).reduced(0, 10));
    noteDivisionBox.setBounds(top.removeFromLeft(100).reduced(0, 10));
    delayTimeSlider.setBounds(top.removeFromLeft(120).reduced(0, 10));

    auto knobArea = area.removeFromTop(160);
    auto knobW = 100;
    feedbackSlider.setBounds(knobArea.removeFromLeft(knobW).withTrimmedTop(10));
    mixSlider.setBounds(knobArea.removeFromLeft(knobW).withTrimmedTop(10));
}

void AnalogDelayAudioProcessorEditor::updateSyncState()
{
    bool sync = syncToggle.getSelectedId() == 2;
    noteDivisionBox.setVisible(sync);
    delayTimeSlider.setVisible(!sync);
}
```

```cmake
#