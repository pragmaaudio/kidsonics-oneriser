#pragma once
#include "PluginProcessor.h"

class OneRiserEditor : public juce::AudioProcessorEditor {
public:
    explicit OneRiserEditor(OneRiserProcessor&);
    ~OneRiserEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void setKnob(Slider& knob, Label& label, const bool& isSmallKnob = true);
    void valueChanged();
    void onKnobChange(Slider& knob, Label& label, bool& enabledState);
    static void onLabelChange(Slider& knob, Label& label);
    void setLabelFonts();
    void checkMasterLabelState();

 private:
    OneRiserProcessor& processorRef;

    // could use containers for these, little easier to set them up separately
    // because they all have slightly different requirements
    Slider flangerKnob, filterKnob, reverbKnob, masterKnob;
    Label flangerAmount, filterAmount, reverbAmount, masterAmount;

    KnobAppearance smallKnobLookFeel, largeKnobLookFeel;
    TooltipWindow tooltipWindow;
    Image backgroundImage;
    Font fontMuli;

    // used to track the state of all the smaller knobs so the master label can
    // be dimmed when they're all at 0 (set in valueChanged())
    bool reverbEnabled {}, filterEnabled {}, flangerEnabled {};

    // used to attach the knobs to the parameter objects in the processor's value tree
    // (allows them to be saved/recalled, etc.)
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> flangerAttachment,
                                   filterAttachment, reverbAttachment, masterAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OneRiserEditor)
};