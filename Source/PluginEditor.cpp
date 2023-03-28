#include "PluginProcessor.h"
#include "PluginEditor.h"

OneRiserEditor::OneRiserEditor(OneRiserProcessor& p) : AudioProcessorEditor(&p), processorRef(p) {
    // set look/feel knob states
    smallKnobLookFeel.knobType = KnobAppearance::small;
    largeKnobLookFeel.knobType = KnobAppearance::large;

    // set SliderAttachment properties
    flangerAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>
                        (processorRef.parameters, "FLG_AMT", flangerKnob);
    filterAttachment =  std::make_unique<AudioProcessorValueTreeState::SliderAttachment>
                        (processorRef.parameters, "FIL_AMT", filterKnob);
    reverbAttachment =  std::make_unique<AudioProcessorValueTreeState::SliderAttachment>
                        (processorRef.parameters, "REV_AMT", reverbKnob);
    masterAttachment =  std::make_unique<AudioProcessorValueTreeState::SliderAttachment>
                        (processorRef.parameters, "MAS_AMT", masterKnob);

    // set font and background image from binary
    fontMuli = Typeface::createSystemTypefaceFor
        (BinaryData::MuliBold_ttf, BinaryData::MuliBold_ttfSize);
    backgroundImage = ImageFileFormat::loadFrom
        (BinaryData::Background2_5_png, BinaryData::Background2_5_pngSize);

    // functions for when a knob's value is changed
    flangerKnob.onValueChange = [&]() { onKnobChange(flangerKnob, flangerAmount, flangerEnabled); };
    filterKnob.onValueChange = [&]() { onKnobChange(filterKnob, filterAmount, filterEnabled); };
    reverbKnob.onValueChange = [&]() { onKnobChange(reverbKnob, reverbAmount, reverbEnabled); };

    masterKnob.onValueChange = [&]() {
        String valStr;
        double val = masterKnob.getValue();

        if (val == 0) valStr = "0.00 %";
        else if (val >= 0.9995) valStr = "100 %";
        else if (val < 0.01) valStr = String(val * 100, 2) + " %";
        else valStr = String::toDecimalStringWithSignificantFigures(val * 100, 3) + " %";

        masterAmount.setText(valStr, dontSendNotification);

        valueChanged();
    };

    // functions for when a knob's label is changed
    flangerAmount.onTextChange = [&]() { onLabelChange(flangerKnob, flangerAmount); };
    filterAmount.onTextChange = [&]() { onLabelChange(filterKnob, filterAmount); };
    reverbAmount.onTextChange = [&]() { onLabelChange(reverbKnob, reverbAmount); };
    masterAmount.onTextChange = [&]() { onLabelChange(masterKnob, masterAmount); };

    setLabelFonts();

    // setup each knob and label
    setKnob(flangerKnob, flangerAmount);
    setKnob(reverbKnob, reverbAmount);
    setKnob(filterKnob, filterAmount);
    setKnob(masterKnob, masterAmount, false);

    // set reset values
    const auto resetKey = ModifierKeys::commandModifier;
    flangerKnob.setDoubleClickReturnValue(true, 0.65f, resetKey);
    filterKnob.setDoubleClickReturnValue(true,  1.00f, resetKey);
    reverbKnob.setDoubleClickReturnValue(true,  0.70f, resetKey);
    masterKnob.setDoubleClickReturnValue(true,  0.00f, resetKey);

    // tooltip text for each control
    reverbKnob.setTooltip("Controls the room size, width amount, and mix level of a reverb");
    filterKnob.setTooltip("Controls the cutoff frequencies of high- and low-pass filters");
    flangerKnob.setTooltip("Controls the delay time, feedback amount, and mix level of a flanger");
    masterKnob.setTooltip("Scales the intensity of all effects in the processor chain\n"
                          "The chain runs in series: flanger -> filter -> reverb");

    // ensure values are initialised
    flangerKnob.setValue(0.65f);
    filterKnob.setValue(1.00f);
    reverbKnob.setValue(0.70f);
    masterKnob.setValue(0.00f);

    // set resizing constraints
    setResizable(true, true);
    constexpr float ratio = 8.0f / 11.0f;
    setResizeLimits(320, int(320 / ratio), 440, int(440 / ratio));
    getConstrainer()->setFixedAspectRatio(ratio);
    setSize(320, 440);
}

OneRiserEditor::~OneRiserEditor() = default;

void OneRiserEditor::paint(juce::Graphics& g) {
    // Fill the background with a solid colour
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

    g.drawImageWithin(backgroundImage, 0, 0, getWidth(), getHeight(), RectanglePlacement::centred);
}

void OneRiserEditor::resized() {
    // naive positioning method, but it works for now.
    // TODO replace using rectangle objects
    int w = getWidth(), h = getHeight();
    int smallKnobY = h / 2 + (float(h) / 4.65f);
    int smallKnobSize = float(w) / 6.6f;

    reverbKnob.setBounds(w / 2 + (float(w) / 3.725f) - smallKnobSize / 2, smallKnobY, smallKnobSize, smallKnobSize);
    filterKnob.setBounds(w / 2 - smallKnobSize / 2, smallKnobY, smallKnobSize, smallKnobSize);
    flangerKnob.setBounds(w / 2 - (float(w) / 3.725f) - smallKnobSize / 2, smallKnobY, smallKnobSize, smallKnobSize);

    float labelWidth = float(smallKnobSize) * 1.15f;

    reverbAmount.setBounds(w / 2 + (float(w) / 3.725f) - labelWidth / 2, smallKnobY + float(h) / 7.55f, labelWidth, float(smallKnobSize) / 2.5f);
    filterAmount.setBounds(w / 2 - labelWidth / 2, smallKnobY + float(h) / 7.55f, labelWidth, float(smallKnobSize) / 2.5f);
    flangerAmount.setBounds(w / 2 - (float(w) / 3.725f) - labelWidth / 2, smallKnobY + float(h) / 7.55f, labelWidth, float(smallKnobSize) / 2.5f);

    int largeKnobSize = float(smallKnobSize) * 2.45f;
    masterKnob.setBounds(w / 2 - largeKnobSize / 2, h / 2 - float(h) / 3.75f, largeKnobSize, largeKnobSize);
    masterAmount.setBounds(w / 2 - labelWidth / 2, h / 2 + h / 28, labelWidth, float(smallKnobSize) / 2.5f);

    /*auto centreSpace = getLocalBounds(), bottomSpace = getLocalBounds();
    auto masterSpace = centreSpace.removeFromTop(static_cast<int>(getHeight() * 0.75));

    // flanger knob y and x pos, respectively
    bottomSpace.removeFromTop(static_cast<int>(getHeight() * 0.717));
    auto flangerSpace = bottomSpace.removeFromLeft(static_cast<int>(getWidth() * 0.462));
    auto filterSpace = bottomSpace;
    auto reverbSpace = bottomSpace.removeFromRight(static_cast<int>(getWidth() * 0.462));

    masterKnob.setBounds(masterSpace.reduced(static_cast<int>(getWidth() * 0.32)));
    // flangerKnob.setBounds(flangerSpace.reduced(static_cast<int>(getWidth() * 0.161), 0));
    // filterKnob.setBounds(bottomSpace.reduced(static_cast<int>(getWidth() * 0.161), 0));
    filterKnob.setBounds(filterSpace.reduced(100, 0));
    // reverbKnob.setBounds(reverbSpace.reduced(static_cast<int>(getWidth() * 0.161), 0));*/

    // TODO the font is slightly too wide for each label, could be tuned
    //  (gets wider when the text is "100 %")
    fontMuli.setHeightWithoutChangingWidth(float(getWidth()) / 20.0f);
    fontMuli.setHorizontalScale(1.25f);

    setLabelFonts();
}

// Used to update the label text and enabled state of each knob
void OneRiserEditor::onKnobChange(Slider& knob, Label& label, bool& enabledState) {
    String valStr;
    const auto val = knob.getValue();
    const bool isZero = val < 0.002;

    enabledState = !isZero;

    // update the processor's parameters
    valueChanged();

    // handles certain value conditions properly
    if (isZero) // 0 %
        valStr = String("0.00 %");
    else if (val >= 0.9995) // 100 %
        valStr = String("100 %");
    else if (val < 0.01) // >1 %
        valStr = String(val * 100, 2) + " %";
    else
        valStr = String::toDecimalStringWithSignificantFigures(val * 100, 3) + " %";

    label.setColour(Label::textColourId, isZero ? Colours::grey : Colours::white);
    label.setText(valStr, dontSendNotification);
}

// Used to pass the parameter values to the processor
void OneRiserEditor::valueChanged() {
    // references for shorthand
    auto& pr = processorRef;
    auto& params = pr.parameters;

    auto flangerAmt = params.getRawParameterValue("FLG_AMT")->load(),
         filterAmt  = params.getRawParameterValue("FIL_AMT")->load(),
         reverbAmt  = params.getRawParameterValue("REV_AMT")->load(),
         masterAmt  = params.getRawParameterValue("MAS_AMT")->load();

    pr.riserProcessor.setParameters(flangerAmt, filterAmt, reverbAmt, masterAmt);

    checkMasterLabelState();
}

// Used to perform the generic setup for each knob
void OneRiserEditor::setKnob(Slider& knob, Label& label, const bool& isSmallKnob) {
    // knob appearance settings
    knob.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, false, 0, 0);
    knob.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
    knob.setMouseCursor(MouseCursor::DraggingHandCursor);
    knob.hideTextBox(true);

    // apply specific settings to small/large knobs
    if (isSmallKnob) {
        knob.setLookAndFeel(&smallKnobLookFeel);
        knob.setMouseDragSensitivity(250);
    }
    else {
        knob.setLookAndFeel(&largeKnobLookFeel);
        knob.setMouseDragSensitivity(360);
    }

    // label settings
    label.setJustificationType(Justification::centred);
    label.setMouseCursor(MouseCursor::IBeamCursor);
    label.setLookAndFeel(&smallKnobLookFeel);
    label.setEditable(true, false, false);

    addAndMakeVisible(knob);
    addAndMakeVisible(label);

    // this is called to ensure the label text is initialised properly upon load
    knob.onValueChange();
}

// Used to handle when a knob's value is set via its label
void OneRiserEditor::onLabelChange(Slider& knob, Label& label) {
    double labelVal = label.getTextValue().getValue();
    knob.setValue(labelVal / 100.0);
    knob.onValueChange();
}

// Used to update each label's font, mainly used when resizing
void OneRiserEditor::setLabelFonts() {
    reverbAmount.setFont(fontMuli);
    filterAmount.setFont(fontMuli);
    flangerAmount.setFont(fontMuli);
    masterAmount.setFont(fontMuli);
}

// Used to grey out the master label if all the small knobs are disabled
void OneRiserEditor::checkMasterLabelState() {
    bool anyEnabled = reverbEnabled || filterEnabled || flangerEnabled;
    masterAmount.setColour(Label::textColourId, anyEnabled ? Colours::white : Colours::grey);
}