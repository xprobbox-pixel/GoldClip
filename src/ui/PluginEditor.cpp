#include "PluginEditor.h"
namespace aurum
{
using namespace juce;
namespace
{
const Colour gold(0xfff0ca72), muted(0xffb6b8bf), blue(0xff4b91e8);
void text(Graphics &g, const String &s, Rectangle<float> r, float size, Colour colour = muted,
          int align = Justification::centredLeft)
{
    g.setColour(colour);
    g.setFont(Font(FontOptions(size, Font::bold)));
    g.drawText(s, r, align);
}
String db(float x)
{
    return x > 0 ? String(20 * std::log10(x), 1)
                 : String(CharPointer_UTF8("\xe2\x88\x92\xe2\x88\x9e"));
}
void panel(Graphics &g, Rectangle<float> r)
{
    g.setColour(Colour(0xff101114));
    g.fillRoundedRectangle(r, 12);
    g.setColour(Colour(0xff45464f));
    g.drawRoundedRectangle(r, 12, 1);
}
} // namespace
CustomLookAndFeel::CustomLookAndFeel()
{
    setColour(Slider::textBoxTextColourId, gold);
    setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour(ComboBox::backgroundColourId, Colour(0xff222329));
    setColour(ComboBox::textColourId, gold);
    setColour(PopupMenu::backgroundColourId, Colour(0xff222329));
    setColour(PopupMenu::textColourId, gold);
}
void CustomLookAndFeel::drawRotarySlider(Graphics &g, int x, int y, int w, int h, float pos,
                                         float start, float end, Slider &s)
{
    auto r = Rectangle<float>(float(x), float(y), float(w), float(h)).reduced(8);
    auto centre = r.getCentre();
    const float radius = jmin(r.getWidth(), r.getHeight()) * .5f;
    g.setGradientFill(ColourGradient(Colour(0xff5b5d67), centre.x, centre.y - radius,
                                     Colour(0xff17181c), centre.x, centre.y + radius, false));
    g.fillEllipse(centre.x - radius, centre.y - radius, 2 * radius, 2 * radius);
    g.setColour(Colour(0xff777980));
    g.drawEllipse(centre.x - radius, centre.y - radius, 2 * radius, 2 * radius, 1);
    Path arc;
    arc.addCentredArc(centre.x, centre.y, radius - 3, radius - 3, 0, start,
                      start + pos * (end - start), true);
    g.setColour(s.isEnabled() ? (s.getName() == "ALCHEMY" ? blue : gold) : Colour(0xff676971));
    g.strokePath(arc, PathStrokeType(3));
    auto endPoint = centre + Point<float>(std::sin(start + pos * (end - start)),
                                          -std::cos(start + pos * (end - start))) *
                                 (radius * .68f);
    g.setColour(Colours::white);
    g.drawLine({centre, endPoint}, 2);
}
void CustomLookAndFeel::drawToggleButton(Graphics &g, ToggleButton &b, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced(1);
    g.setColour(Colour(over ? 0xff35363d : 0xff222329));
    g.fillRoundedRectangle(r, 6);
    g.setColour(b.getToggleState() ? gold : Colour(0xff61636d));
    g.drawRoundedRectangle(r, 6, 1);
    text(g, b.getButtonText(), r, 12, b.getToggleState() ? gold : muted, Justification::centred);
}
void CustomLookAndFeel::drawCornerResizer(Graphics &g, int w, int h, bool, bool)
{
    g.setColour(gold);
    for (int i = 4; i < 17; i += 5)
        g.drawLine(float(w - i), float(h - 2), float(w - 2), float(h - i), 2);
}
void CustomLookAndFeel::drawLinearSlider(Graphics &g, int x, int y, int w, int h, float position,
                                         float, float, const Slider::SliderStyle style, Slider &)
{
    const bool vertical = style == Slider::LinearVertical;
    g.setColour(Colour(0xff08090b));
    if (vertical)
        g.fillRoundedRectangle(float(x + w / 2 - 3), float(y), 6, float(h), 3);
    else
        g.fillRoundedRectangle(float(x), float(y + h / 2 - 3), float(w), 6, 3);
    g.setColour(gold);
    if (vertical)
    {
        g.fillRoundedRectangle(float(x + w / 2 - 11), position - 5, 22, 10, 3);
        g.setColour(Colours::white);
        g.drawHorizontalLine(roundToInt(position), float(x + w / 2 - 7), float(x + w / 2 + 7));
    }
    else
    {
        g.fillEllipse(position - 5, float(y + h / 2 - 5), 10, 10);
    }
}
PluginEditor::PluginEditor(PluginProcessor &p)
    : AudioProcessorEditor(p), processor(p), baseWidth(p.isCompact() ? 520 : 1200)
{
    setLookAndFeel(&look);
    canvas.setInterceptsMouseClicks(false, true);
    addAndMakeVisible(canvas);
    loadPreferences();
    tips.setMillisecondsBeforeTipAppears(tooltips ? 500 : 1000000000);
    const char *ids[]{"input",      "output",  "blend",   "wet",       "dry",
                      "goldModern", "alchemy", "ceiling", "trimOffset"};
    const char *names[]{"INPUT TRIM", "OUTPUT TRIM", "WET BLEND",    "WET LEVEL",  "DRY LEVEL",
                        "GOLD",       "ALCHEMY",     "CLIP CEILING", "TRIM OFFSET"};
    const char *descriptions[]{
        "Wet-path input trim, before oversampling. Original dry input is unchanged.",
        "Final combined output trim. With Trim Link, set offset in Settings; output follows offset "
        "minus input.",
        "Linked linear wet/dry blend. Both paths use fixed compensated delay.",
        "Independent wet gain, active when Wet/Dry Link is off.",
        "Independent original dry gain, active when Wet/Dry Link is off.",
        "Instantaneous ceiling-relative bottom-up enhancement. Modern 0-2.5 dB; Classic 0-6 dB. "
        "Independent approximations.",
        "Stereo-linked dynamic upper-frequency contour, 0-2 dB. Master only.",
        "Nonlinear-stage ceiling. The optional wet sample guard controls reconstructed wet "
        "samples; parallel sums can exceed it. No true-peak guarantee.",
        "Linked trim sum: effective Output = Offset minus Input. Shift-drag Input changes offset."};
    for (size_t i = 0; i < sliders.size(); ++i)
    {
        if (i == 6 && p.isCompact())
            continue;
        auto &s = sliders[i];
        s.setName(names[i]);
        s.setTooltip(descriptions[i]);
        s.setSliderStyle(i == 5 || i == 6 ? Slider::RotaryVerticalDrag
                                          : (i == 2 || i >= 7 ? Slider::LinearHorizontal
                                                              : Slider::LinearVertical));
        s.setTextBoxStyle(Slider::TextBoxBelow, false, i < 5 ? 65 : 85, 22);
        s.setNumDecimalPlacesToDisplay(1);
        s.setTextValueSuffix(i == 2 ? " %" : (i == 7 ? " dBFS" : " dB"));
        s.setDoubleClickReturnValue(true, i == 2 ? 100 : i == 4 ? -60 : 0);
        canvas.addAndMakeVisible(s);
        labels[i].setText(names[i], dontSendNotification);
        labels[i].setFont(Font(FontOptions(11, Font::bold)));
        labels[i].setJustificationType(Justification::centred);
        canvas.addAndMakeVisible(labels[i]);
        if (i != 5)
            sliderLinks[i] = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
                p.parameters.state, ids[i], s);
    }
    sliders[6].setColour(Slider::rotarySliderFillColourId, blue);
    const char *boolIds[]{"linked",    "wetMute",    "dryMute",  "delta",     "bypass",
                          "goldUnity", "trimLinked", "waveform", "peakGuard", "clipFilter"};
    const char *boolNames[]{"LINK WET/DRY", "MUTE WET",   "MUTE DRY", "DELTA",     "BYPASS",
                            "GOLD UNITY",   "LINK TRIMS", "WAVEFORM", "WET GUARD", "CLIP FILTER"};
    for (size_t i = 0; i < toggles.size(); ++i)
    {
        auto &button = toggles[i];
        button.setButtonText(boolNames[i]);
        button.setName(boolNames[i]);
        button.setTooltip(
            i == 3   ? "Output aligned wet minus original dry, then Output Trim. This affects "
                       "rendered audio."
            : i == 4 ? "Crossfade to delayed unity dry. Host bypass immediately selects the same "
                       "compensated dry input."
            : i == 5 ? "Compensate Gold enhancement by its selected dB amount, independently of "
                       "trims and mixing."
            : i == 6 ? "Link trims while preserving their current sum. Settings edits the offset; "
                       "Shift-drag Input adjusts it."
            : i == 8 ? "32-sample lookahead, stereo-linked sample-peak gain control on "
                       "reconstructed wet only. Final mix and true peaks are not guaranteed."
            : i == 9 ? "Optional one-pole low-pass at the configured cutoff, after Gold inside the "
                       "oversampled wet chain."
                     : "Automatable audio/display setting. Double-click sliders to reset; numeric "
                       "fields support keyboard entry.");
        canvas.addAndMakeVisible(button);
        toggleLinks[i] = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(
            p.parameters.state, boolIds[i], button);
    }
    toggles[6].onClick = [this]
    {
        if (toggles[6].getToggleState())
        {
            const auto q = processor.parameters.snapshot();
            setParameter("trimOffset", float(q.inputDb + q.outputDb));
        }
        else
        {
            const auto q = processor.parameters.snapshot();
            setParameter("output", float(jlimit(-12., 12., q.trimOffset - q.inputDb)));
        }
    };
    sliders[0].onDragStart = [this]
    {
        if (ModifierKeys::getCurrentModifiersRealtime().isShiftDown() &&
            toggles[6].getToggleState())
            sliders[0].setTooltip("Linked offset is available as a separate Settings control; "
                                  "Shift-drag changes it.");
    };
    sliders[0].onValueChange = [this]
    {
        if (ModifierKeys::getCurrentModifiersRealtime().isShiftDown() &&
            toggles[6].getToggleState())
            setParameter("trimOffset", float(sliders[0].getValue() + sliders[1].getValue()));
    };
    clipMode.addItemList({"Off", "Hard", "Classic", "Modern"}, 1);
    goldMode.addItemList({"Modern Gold", "Classic Gold"}, 1);
    boxTone.addItemList({"Flat", "Classic", "Modern"}, 1);
    for (auto *c : {&clipMode, &goldMode, &scale, &preset})
        canvas.addAndMakeVisible(*c);
    clipMode.setName("Clip Mode");
    goldMode.setName("Gold Curve");
    preset.setName("Preset");
    clipLink = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(
        p.parameters.state, "clipMode", clipMode);
    goldModeLink = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(
        p.parameters.state, "goldMode", goldMode);
    if (!p.isCompact())
    {
        canvas.addAndMakeVisible(boxTone);
        boxTone.setName("Box Tone");
        toneLink = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(
            p.parameters.state, "boxTone", boxTone);
    }
    for (int percent : {80, 100, 125, 150, 175})
        scale.addItem(String(percent) + "%", percent);
    scale.setName("Window scale");
    scale.setTooltip("80-175% interface scaling. Drag the corner for intermediate sizes.");
    scale.onChange = [this]
    {
        if (scale.getSelectedId() > 0)
            setSize(roundToInt(baseWidth * scale.getSelectedId() / 100.),
                    roundToInt(baseHeight * scale.getSelectedId() / 100.));
    };
    for (auto *button : {&reset, &settings, &save, &previous, &next, &a, &b, &copy})
        canvas.addAndMakeVisible(*button);
    reset.onClick = [this]
    {
        inputHold = {};
        outputHold = {};
        overloaded = false;
        processor.telemetry.resetMeasurements();
    };
    reset.setTooltip(
        "Reset peak holds, overload, Clip Clock and short-term loudness startup window.");
    settings.onClick = [this] { showSettings(); };
    save.onClick = [this] { namePreset(false); };
    a.onClick = [this] { processor.presets.selectAB(0); };
    b.onClick = [this] { processor.presets.selectAB(1); };
    copy.onClick = [this]
    { processor.presets.copyAB(processor.presets.activeAB(), 1 - processor.presets.activeAB()); };
    preset.onChange = [this]
    {
        const int selected = preset.getSelectedId();
        if (selected > 0 && selected <= processor.presets.factoryNames().size())
            processor.presets.factory(selected - 1);
        else if (selected >= 100 && selected - 100 < userFiles.size())
            processor.presets.loadUser(userFiles[selected - 100]);
    };
    previous.onClick = [this]
    {
        int i = preset.getSelectedItemIndex();
        preset.setSelectedItemIndex(i <= 0 ? preset.getNumItems() - 1 : i - 1);
    };
    next.onClick = [this]
    {
        int i = preset.getSelectedItemIndex();
        preset.setSelectedItemIndex((i + 1) % jmax(1, preset.getNumItems()));
    };
    refreshPresets();
    for (auto &hit : scaleHits)
    {
        canvas.addAndMakeVisible(hit);
        hit.cycle = [this] { setParameter("meterScale", float((meterScale + 1) % 3)); };
        hit.setName("Meter scale: double-click to cycle");
    }
    setResizable(true, true);
    const int saved = jlimit(int(baseWidth * .8), int(baseWidth * 1.75), p.editorWidth.load());
    setResizeLimits(int(baseWidth * .8), 480, int(baseWidth * 1.75), 1050);
    getConstrainer()->setFixedAspectRatio(double(baseWidth) / 600);
    setSize(saved, roundToInt(double(saved) * 600 / baseWidth));
    timerCallback();
    startTimerHz(30);
}
PluginEditor::~PluginEditor()
{
    stopTimer();
    savePreferences();
    setLookAndFeel(nullptr);
}
void PluginEditor::setParameter(const char *id, float value)
{
    if (auto *p = processor.parameters.state.getParameter(id))
        p->setValueNotifyingHost(p->convertTo0to1(value));
}
File PluginEditor::preferencesFile() const
{
    return File::getSpecialLocation(File::userApplicationDataDirectory)
        .getChildFile("AurumClip")
        .getChildFile(processor.isCompact() ? "Track" : "Master")
        .getChildFile("Preferences.xml");
}
void PluginEditor::loadPreferences()
{
    if (SystemStats::getEnvironmentVariable("AURUM_NO_SETTINGS", "") == "1")
        return;
    if (auto xml = XmlDocument::parse(preferencesFile()))
    {
        tooltips = xml->getBoolAttribute("tooltips", true);
        meterScale = jlimit(0, 2, xml->getIntAttribute("meterScale"));
        if (!processor.hostStateRestored.load() && processor.editorWidth == baseWidth)
            processor.editorWidth = jlimit(int(baseWidth * .8), int(baseWidth * 1.75),
                                           xml->getIntAttribute("width", baseWidth));
    }
}
void PluginEditor::savePreferences()
{
    if (SystemStats::getEnvironmentVariable("AURUM_NO_SETTINGS", "") == "1")
        return;
    auto file = preferencesFile();
    file.getParentDirectory().createDirectory();
    XmlElement xml("Preferences");
    xml.setAttribute("tooltips", tooltips);
    xml.setAttribute("meterScale", meterScale);
    xml.setAttribute("width", getWidth());
    xml.writeTo(file);
}
void PluginEditor::refreshPresets()
{
    const auto current = processor.presets.name;
    preset.clear(dontSendNotification);
    int id = 1;
    for (const auto &name : processor.presets.factoryNames())
        preset.addItem(name, id++);
    userFiles = processor.presets.userPresets();
    if (!userFiles.isEmpty())
        preset.addSeparator();
    for (int i = 0; i < userFiles.size(); ++i)
        preset.addItem("User / " + userFiles[i].getFileNameWithoutExtension(), 100 + i);
    preset.setText((processor.presets.userPreset ? "User / " : "") + current, dontSendNotification);
}
void PluginEditor::namePreset(bool rename)
{
    auto *dialog = new AlertWindow(rename ? "Rename preset" : "Save user preset",
                                   "Enter a preset name.", MessageBoxIconType::NoIcon);
    dialog->addTextEditor("name", processor.presets.name);
    dialog->addButton("Cancel", 0);
    dialog->addButton("Save", 1, KeyPress(KeyPress::returnKey));
    Component::SafePointer<PluginEditor> self(this);
    dialog->enterModalState(
        true,
        ModalCallbackFunction::create(
            [self, dialog, rename](int result)
            {
                if (!self || result != 1)
                    return;
                const auto name = dialog->getTextEditorContents("name");
                bool ok = false;
                if (rename)
                {
                    for (auto file : self->userFiles)
                        if (file.getFileNameWithoutExtension() == self->processor.presets.name)
                        {
                            ok = self->processor.presets.renameUser(file, name);

                            break;
                        }
                }
                else
                    ok = self->processor.presets.saveUser(name);
                if (ok)
                    self->refreshPresets();
                else
                    AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, "Preset",
                                                     "Could not save or rename this preset.");
            }),
        true);
}
void PluginEditor::editNumber(const char *id, const String &title)
{
    auto *p = processor.parameters.state.getParameter(id);
    if (!p)
        return;
    auto *dialog = new AlertWindow(title, "Enter a numeric value.", MessageBoxIconType::NoIcon);
    dialog->addTextEditor("value", String(p->convertFrom0to1(p->getValue()), 1));
    dialog->addButton("Cancel", 0);
    dialog->addButton("Apply", 1, KeyPress(KeyPress::returnKey));
    Component::SafePointer<PluginEditor> self(this);
    String key(id);
    dialog->enterModalState(true,
                            ModalCallbackFunction::create(
                                [self, dialog, key](int result)
                                {
                                    if (self && result == 1)
                                        self->setParameter(
                                            key.toRawUTF8(),
                                            dialog->getTextEditorContents("value").getFloatValue());
                                }),
                            true);
}
void PluginEditor::showSettings()
{
    PopupMenu menu;
    auto q = processor.parameters.snapshot();
    auto choices = [&](const String &title, const StringArray &options, int base, int selected)
    {
        PopupMenu sub;
        for (int i = 0; i < options.size(); ++i)
            sub.addItem(base + i, options[i], true, i == selected);
        menu.addSubMenu(title, sub);
    };
    choices("Realtime quality", {"High", "Pristine", "Extra Pristine"}, 100, q.quality);
    choices("Offline quality", {"High", "Pristine", "Extra Pristine"}, 110, q.offlineQuality);
    choices("Oversampling phase", {"Linear FIR", "Minimum IIR"}, 120, q.filterPhase);
    menu.addItem(1, "Clip Filter", true, q.clipFilter);
    menu.addItem(2, "Clip Filter cutoff...");
    menu.addItem(3, "Wet sample-peak guard", true, q.peakGuard);
    menu.addItem(4, "Tooltips", true, tooltips);
    menu.addItem(5, "Linked trim offset...");
    menu.addItem(6, "Wet/Dry link", true, q.linked);
    menu.addItem(7, "Independent wet level...");
    menu.addItem(8, "Independent dry level...");
    menu.addItem(9, "Mute wet", true, q.wetMute);
    menu.addItem(10, "Mute dry", true, q.dryMute);
    menu.addItem(11, "Delta monitor", true, q.delta);
    menu.addSeparator();
    menu.addItem(12, "Copy A to B");
    menu.addItem(13, "Copy B to A");
    menu.addItem(14, "Make current preset the default");
    menu.addItem(15, "Rename user preset...");
    menu.addItem(16, "Delete current user preset");
    menu.addItem(17, "Cycle meter scale");
    Component::SafePointer<PluginEditor> self(this);
    menu.showMenuAsync(
        PopupMenu::Options().withTargetComponent(settings),
        [self](int result)
        {
            if (!self)
                return;
            auto q = self->processor.parameters.snapshot();
            if (result >= 100 && result < 103)
                self->setParameter("quality", float(result - 100));
            else if (result >= 110 && result < 113)
                self->setParameter("offlineQuality", float(result - 110));
            else if (result >= 120 && result < 122)
                self->setParameter("filterPhase", float(result - 120));
            else
                switch (result)
                {
                case 1:
                    self->setParameter("clipFilter", !q.clipFilter);
                    break;
                case 2:
                    self->editNumber("clipFilterHz", "Clip Filter cutoff (Hz)");
                    break;
                case 3:
                    self->setParameter("peakGuard", !q.peakGuard);
                    break;
                case 4:
                    self->tooltips = !self->tooltips;
                    self->tips.setMillisecondsBeforeTipAppears(self->tooltips ? 500 : 1000000000);
                    break;
                case 5:
                    self->editNumber("trimOffset", "Linked trim offset (dB)");
                    break;
                case 6:
                    self->setParameter("linked", !q.linked);
                    break;
                case 7:
                    self->editNumber("wet", "Independent wet level (dB)");
                    break;
                case 8:
                    self->editNumber("dry", "Independent dry level (dB)");
                    break;
                case 9:
                    self->setParameter("wetMute", !q.wetMute);
                    break;
                case 10:
                    self->setParameter("dryMute", !q.dryMute);
                    break;
                case 11:
                    self->setParameter("delta", !q.delta);
                    break;
                case 12:
                    self->processor.presets.copyAB(0, 1);
                    break;
                case 13:
                    self->processor.presets.copyAB(1, 0);
                    break;
                case 14:
                {
                    if (!self->processor.presets.makeDefault())
                        AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon,
                                                         "Default preset",
                                                         "Could not save the default preset.");
                    self->refreshPresets();
                    break;
                }
                case 15:
                    self->namePreset(true);
                    break;
                case 16:
                    for (auto file : self->userFiles)
                        if (file.getFileNameWithoutExtension() == self->processor.presets.name)
                        {
                            if (!self->processor.presets.removeUser(file))
                            {
                                AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon,
                                                                 "Preset",
                                                                 "Could not delete this preset.");
                                break;
                            }
                            self->processor.presets.name = "Session";
                            self->processor.presets.userPreset = false;
                            self->refreshPresets();
                            break;
                        }
                    break;
                case 17:
                    self->setParameter("meterScale", float((self->meterScale + 1) % 3));
                    break;
                default:
                    break;
                }
        });
}
void PluginEditor::resized()
{
    canvas.setBounds(0, 0, baseWidth, 600);
    canvas.setTransform(AffineTransform::scale(getWidth() / float(baseWidth), getHeight() / 600.f));
    processor.editorWidth = getWidth();
    const bool t = processor.isCompact();
    if (t)
    {
        sliders[0].setBounds(18, 145, 74, 270);
        sliders[1].setBounds(428, 145, 74, 270);
        sliders[5].setBounds(196, 283, 128, 110);
        labels[5].setBounds(196, 267, 128, 20);
        sliders[7].setBounds(140, 449, 240, 48);
        labels[7].setBounds(140, 430, 240, 18);
        sliders[2].setBounds(140, 510, 240, 48);
        labels[2].setBounds(140, 494, 240, 18);
        clipMode.setBounds(20, 442, 100, 26);
        goldMode.setBounds(180, 89, 160, 26);
        toggles[5].setBounds(197, 408, 126, 27);
        toggles[6].setBounds(20, 496, 105, 27);
        toggles[7].setBounds(395, 444, 105, 27);
        reset.setBounds(395, 496, 105, 27);
        scale.setBounds(415, 555, 85, 25);
    }
    else
    {
        sliders[0].setBounds(161, 122, 75, 316);
        sliders[1].setBounds(926, 122, 75, 316);
        sliders[3].setBounds(1017, 122, 74, 316);
        sliders[4].setBounds(1104, 122, 74, 316);
        sliders[5].setBounds(298, 365, 120, 124);
        sliders[6].setBounds(752, 365, 120, 124);
        labels[5].setBounds(298, 345, 120, 20);
        labels[6].setBounds(752, 345, 120, 20);
        sliders[7].setBounds(445, 377, 270, 52);
        labels[7].setBounds(445, 357, 270, 20);
        sliders[2].setBounds(1015, 492, 164, 48);
        labels[2].setBounds(1015, 473, 164, 20);
        clipMode.setBounds(22, 205, 115, 26);
        goldMode.setBounds(22, 281, 115, 26);
        boxTone.setBounds(22, 129, 115, 26);
        toggles[0].setBounds(1018, 80, 160, 28);
        toggles[1].setBounds(1018, 442, 74, 26);
        toggles[2].setBounds(1104, 442, 74, 26);
        toggles[3].setBounds(1018, 543, 160, 26);
        toggles[5].setBounds(294, 495, 128, 28);
        toggles[6].setBounds(490, 447, 160, 28);
        toggles[7].setBounds(752, 495, 128, 28);
        reset.setBounds(22, 483, 115, 28);
        scale.setBounds(1085, 18, 95, 26);
    }
    for (size_t i = 0; i < sliders.size(); ++i)
    {
        if (i == 6 && t)
            continue;
        if (i < 2)
            labels[i].setBounds(t ? (i == 0 ? 12 : 422) : (i == 0 ? 155 : 920), t ? 120 : 100,
                                t ? 86 : 90, 20);
        if (i == 3 || i == 4)
        {
            sliders[i].setVisible(!t);
            labels[i].setVisible(!t);
            labels[i].setBounds(i == 3 ? 1014 : 1101, 100, 80, 20);
        }
        if (i == 8)
        {
            sliders[i].setVisible(false);
            labels[i].setVisible(false);
        }
    }
    for (size_t i : {size_t(0), size_t(1), size_t(2), size_t(3)})
        toggles[i].setVisible(!t);
    toggles[8].setVisible(false);
    toggles[9].setVisible(false);
    const int headerY = t ? 52 : 18;
    previous.setBounds(t ? 18 : 175, headerY, 24, 25);
    next.setBounds(t ? 46 : 203, headerY, 24, 25);
    preset.setBounds(t ? 154 : 235, 18, t ? 248 : 270, 26);
    save.setBounds(t ? 76 : 512, headerY, t ? 48 : 58, 25);
    a.setBounds(t ? 140 : 582, headerY, 30, 25);
    b.setBounds(t ? 174 : 616, headerY, 30, 25);
    copy.setBounds(t ? 211 : 653, headerY, 64, 25);
    settings.setBounds(t ? 282 : 730, headerY, 85, 25);
    toggles[4].setBounds(t ? 383 : 850, headerY, t ? 117 : 104, 26);
    scaleHits[0].setBounds(t ? 12 : 155, t ? 358 : 376, t ? 86 : 90, 16);
    scaleHits[1].setBounds(t ? 422 : 920, t ? 358 : 376, t ? 86 : 90, 16);
    const double percent = 100. * getWidth() / baseWidth;
    bool exact = false;
    for (int v : {80, 100, 125, 150, 175})
        if (getWidth() == roundToInt(baseWidth * v / 100.))
        {
            scale.setSelectedId(v, dontSendNotification);
            exact = true;
            break;
        }
    if (!exact)
        scale.setText(String(percent, 1) + "%", dontSendNotification);
}
void PluginEditor::timerCallback()
{
    const int desired =
        jlimit(int(baseWidth * .8), int(baseWidth * 1.75), processor.editorWidth.load());
    if (desired != getWidth())
        setSize(desired, roundToInt(double(desired) * 600 / baseWidth));
    Envelope frame;
    unsigned drained = 0;
    while (drained++ < 1024 && processor.telemetry.pop(frame))
    {
        history[head] = frame;
        head = (head + 1) % unsigned(history.size());
    }
    auto peaks = processor.telemetry.consumePeaks();
    for (size_t c = 0; c < 2; ++c)
    {
        inputHold[c] = jmax(inputHold[c], peaks.input[c]);
        outputHold[c] = jmax(outputHold[c], peaks.output[c]);
        inputNow[c] = jmax(peaks.input[c], inputNow[c] * .86f);
        outputNow[c] = jmax(peaks.output[c], outputNow[c] * .86f);
    }
    overloaded = overloaded || peaks.overloaded;
    analysis = processor.telemetry.analysis();
    const auto q = processor.parameters.snapshot();
    meterScale = q.meterScale;
    if (lastGoldMode != q.goldMode)
    {
        sliderLinks[5].reset();
        sliderLinks[5] = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters.state, q.goldMode == 0 ? "goldModern" : "goldClassic", sliders[5]);
        lastGoldMode = q.goldMode;
    }
    sliders[2].setEnabled(q.linked);
    sliders[3].setEnabled(!q.linked);
    sliders[4].setEnabled(!q.linked);
    sliders[1].setEnabled(!q.trimLinked);
    if (q.trimLinked)
        sliders[1].setValue(jlimit(-12., 12., q.trimOffset - q.inputDb), dontSendNotification);
    if (q.trimLinked)
        sliders[1].setTooltip("Effective output " +
                              String(jlimit(-12., 12., q.trimOffset - q.inputDb), 1) +
                              " dB; edit linked offset in Settings.");
    a.setColour(TextButton::buttonColourId,
                processor.presets.activeAB() == 0 ? Colour(0xff6c5525) : Colour(0xff222329));
    b.setColour(TextButton::buttonColourId,
                processor.presets.activeAB() == 1 ? Colour(0xff6c5525) : Colour(0xff222329));
    copy.setButtonText(processor.presets.activeAB() == 0 ? "A > B" : "B > A");
    if (!preset.isPopupActive())
        preset.setText((processor.presets.userPreset ? "User / " : "") + processor.presets.name,
                       dontSendNotification);
    repaint();
}
void PluginEditor::drawWaveform(Graphics &g, Rectangle<float> r)
{
    panel(g, r);
    if (!processor.parameters.snapshot().waveform)
    {
        text(g, "WAVEFORM OFF", r, 14, gold, Justification::centred);
        return;
    }
    g.saveState();
    g.reduceClipRegion(r.reduced(2).toNearestInt());
    const float maximum = float(gain(meterScale == 0 ? 6 : meterScale == 1 ? 4 : 2));
    auto y = [&](float value)
    { return r.getCentreY() - jlimit(-1.2f, 1.2f, value / maximum) * r.getHeight() * .42f; };
    g.setColour(Colour(0xff292c34));
    for (int row = 1; row < 4; ++row)
        g.drawHorizontalLine(roundToInt(r.getY() + r.getHeight() * row / 4), r.getX(),
                             r.getRight());
    for (unsigned i = 0; i < history.size(); ++i)
    {
        const auto &v = history[(head + i) % unsigned(history.size())];
        const float x = r.getX() + r.getWidth() * i / float(history.size());
        g.setColour(Colour(0xff757983));
        g.drawVerticalLine(roundToInt(x), y(v.inputMax), y(v.inputMin));
        g.setColour(Colour(0xff7b5b27));
        g.drawVerticalLine(roundToInt(x), y(v.outputMax), y(v.outputMin));
        g.setColour(gold);
        g.drawVerticalLine(roundToInt(x), y(v.outputMax), y(v.outputMax) + 1);
        g.drawVerticalLine(roundToInt(x), y(v.outputMin) - 1, y(v.outputMin));
    }
    const float ceiling = float(gain(processor.parameters.snapshot().ceilingDb));
    g.setColour(blue);
    g.drawHorizontalLine(roundToInt(y(ceiling)), r.getX(), r.getRight());
    g.drawHorizontalLine(roundToInt(y(-ceiling)), r.getX(), r.getRight());
    g.restoreState();
}
void PluginEditor::drawMeter(Graphics &g, Rectangle<float> r, bool input)
{
    panel(g, r);
    const auto &current = input ? inputNow : outputNow;
    const auto &hold = input ? inputHold : outputHold;
    const float maximum = meterScale == 0   ? 6.f
                          : meterScale == 1 ? 4.f
                                            : 2.f,
                minimum = meterScale == 0   ? -36.f
                          : meterScale == 1 ? -24.f
                                            : -12.f;
    const float bottom = r.getBottom() - 52, top = r.getY() + 22;
    auto y = [&](float dbValue) {
        return bottom -
               jlimit(0.f, 1.f, (dbValue - minimum) / (maximum - minimum)) * (bottom - top);
    };
    for (int c = 0; c < 2; ++c)
    {
        const float x = r.getX() + 7 + c * (r.getWidth() - 25);
        const float value = current[size_t(c)] > 0 ? 20 * std::log10(current[size_t(c)]) : -120;
        for (float level = minimum; level <= maximum; level += 2)
        {
            g.setColour(level <= value ? (level >= 0 ? Colour(0xffd96d5c) : gold)
                                       : Colour(0xff343239));
            g.fillRect(x, y(level), 11.f, 3.f);
        }
        text(g, db(hold[size_t(c)]),
             {r.getX() + c * r.getWidth() / 2, bottom + 9, r.getWidth() / 2, 18}, 9, gold,
             Justification::centred);
    }
    if (input)
    {
        g.setColour(blue);
        g.drawHorizontalLine(roundToInt(y(float(processor.parameters.snapshot().ceilingDb))),
                             r.getX() + 2, r.getRight() - 2);
    }
    text(g, String(int(maximum)) + " / " + String(int(minimum)),
         {r.getX(), r.getBottom() - 16, r.getWidth(), 14}, 8, muted, Justification::centred);
}
void PluginEditor::mouseDoubleClick(const MouseEvent &event)
{
    const auto p = event.position / float(getWidth() / double(baseWidth));
    if (!processor.isCompact() && p.x < 150 && p.y > 330 && p.y < 480)
    {
        reset.onClick();
        return;
    }
    const bool track = processor.isCompact();
    const auto left =
        track ? Rectangle<float>(12, 142, 86, 232) : Rectangle<float>(155, 122, 90, 270);
    const auto right =
        track ? Rectangle<float>(422, 142, 86, 232) : Rectangle<float>(920, 122, 90, 270);
    if (left.contains(p) || right.contains(p))
    {
        meterScale = (processor.parameters.snapshot().meterScale + 1) % 3;
        setParameter("meterScale", float(meterScale));
        repaint();
    }
}
void PluginEditor::paint(Graphics &original)
{
    Graphics::ScopedSaveState saved(original);
    original.addTransform(
        AffineTransform::scale(getWidth() / float(baseWidth), getHeight() / 600.f));
    auto &g = original;
    g.fillAll(Colour(0xff17181c));
    g.setGradientFill(ColourGradient(Colour(0xff34353d), 0, 0, Colour(0xff17181c), 0, 140, false));
    g.fillRoundedRectangle(2, 2, float(baseWidth - 4), 596, 14);
    g.setColour(Colour(0xff555760));
    g.drawRoundedRectangle({2, 2, float(baseWidth - 4), 596}, 14, 2);
    g.setColour(gold);
    g.fillRect(18, processor.isCompact() ? 82 : 60, baseWidth - 36, 1);
    text(g, "AURUM", {18, 12, 145, 28}, processor.isCompact() ? 22 : 25, gold);
    const bool t = processor.isCompact();
    text(g, t ? "CLIP TRACK" : "CLIP MASTER", {18, 40, 140, 12}, 9, gold);
    const auto q = processor.parameters.snapshot();
    drawMeter(g, t ? Rectangle<float>(12, 142, 86, 232) : Rectangle<float>(155, 122, 90, 270),
              true);
    drawMeter(g, t ? Rectangle<float>(422, 142, 86, 232) : Rectangle<float>(920, 122, 90, 270),
              false);
    drawWaveform(g, t ? Rectangle<float>(117, 124, 286, 132) : Rectangle<float>(258, 92, 647, 245));
    auto loudness = [](float value)
    {
        return std::isnan(value)   ? String(CharPointer_UTF8("\xe2\x80\x94"))
               : std::isinf(value) ? String(CharPointer_UTF8("\xe2\x88\x92\xe2\x88\x9e"))
                                   : String(value, 1);
    };
    text(g, "IN " + loudness(analysis.inputLufs) + " LUFS",
         t ? Rectangle<float>(12, 421, 115, 18) : Rectangle<float>(150, 444, 110, 20), t ? 9 : 10);
    text(g, "OUT " + loudness(analysis.outputLufs) + " LUFS",
         t ? Rectangle<float>(397, 421, 115, 18) : Rectangle<float>(918, 444, 110, 20), t ? 9 : 10);
    if (!t)
    {
        panel(g, {14, 92, 130, 438});
        text(g, "BOX TONE", {22, 101, 115, 20}, 11, gold);
        text(g, "CLIPPER", {22, 177, 115, 20}, 11, gold);
        text(g, "GOLD CURVE", {22, 253, 115, 20}, 11, gold);
        text(g, "MAX CLIP", {22, 332, 115, 20}, 11);
        text(g, String(analysis.clip, 1) + " dB", {22, 355, 115, 26}, 21, gold);
        text(g, "TIMELINE", {22, 395, 115, 20}, 10);
        String time = CharPointer_UTF8("\xe2\x80\x94");
        if (std::isfinite(analysis.seconds) && analysis.seconds >= 0)
            time = String(int(analysis.seconds) / 60) + ":" +
                   String(int(analysis.seconds) % 60).paddedLeft('0', 2);
        text(g, time, {22, 416, 115, 26}, 20, gold);
        text(g, "STAGE CEILING", {445, 434, 270, 17}, 9, blue, Justification::centred);
    }
    if (q.trimLinked)
        text(g, "OUT = " + String(jlimit(-12., 12., q.trimOffset - q.inputDb), 1) + " dB",
             t ? Rectangle<float>(390, 530, 125, 18) : Rectangle<float>(480, 480, 180, 18), 10,
             gold, Justification::centred);
    if (overloaded || (q.clipMode == 0 && (jmax(inputHold[0], inputHold[1]) > gain(q.ceilingDb))))
        text(g, q.clipMode == 0 ? "CLIPPER OFF: ABOVE CEILING" : "OUTPUT OVERLOAD",
             t ? Rectangle<float>(118, 127, 286, 18) : Rectangle<float>(470, 500, 270, 23),
             t ? 10 : 12, Colour(0xfff47a70), Justification::centred);
    text(g,
         "RT " + StringArray{"High", "Pristine", "Extra"}[q.quality] + " | OFFLINE " +
             StringArray{"High", "Pristine", "Extra"}[q.offlineQuality] + " | " +
             (q.filterPhase == 0 ? "FIR" : "IIR") + " | " + String(processor.getLatencySamples()) +
             " samples",
         t ? Rectangle<float>(18, 557, 390, 18) : Rectangle<float>(250, 548, 650, 20), t ? 9 : 12,
         gold);
    text(g,
         String(JucePlugin_VersionString) + " | " + (t ? "TRACK" : "MASTER") + " | " +
             (q.peakGuard ? "WET SAMPLE GUARD ON" : "WET GUARD OFF") + " | NOT TRUE-PEAK LIMITED",
         {18, 581, float(baseWidth - 36), 14}, t ? 8 : 10, muted);
}
} // namespace aurum
