#include "MainComponent.h"
#include "gui/GraphicUtils.h"
#include "gui/HardwareTheme.h"

MainComponent::MainComponent()
    : pedalboard(engine),
      ampPage(engine),
      ioPage(engine, deviceManager)
{
    setSize(1440, 860);
    setOpaque(true);

    auto error = deviceManager.initialise(1, 2, nullptr, true);
    if (error.isNotEmpty())
        juce::Logger::writeToLog("Audio init: " + error);

    auto setup = deviceManager.getAudioDeviceSetup();
    if (setup.bufferSize > 128)
    {
        setup.bufferSize = 128;
        deviceManager.setAudioDeviceSetup(setup, true);
    }

    deviceManager.addAudioCallback(&engine);
    engine.addChangeListener(this);

    title.setFont(gfx::fontSerifItalic(28.0f));
    title.setColour(juce::Label::textColourId, juce::Colour(0xfff3e6c4));
    addAndMakeVisible(title);

    auto setupButton = [this] (juce::TextButton& b, const juce::String& chrome)
    {
        b.setLookAndFeel(&knobsLaf);
        KnobLookAndFeel::applyChrome(b, chrome);
        addAndMakeVisible(b);
    };
    setupButton(audioButton, "teal");
    setupButton(bypassButton, "red");
    setupButton(resetButton, "steel");
    setupButton(saveButton, "green");
    setupButton(pedalsTab, "blue");
    setupButton(ampTab, "gold");
    setupButton(ioTab, "cream");

    audioButton.onClick = [this] { showAudioSettings(); };
    bypassButton.setClickingTogglesState(true);
    bypassButton.onClick = [this]
    {
        engine.setMasterBypass(bypassButton.getToggleState());
        const bool live = ! bypassButton.getToggleState();
        pedalboard.setEnabled(live);
        ampPage.setEnabled(live);
        ioPage.setEnabled(true);
        pedalboard.repaint();
        ampPage.repaint();
    };
    resetButton.onClick = [this]
    {
        engine.factoryReset();
        presetName.setText("Untitled", juce::dontSendNotification);
    };
    saveButton.onClick = [this] { saveCurrentPreset(); };

    makePresetLabel.setColour(juce::Label::textColourId, juce::Colour(0xfff3e6c4).withAlpha(0.75f));
    makePresetLabel.setFont(gfx::fontCondensed(11.0f));
    makePresetLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(makePresetLabel);

    loadPresetLabel.setColour(juce::Label::textColourId, juce::Colour(0xfff3e6c4).withAlpha(0.75f));
    loadPresetLabel.setFont(gfx::fontCondensed(11.0f));
    loadPresetLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(loadPresetLabel);

    presetName.setTextToShowWhenEmpty("Preset name", juce::Colours::grey);
    presetName.setText("Untitled");
    addAndMakeVisible(presetName);

    presetBox.onChange = [this]
    {
        if (presetBox.getSelectedId() > 0)
            loadSelectedPreset();
    };
    addAndMakeVisible(presetBox);

    constexpr int tabGroup = 1001;
    auto setupTab = [this] (juce::TextButton& tab, Page page)
    {
        tab.setClickingTogglesState(true);
        tab.setRadioGroupId(tabGroup);
        tab.onClick = [this, page] { showPage(page); };
    };
    setupTab(pedalsTab, Page::Pedals);
    setupTab(ampTab, Page::Amp);
    setupTab(ioTab, Page::IO);
    pedalsTab.setToggleState(true, juce::dontSendNotification);

    addAndMakeVisible(pedalboard);
    addChildComponent(ampPage);
    addChildComponent(ioPage);
    showPage(Page::Pedals);

    refreshPresetList();
}

MainComponent::~MainComponent()
{
    engine.removeChangeListener(this);
    deviceManager.removeAudioCallback(&engine);

    for (auto* b : { &audioButton, &bypassButton, &resetButton, &saveButton, &pedalsTab, &ampTab, &ioTab })
        b->setLookAndFeel(nullptr);
}

void MainComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    theme::studioFloor(g, bounds);
    gfx::brushed(g, bounds.removeFromTop(120.0f), juce::Colour(0xff3a3228), true, 4);

    auto area = getLocalBounds().reduced(16);
    area.removeFromTop(56);
    area.removeFromTop(56);
    area.removeFromTop(8);
    auto tabs = area.removeFromTop(42).toFloat();
    gfx::vinyl(g, tabs, juce::Colour(0xff1c1814), 10.0f);
    g.setColour(juce::Colour(0xffc9a45a).withAlpha(0.22f));
    g.drawRoundedRectangle(tabs, 10.0f, 1.2f);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(16);
    auto header = area.removeFromTop(56);
    title.setBounds(header.removeFromLeft(320));
    audioButton.setBounds(header.removeFromRight(150).reduced(4));
    resetButton.setBounds(header.removeFromRight(130).reduced(4));
    bypassButton.setBounds(header.removeFromRight(140).reduced(4));

    auto presetRow = area.removeFromTop(56);
    auto makeCol = presetRow.removeFromLeft(300);
    makePresetLabel.setBounds(makeCol.removeFromTop(16).reduced(4, 0));
    presetName.setBounds(makeCol.removeFromLeft(220).reduced(4, 4));
    saveButton.setBounds(makeCol.reduced(4, 4));

    auto loadCol = presetRow.removeFromLeft(250);
    loadPresetLabel.setBounds(loadCol.removeFromTop(16).reduced(4, 0));
    presetBox.setBounds(loadCol.reduced(4, 4));

    area.removeFromTop(8);
    auto tabs = area.removeFromTop(42);
    const int tabW = juce::jmin(220, tabs.getWidth() / 3);
    pedalsTab.setBounds(tabs.removeFromLeft(tabW).reduced(4, 6));
    ampTab.setBounds(tabs.removeFromLeft(tabW).reduced(4, 6));
    ioTab.setBounds(tabs.removeFromLeft(tabW).reduced(4, 6));

    area.removeFromTop(8);
    pedalboard.setBounds(area);
    ampPage.setBounds(area);
    ioPage.setBounds(area);
}

void MainComponent::showPage(Page page)
{
    currentPage = page;
    pedalboard.setVisible(page == Page::Pedals);
    ampPage.setVisible(page == Page::Amp);
    ioPage.setVisible(page == Page::IO);

    if (page == Page::Amp)
        ampPage.refresh();
    else if (page == Page::IO)
        ioPage.sync();
    else
        pedalboard.rebuild();
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster*)
{
    syncGlobalControls();
    pedalboard.rebuild();
    ampPage.refresh();
    ioPage.sync();
}

void MainComponent::syncGlobalControls()
{
    bypassButton.setToggleState(engine.isMasterBypass(), juce::dontSendNotification);
    const bool live = ! engine.isMasterBypass();
    pedalboard.setEnabled(live);
    ampPage.setEnabled(live);
}

void MainComponent::refreshPresetList()
{
    presetBox.clear(juce::dontSendNotification);
    const auto names = presets.listPresetNames();
    for (int i = 0; i < names.size(); ++i)
        presetBox.addItem(names[i], i + 1);
}

void MainComponent::saveCurrentPreset()
{
    auto name = presetName.getText().trim();
    if (name.isEmpty())
        name = "Untitled";

    auto json = engine.toJson();
    if (auto* obj = json.getDynamicObject())
        obj->setProperty("name", name);

    if (presets.savePreset(name, json))
    {
        refreshPresetList();
        presetBox.setText(name, juce::dontSendNotification);
    }
}

void MainComponent::loadSelectedPreset()
{
    const auto name = presetBox.getText();
    auto json = presets.loadPreset(name);
    if (json.isVoid())
        return;
    engine.fromJson(json);
    presetName.setText(name, juce::dontSendNotification);
}

void MainComponent::showAudioSettings()
{
    IOPageComponent::openAudioSettings(deviceManager);
}
