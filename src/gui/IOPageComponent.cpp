#include "IOPageComponent.h"
#include "GraphicUtils.h"
#include "HardwareTheme.h"

namespace
{
void styleLabel(juce::Label& label, float size = 13.0f)
{
    label.setColour(juce::Label::textColourId, juce::Colour(0xfff3e6c4));
    label.setJustificationType(juce::Justification::centred);
    label.setFont(gfx::fontCondensed(size));
    label.setInterceptsMouseClicks(false, false);
}
}

IOPageComponent::IOPageComponent(AudioEngine& engineToUse, juce::AudioDeviceManager& devicesToUse)
    : engine(engineToUse), devices(devicesToUse)
{
    auto setupKnob = [this] (juce::Slider& s, juce::Label& label, auto setter, float initial)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setRange(0.0, 1.0, 0.001);
        s.setValue(initial, juce::dontSendNotification);
        s.setLookAndFeel(&knobsLaf);
        s.setRotaryParameters(juce::degreesToRadians(225.0f), juce::degreesToRadians(495.0f), true);
        s.setMouseDragSensitivity(180);
        s.onValueChange = [&s, setter] { setter((float) s.getValue()); };
        styleLabel(label, 12.0f);
        addAndMakeVisible(s);
        addAndMakeVisible(label);
    };

    auto setupReadout = [] (juce::Label& readout)
    {
        readout.setJustificationType(juce::Justification::centred);
        readout.setColour(juce::Label::textColourId, juce::Colour(0xfff3e6c4));
        readout.setColour(juce::Label::backgroundColourId, juce::Colour(0xcc101114));
        readout.setFont(gfx::fontCondensed(12.0f));
        readout.setInterceptsMouseClicks(false, false);
        readout.setVisible(false);
    };
    setupReadout(inputValue);
    setupReadout(outputValue);
    addAndMakeVisible(inputValue);
    addAndMakeVisible(outputValue);

    setupKnob(inputGain, inputLabel, [this] (float v)
    {
        engine.setInputGain(v);
        inputValue.setText(juce::String(juce::roundToInt(v * 100.0f)), juce::dontSendNotification);
        inputValue.setVisible(true);
        inputValue.toFront(false);
    }, engine.getInputGain());
    inputGain.onDragEnd = [this] { inputValue.setVisible(false); };

    setupKnob(outputGain, outputLabel, [this] (float v)
    {
        engine.setOutputGain(v);
        outputValue.setText(juce::String(juce::roundToInt(v * 100.0f)), juce::dontSendNotification);
        outputValue.setVisible(true);
        outputValue.toFront(false);
    }, engine.getOutputGain());
    outputGain.onDragEnd = [this] { outputValue.setVisible(false); };

    audioButton.setLookAndFeel(&knobsLaf);
    KnobLookAndFeel::applyChrome(audioButton, "teal");
    audioButton.onClick = [this] { openAudioSettings(devices); };
    addAndMakeVisible(audioButton);
}

IOPageComponent::~IOPageComponent()
{
    stopTimer();
    inputGain.setLookAndFeel(nullptr);
    outputGain.setLookAndFeel(nullptr);
    audioButton.setLookAndFeel(nullptr);
}

void IOPageComponent::openAudioSettings(juce::AudioDeviceManager& devices)
{
    juce::DialogWindow::LaunchOptions options;
    auto* selector = new juce::AudioDeviceSelectorComponent(devices, 1, 2, 1, 2, false, false, true, false);
    selector->setSize(500, 420);
    options.content.setOwned(selector);
    options.dialogTitle = "Audio Settings";
    options.dialogBackgroundColour = juce::Colour(0xff1a1a1a);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;
    options.launchAsync();
}

void IOPageComponent::sync()
{
    inputGain.setValue(engine.getInputGain(), juce::dontSendNotification);
    outputGain.setValue(engine.getOutputGain(), juce::dontSendNotification);
    repaint();
}

void IOPageComponent::visibilityChanged()
{
    if (isShowing())
        startTimerHz(24);
    else
        stopTimer();
}

void IOPageComponent::timerCallback()
{
    inMeter = 0.85f * inMeter + 0.15f * engine.getInputPeak();
    outMeter = 0.85f * outMeter + 0.15f * engine.getOutputPeak();
    repaint(inMeterBounds.expanded(4).getUnion(outMeterBounds.expanded(4)));
}

void IOPageComponent::paint(juce::Graphics& g)
{
    theme::studioFloor(g, getLocalBounds().toFloat());

    auto area = getLocalBounds().reduced(36, 28);
    auto title = area.removeFromTop(36);
    g.setColour(juce::Colour(0xfff3e6c4));
    g.setFont(gfx::fontSerifItalic(22.0f));
    g.drawText("Input & Output", title, juce::Justification::centredLeft);

    auto modules = area.removeFromTop(juce::jmax(280, area.getHeight() - 80)).reduced(0, 8);
    auto inPanel = modules.removeFromLeft(modules.getWidth() / 2).reduced(10).toFloat();
    auto outPanel = modules.reduced(10).toFloat();

    auto drawPanel = [&g] (juce::Rectangle<float> r, juce::String name)
    {
        gfx::vinyl(g, r, juce::Colour(0xff1a1c20), 12.0f);
        gfx::brushed(g, r, juce::Colour(0xff2a2c30), true, 9);
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawRoundedRectangle(r, 12.0f, 1.2f);
        g.setColour(juce::Colour(0xfff3e6c4).withAlpha(0.8f));
        g.setFont(gfx::fontCondensed(14.0f));
        g.drawText(name, r.removeFromTop(36.0f), juce::Justification::centred);
    };
    drawPanel(inPanel, "INPUT");
    drawPanel(outPanel, "OUTPUT");

    auto drawMeter = [&g] (juce::Rectangle<int> r, float level)
    {
        g.setColour(juce::Colour(0xff101114));
        g.fillRoundedRectangle(r.toFloat(), 4.0f);
        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.drawRoundedRectangle(r.toFloat(), 4.0f, 1.0f);

        const float clamped = juce::jlimit(0.0f, 1.0f, level);
        auto fill = r.removeFromBottom(juce::roundToInt((float) r.getHeight() * clamped));
        g.setColour(clamped > 0.9f ? juce::Colours::red : juce::Colour(0xff7CFF6B));
        g.fillRoundedRectangle(fill.toFloat().reduced(2.0f, 1.0f), 3.0f);
    };
    drawMeter(inMeterBounds, inMeter);
    drawMeter(outMeterBounds, outMeter);
}

void IOPageComponent::resized()
{
    auto area = getLocalBounds().reduced(36, 28);
    area.removeFromTop(36);
    auto modules = area.removeFromTop(juce::jmax(280, area.getHeight() - 80)).reduced(0, 8);

    auto layoutModule = [this] (juce::Rectangle<int> module, juce::Slider& knob, juce::Label& label,
                                juce::Rectangle<int>& meterBounds)
    {
        module = module.reduced(18, 44);
        auto meter = module.removeFromRight(36).reduced(8, 12);
        meterBounds = meter;
        knob.setBounds(module.removeFromTop(module.getHeight() - 28).reduced(24, 8));
        label.setBounds(module);
    };

    layoutModule(modules.removeFromLeft(modules.getWidth() / 2).reduced(10), inputGain, inputLabel, inMeterBounds);
    layoutModule(modules.reduced(10), outputGain, outputLabel, outMeterBounds);
    inputValue.setBounds(inputGain.getBounds().withSizeKeepingCentre(44, 18));
    outputValue.setBounds(outputGain.getBounds().withSizeKeepingCentre(44, 18));

    audioButton.setBounds(area.removeFromTop(40).withSizeKeepingCentre(180, 34));
}
