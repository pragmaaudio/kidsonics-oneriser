// drawRotarySlider() overloads for custom knob appearances
#pragma once
#include "BinaryData.h"

class KnobAppearance : public juce::LookAndFeel_V4 {
 public:
    enum {
        small,
        large
    } knobType;

    void drawRotarySlider(Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, Slider&) override {
        juce::ignoreUnused(x, y, height);

        // (you could also interpret this enum as a bool directly)
        bool isLarge = knobType == large;

        // load the image files
        Image knobBase, knobTop;

        switch (knobType) {
            case small:
                knobBase = ImageFileFormat::loadFrom
                    (BinaryData::SmallKnobBase_png, BinaryData::SmallKnobBase_pngSize);
                knobTop = ImageFileFormat::loadFrom
                    (BinaryData::SmallKnobTop_png, BinaryData::SmallKnobTop_pngSize);
                break;
            case large:
                knobBase = ImageFileFormat::loadFrom
                    (BinaryData::LargeKnobBase_png, BinaryData::LargeKnobBase_pngSize);
                knobTop = ImageFileFormat::loadFrom
                    (BinaryData::LargeKnobTop_png, BinaryData::LargeKnobTop_pngSize);
                break;
        }

        // transform for rotation/scale
        AffineTransform base, top;

        // rotate the bottom layer less than the top for a "reflective" effect
        float baseAngle = rotaryStartAngle * 0.5f + sliderPosProportional
              * (rotaryEndAngle * 0.2f - rotaryStartAngle * 0.2f) + 3.0f;

        // rotate the top layer within the bounds of the knobs' visual markers

        // (the reason for the ternary here is because the bg image's markers
        // are at slightly different angles for the large knob)
        const float angleOffset = isLarge ? 0.07f : 0.035f;
        rotaryStartAngle /= 1.0f - angleOffset;
        rotaryEndAngle *= 1.0f - angleOffset / 2.0f;

        float topAngle = rotaryStartAngle + sliderPosProportional
                         * (rotaryEndAngle - rotaryStartAngle);

        // apply rotation and scale to the transform - rotation must come
        // first, otherwise "pivotPoint" becomes erroneous
        const float knobWidth = isLarge ? largeKnobWidth : smallKnobWidth;

        const float pivotPoint = knobWidth / 2.0f;
        base = base.rotated(baseAngle, pivotPoint, pivotPoint);
        top = top.rotated(topAngle, pivotPoint, pivotPoint);

        const float knobScale = static_cast<float>(width) / knobWidth;
        base = base.scaled(knobScale);
        top = top.scaled(knobScale);

        // draw the images with their transform
        g.drawImageTransformed(knobBase, base);
        g.drawImageTransformed(knobTop, top);
    }

 private:
    // this is set to the pixel width of the knob assets
    static constexpr float largeKnobWidth = 420.0f, smallKnobWidth = 168.0f;
};