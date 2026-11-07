#include "AmpPageComponent.h"
#include "GraphicUtils.h"
#include "HardwareTheme.h"

DigitalPreamp* AmpPageComponent::preamp() const
{
    return dynamic_cast<DigitalPreamp*>(engine.getChain().findById("preamp").get());
}

KnobStyle AmpPageComponent::knobStyleForModel(int model) const
{
    if (model == 1) return KnobStyle::AmpChrome;
    return KnobStyle::AmpSkirted;
}

juce::Colour AmpPageComponent::labelColourForModel(int model) const
{
    if (model == 1) return juce::Colour(0xff2a2418);
    return juce::Colour(0xfff3e6c4);
}

AmpPageComponent::AmpPageComponent(AudioEngine& engineToUse) : engine(engineToUse)
{
    auto* amp = preamp();
    if (amp == nullptr)
        return;

    modelBox.addItem("Deluxe Reverb", 1);
    modelBox.addItem("Steel String", 2);
    modelBox.setSelectedId(juce::jlimit(1, 2, amp->getModelIndex() + 1), juce::dontSendNotification);
    modelBox.onChange = [this]
    {
        if (auto* a = preamp())
        {
            a->setModelIndex(modelBox.getSelectedId() - 1);
            rebuildAmpControls();
            resized();
            repaint();
        }
    };
    addAndMakeVisible(modelBox);

    powerButton.setButtonText({});
    powerButton.setClickingTogglesState(true);
    powerButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    powerButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    powerButton.setLookAndFeel(&knobsLaf);
    powerButton.getProperties().set("footswitchStyle", (int) FootswitchStyle::None);
    powerButton.setToggleState(! amp->isBypassed(), juce::dontSendNotification);
    powerButton.onClick = [this]
    {
        if (auto* a = preamp())
            a->setBypassed(! powerButton.getToggleState());
        repaint();
    };
    addAndMakeVisible(powerButton);

    valueReadout.setJustificationType(juce::Justification::centred);
    valueReadout.setColour(juce::Label::textColourId, juce::Colour(0xff2a1c12));
    valueReadout.setColour(juce::Label::backgroundColourId, juce::Colour(0xddefe0c0));
    valueReadout.setFont(gfx::fontCondensed(11.0f));
    valueReadout.setInterceptsMouseClicks(false, false);
    valueReadout.setVisible(false);
    addAndMakeVisible(valueReadout);

    auto setupCabKnob = [this] (juce::Slider& s, juce::Label& label, auto setter, float initial)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setRange(0.0, 1.0, 0.001);
        s.setValue(initial, juce::dontSendNotification);
        s.setLookAndFeel(&knobsLaf);
        s.setRotaryParameters(juce::degreesToRadians(225.0f), juce::degreesToRadians(495.0f), true);
        s.setMouseDragSensitivity(180);
        KnobLookAndFeel::applyKnobStyle(s, KnobStyle::AmpVintage);
        s.onValueChange = [&s, setter, this]
        {
            setter((float) s.getValue());
            showValue(s);
        };
        s.onDragEnd = [this] { valueReadout.setVisible(false); liveSlider = nullptr; };
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colour(0xfff3e6c4));
        label.setFont(gfx::fontCondensed(12.0f));
        label.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(s);
        addAndMakeVisible(label);
    };

    setupCabKnob(cabBody, cabBodyLabel, [this] (float v) { engine.getCabinet().setBody(v); }, engine.getCabinet().getBody());
    setupCabKnob(cabAir, cabAirLabel, [this] (float v) { engine.getCabinet().setAir(v); }, engine.getCabinet().getAir());
    setupCabKnob(cabLevel, cabLevelLabel, [this] (float v) { engine.getCabinet().setLevel(v); }, engine.getCabinet().getLevel());
    cabLevel.onValueChange = [this]
    {
        engine.getCabinet().setLevel((float) cabLevel.getValue());
        liveSlider = &cabLevel;
        const float db = CabinetSim::levelToDecibels((float) cabLevel.getValue());
        valueReadout.setText((db >= 0.0f ? "+" : "") + juce::String(db, 1) + " dB", juce::dontSendNotification);
        valueReadout.setVisible(true);
        valueReadout.toFront(false);
        valueReadout.setBounds(cabLevel.getBounds().withSizeKeepingCentre(52, 18));
    };

    cabEnable.setClickingTogglesState(true);
    cabEnable.setLookAndFeel(&knobsLaf);
    KnobLookAndFeel::applyChrome(cabEnable, "gold");
    cabEnable.onClick = [this]
    {
        engine.getCabinet().setEnabled(cabEnable.getToggleState());
        updateCabEnabled();
        repaint();
    };
    addAndMakeVisible(cabEnable);

    irBox.onChange = [this]
    {
        const auto name = irBox.getText();
        if (name.isEmpty() || name == "(filter cab)")
        {
            engine.getCabinet().clearImpulse();
            return;
        }
        engine.getCabinet().loadImpulseFromFile(engine.getCabinet().getIrFolder().getChildFile(name));
    };
    addAndMakeVisible(irBox);

    irFolderButton.setLookAndFeel(&knobsLaf);
    KnobLookAndFeel::applyChrome(irFolderButton, "steel");
    irFolderButton.onClick = [this] { chooseIrFolder(); };
    addAndMakeVisible(irFolderButton);

    rebuildAmpControls();
    refreshIrList();
    updateCabEnabled();
}

void AmpPageComponent::rebuildAmpControls()
{
    auto* amp = preamp();
    if (amp == nullptr)
        return;

    for (auto& s : sliders)
        s->setLookAndFeel(nullptr);
    sliders.clear();
    sliderLabels.clear();
    switches.clear();

    const int model = amp->getModelIndex();
    const auto style = knobStyleForModel(model);
    const auto textColour = labelColourForModel(model);

    for (const auto& p : amp->getParamList())
    {
        auto slider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,
                                                     juce::Slider::NoTextBox);
        const auto paramId = p.id;

        if (p.isSwitch)
        {
            slider->setRange(0.0, 1.0, 1.0);
            slider->setValue(amp->getParameter(p.id) >= 0.5f ? 1.0 : 0.0, juce::dontSendNotification);
            slider->setLookAndFeel(&knobsLaf);
            slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            slider->setRotaryParameters(juce::degreesToRadians(225.0f), juce::degreesToRadians(495.0f), true);
            slider->setMouseDragSensitivity(200);
            KnobLookAndFeel::applyKnobStyle(*slider, KnobStyle::AmpToggle);
            slider->onValueChange = [this, paramId, s = slider.get()]
            {
                if (auto* a = preamp())
                    a->setParameter(paramId, s->getValue() >= 0.5 ? 1.0f : 0.0f);
                repaint();
            };
        }
        else
        {
            slider->setRange(0.0, 1.0, 0.001);
            slider->setValue(amp->getParameter(p.id), juce::dontSendNotification);
            slider->setLookAndFeel(&knobsLaf);
            slider->setRotaryParameters(juce::degreesToRadians(225.0f), juce::degreesToRadians(495.0f), true);
            slider->setMouseDragSensitivity(180);
            slider->setVelocityBasedMode(true);
            slider->setVelocityModeParameters(0.4, 1, 0.09, true);
            slider->setDoubleClickReturnValue(true, p.defaultValue);
            KnobLookAndFeel::applyKnobStyle(*slider, style);
            slider->onValueChange = [this, paramId, s = slider.get()]
            {
                if (auto* a = preamp())
                    a->setParameter(paramId, (float) s->getValue());
                liveSlider = s;
                valueReadout.setText(juce::String(juce::roundToInt(s->getValue() * 100.0)), juce::dontSendNotification);
                valueReadout.setVisible(true);
                valueReadout.toFront(false);
                if (liveSlider != nullptr)
                    valueReadout.setBounds(liveSlider->getBounds().withSizeKeepingCentre(40, 18));
            };
            slider->onDragEnd = [this]
            {
                liveSlider = nullptr;
                valueReadout.setVisible(false);
            };
        }
        addAndMakeVisible(*slider);

        auto label = std::make_unique<juce::Label>();
        label->setText(p.label.toUpperCase(), juce::dontSendNotification);
        label->setJustificationType(juce::Justification::centred);
        label->setColour(juce::Label::textColourId, textColour);
        label->setFont(gfx::fontCondensed(11.0f));
        label->setInterceptsMouseClicks(false, false);
        addAndMakeVisible(*label);

        sliders.push_back(std::move(slider));
        sliderLabels.push_back(std::move(label));
    }
}

void AmpPageComponent::showValue(juce::Slider& slider)
{
    liveSlider = &slider;
    valueReadout.setText(juce::String(juce::roundToInt(slider.getValue() * 100.0)), juce::dontSendNotification);
    valueReadout.setVisible(true);
    valueReadout.toFront(false);
    valueReadout.setBounds(slider.getBounds().withSizeKeepingCentre(40, 18));
}

void AmpPageComponent::updateCabEnabled()
{
    const bool on = engine.getCabinet().isEnabled();
    cabEnable.setToggleState(on, juce::dontSendNotification);
    cabEnable.setButtonText(on ? "CAB SIM ON" : "CAB SIM OFF");
    cabBody.setEnabled(on);
    cabAir.setEnabled(on);
    cabLevel.setEnabled(on);
    irBox.setEnabled(on);
    irFolderButton.setEnabled(on);
}

void AmpPageComponent::refreshIrList()
{
    irBox.clear(juce::dontSendNotification);
    irBox.addItem("(filter cab)", 1);
    int id = 2;
    int selected = 1;
    const auto loaded = engine.getCabinet().getLoadedIrName();
    for (const auto& name : engine.getCabinet().listImpulseFiles())
    {
        irBox.addItem(name, id);
        if (name == loaded)
            selected = id;
        ++id;
    }
    irBox.setSelectedId(selected, juce::dontSendNotification);
}

void AmpPageComponent::chooseIrFolder()
{
    auto chooser = std::make_shared<juce::FileChooser>("Choose IR folder", engine.getCabinet().getIrFolder());
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                         [this, chooser] (const juce::FileChooser& fc)
                         {
                             auto result = fc.getResult();
                             if (result.isDirectory())
                             {
                                 engine.getCabinet().setIrFolder(result);
                                 refreshIrList();
                             }
                         });
}

AmpPageComponent::~AmpPageComponent()
{
    powerButton.setLookAndFeel(nullptr);
    cabEnable.setLookAndFeel(nullptr);
    cabBody.setLookAndFeel(nullptr);
    cabAir.setLookAndFeel(nullptr);
    cabLevel.setLookAndFeel(nullptr);
    irFolderButton.setLookAndFeel(nullptr);
    for (auto& s : sliders)
        s->setLookAndFeel(nullptr);
    for (auto& b : switches)
        b->setLookAndFeel(nullptr);
}

void AmpPageComponent::refresh()
{
    auto* amp = preamp();
    if (amp == nullptr)
        return;

    modelBox.setSelectedId(juce::jlimit(1, 2, amp->getModelIndex() + 1), juce::dontSendNotification);
    powerButton.setToggleState(! amp->isBypassed(), juce::dontSendNotification);
    rebuildAmpControls();
    cabBody.setValue(engine.getCabinet().getBody(), juce::dontSendNotification);
    cabAir.setValue(engine.getCabinet().getAir(), juce::dontSendNotification);
    cabLevel.setValue(engine.getCabinet().getLevel(), juce::dontSendNotification);
    refreshIrList();
    updateCabEnabled();
    resized();
    repaint();
}

AmpPageComponent::Layout AmpPageComponent::computeLayout() const
{
    Layout layout;
    auto area = getLocalBounds().reduced(28, 16);
    layout.modelBar = area.removeFromTop(34).reduced(8, 4);
    const int comboW = juce::jlimit(640, 980, area.getWidth() - 40);
    const int comboH = juce::jlimit(440, 700, area.getHeight() - 12);
    layout.combo = juce::Rectangle<int>(comboW, comboH).withCentre(area.getCentre());

    const auto fullCombo = layout.combo;
    layout.head = fullCombo.withHeight(juce::roundToInt((float) fullCombo.getHeight() * 0.36f));
    auto rest = fullCombo.withTrimmedTop(layout.head.getHeight());
    layout.cabPanel = rest.removeFromBottom(118);
    layout.cab = rest;

    layout.handle = juce::Rectangle<float>((float) layout.head.getCentreX() - 70.0f,
                                           (float) layout.head.getY() + 8.0f,
                                           140.0f, 16.0f);

    layout.face = layout.head.reduced(18, 14);
    layout.face.removeFromTop(18);
    layout.face.removeFromBottom(10);

    auto strip = layout.face.reduced(10, 6);
    layout.switchRow = strip.removeFromBottom(0);
    layout.knobRow = strip.removeFromTop(juce::jmax(86, strip.getHeight() - 4));

    auto powerCol = layout.knobRow.removeFromRight(juce::jmin(96, layout.knobRow.getWidth() / 6));
    layout.jewel = juce::Rectangle<float>(16.0f, 16.0f)
                       .withCentre({ (float) powerCol.getCentreX(), (float) powerCol.getY() + 14.0f });
    layout.power = powerCol.removeFromBottom(40).reduced(16, 2);
    return layout;
}

void AmpPageComponent::paintGrill(juce::Graphics& g, juce::Rectangle<float> cloth) const
{
    auto* amp = preamp();
    const int model = amp != nullptr ? amp->getModelIndex() : 0;
    juce::Colour a = juce::Colour(0xffc8d4dc), b = juce::Colour(0xff8aa0b0);
    if (model == 1) { a = juce::Colour(0xffd8c4a0); b = juce::Colour(0xffb89a72); }

    juce::ColourGradient clothGrad(a, cloth.getTopLeft(), b, cloth.getBottomRight(), false);
    g.setGradientFill(clothGrad);
    g.fillRoundedRectangle(cloth, 6.0f);
    gfx::weave(g, cloth, juce::Colour(0xffd8c4a0).withAlpha(0.18f),
               juce::Colour(0xff1a0c0a).withAlpha(0.22f), 6.0f);

    auto cone = cloth.reduced(cloth.getWidth() * 0.22f, cloth.getHeight() * 0.12f);
    cone = cone.withSizeKeepingCentre(juce::jmin(cone.getWidth(), cone.getHeight()),
                                      juce::jmin(cone.getWidth(), cone.getHeight()));
    g.setColour(juce::Colours::black.withAlpha(0.22f));
    g.fillEllipse(cone);
    g.setColour(juce::Colour(0xffcfc0a8).withAlpha(0.12f));
    g.drawEllipse(cone.reduced(cone.getWidth() * 0.18f), 8.0f);
    g.setColour(juce::Colour(0xff3a241c).withAlpha(0.55f));
    g.fillEllipse(cone.withSizeKeepingCentre(cone.getWidth() * 0.18f, cone.getHeight() * 0.18f));
    g.setColour(juce::Colour(0xffc4a574).withAlpha(0.55f));
    g.drawRoundedRectangle(cloth.reduced(1.0f), 6.0f, 1.6f);
}

void AmpPageComponent::paintCabinet(juce::Graphics& g, juce::Rectangle<float> cab) const
{
    gfx::tolex(g, cab, juce::Colour(0xff1a1a1a), 12.0f);
    gfx::stitch(g, cab, juce::Colour(0xffc9a45a), 8.0f);
    g.setColour(juce::Colour(0xffc9a45a).withAlpha(0.45f));
    g.drawRoundedRectangle(cab.reduced(3.0f), 10.0f, 1.4f);

    auto cloth = cab.reduced(22.0f, 18.0f);
    paintGrill(g, cloth);

    auto* amp = preamp();
    const int model = amp != nullptr ? amp->getModelIndex() : 0;
    auto plate = juce::Rectangle<float>(cloth.getWidth() * 0.60f, cloth.getHeight() * 0.32f)
                     .withCentre(cloth.getCentre());
    auto leather = plate.expanded(8.0f, 7.0f);
    g.setColour(juce::Colour(0xff1a1210));
    g.fillRoundedRectangle(leather, 4.0f);
    gfx::stitch(g, leather, juce::Colour(0xffc9a45a), 4.0f);
    gfx::anodized(g, plate, juce::Colour(0xffc8c8c8), 3.0f);
    gfx::brushed(g, plate, juce::Colour(0xff9a9a9a), true, 11);
    g.setColour(juce::Colour(0xff6a6a6a).withAlpha(0.55f));
    g.drawRoundedRectangle(plate, 3.0f, 1.1f);
    gfx::screws(g, plate, 7.0f, 5.5f);

    auto text = plate.reduced(12.0f, 6.0f);
    if (model == 1)
    {
        g.setColour(juce::Colour(0xff1a1a1a));
        g.setFont(gfx::fontImpact(18.0f));
        g.drawText("STEEL STRING SINGER", text, juce::Justification::centred);
    }
    else
    {
        g.setColour(juce::Colour(0xff1a1a1a));
        g.setFont(gfx::fontBrush(26.0f));
        g.drawText("Deluxe Reverb", text, juce::Justification::centred);
    }
}

void AmpPageComponent::paintToggle(juce::Graphics& g, juce::Rectangle<float> area, bool on) const
{
    auto boot = area.reduced(2.0f);
    g.setColour(juce::Colour(0xff1a1210));
    g.fillRoundedRectangle(boot, 4.0f);
    g.setColour(juce::Colour(0xffc9a45a).withAlpha(0.4f));
    g.drawRoundedRectangle(boot, 4.0f, 1.0f);
    auto lever = juce::Rectangle<float>(10.0f, boot.getHeight() * 0.62f)
                     .withCentre({ boot.getCentreX(), on ? boot.getY() + 10.0f : boot.getBottom() - 10.0f });
    juce::ColourGradient chrome(juce::Colour(0xfff0ece4), lever.getTopLeft(),
                                juce::Colour(0xff8a8478), lever.getBottomRight(), false);
    g.setGradientFill(chrome);
    g.fillRoundedRectangle(lever, 3.0f);
}

void AmpPageComponent::paintHead(juce::Graphics& g, const Layout& layout) const
{
    auto* amp = preamp();
    const int model = amp != nullptr ? amp->getModelIndex() : 0;
    auto head = layout.head.toFloat();
    gfx::tolex(g, head, juce::Colour(0xff161616), 12.0f);
    gfx::stitch(g, head, juce::Colour(0xffc9a45a), 7.0f);

    auto handle = layout.handle;
    g.setColour(juce::Colour(0xff1a1210));
    g.fillRoundedRectangle(handle.expanded(10.0f, 3.0f), 8.0f);
    juce::ColourGradient leather(juce::Colour(0xff5a3a28), handle.getTopLeft(),
                                 juce::Colour(0xff2a1810), handle.getBottomRight(), false);
    g.setGradientFill(leather);
    g.fillRoundedRectangle(handle, 6.0f);

    auto face = layout.face.toFloat();
    if (model == 1)
    {
        gfx::anodized(g, face, juce::Colour(0xffd8c8a8), 6.0f);
        g.setColour(juce::Colour(0xff5a4630).withAlpha(0.55f));
        g.drawRoundedRectangle(face.reduced(0.8f), 6.0f, 1.2f);
    }
    else
    {
        g.setColour(juce::Colour(0xff111111));
        g.fillRoundedRectangle(face, 6.0f);
        gfx::speckles(g, face, juce::Colours::white.withAlpha(0.06f), 40, 1.1f, 3);
    }

    const bool on = powerButton.getToggleState();
    auto jewel = layout.jewel;
    g.setColour(juce::Colour(0xff3a1a12));
    g.fillEllipse(jewel.expanded(2.5f));
    g.setColour(on ? juce::Colour(0xffff2a22) : juce::Colour(0xff401410));
    g.fillEllipse(jewel);
    if (on)
    {
        g.setColour(juce::Colours::white.withAlpha(0.45f));
        g.fillEllipse(jewel.reduced(4.5f).translated(-1.5f, -1.5f));
    }
    paintToggle(g, layout.power.toFloat(), on);
}

void AmpPageComponent::paint(juce::Graphics& g)
{
    theme::studioFloor(g, getLocalBounds().toFloat());
    const auto layout = computeLayout();
    auto combo = layout.head.toFloat().getUnion(layout.cab.toFloat()).expanded(10.0f, 12.0f);
    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.fillRoundedRectangle(combo.translated(0.0f, 8.0f), 16.0f);
    paintHead(g, layout);
    paintCabinet(g, layout.cab.toFloat());
    auto cabPanel = layout.cabPanel.toFloat();
    gfx::tolex(g, cabPanel, juce::Colour(0xff2a1812), 10.0f);
    g.setColour(juce::Colour(0xffc9a45a).withAlpha(0.4f));
    g.drawRoundedRectangle(cabPanel.reduced(1.5f), 10.0f, 1.2f);
}

void AmpPageComponent::resized()
{
    const auto layout = computeLayout();
    modelBox.setBounds(layout.modelBar.withWidth(juce::jmin(260, layout.modelBar.getWidth())));
    powerButton.setBounds(layout.power);

    auto row = layout.knobRow;
    if (! sliders.empty())
    {
        const int w = row.getWidth() / (int) sliders.size();
        for (size_t i = 0; i < sliders.size(); ++i)
        {
            auto col = row.removeFromLeft(w);
            sliderLabels[i]->setBounds(col.removeFromBottom(18));
            sliders[i]->setBounds(col.reduced(6, 2));
        }
    }

    auto cab = layout.cabPanel.reduced(12, 10);
    auto irRow = cab.removeFromTop(28);
    irBox.setBounds(irRow.removeFromLeft(juce::jmax(180, irRow.getWidth() / 2)).reduced(2, 2));
    irFolderButton.setBounds(irRow.reduced(2, 2));
    cabEnable.setBounds(cab.removeFromLeft(130).reduced(4, 6));
    const int knobW = cab.getWidth() / 3;
    auto bodyCol = cab.removeFromLeft(knobW);
    auto airCol = cab.removeFromLeft(knobW);
    cabBody.setBounds(bodyCol.removeFromTop(bodyCol.getHeight() - 16).reduced(8, 0));
    cabBodyLabel.setBounds(bodyCol);
    cabAir.setBounds(airCol.removeFromTop(airCol.getHeight() - 16).reduced(8, 0));
    cabAirLabel.setBounds(airCol);
    cabLevel.setBounds(cab.removeFromTop(cab.getHeight() - 16).reduced(8, 0));
    cabLevelLabel.setBounds(cab);
}
