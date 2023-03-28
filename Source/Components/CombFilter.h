#pragma once
#include "pa.h"

// Simple comb filter, no damping or any other bells and whistles

namespace pa::dsp {

class CombFilter {
 public:
    // Parameter object
    struct Parameters {
        float freq = 0.0f,
              wet = 0.0f,
              feedback = 0.0f;
        pa::dsp::InterpolationType interpType = noInterp;
    };

    void prepare(uint newSampleRate) {
        // prepare the buffer for playback
        delay.prepare(newSampleRate, newSampleRate);
        delay.setDelayTime(0);
    }

    void setParameters(const Parameters& newParams, const float& freqOffset) {
        Parameters& p = parameters; // just used for shorthand

        p = newParams;
        pa::math::setClamp<float>(&p.feedback, 0.0f, 1.0f);

        delay.setDelayTime(1.0f / (p.freq + freqOffset), 0.03f);
    }

    float process(const float* input) {
        Parameters& p = parameters; // just used for shorthand

        // get delayed signal
        float delayed {};
        getDelay(&delayed);

        float feedbackLine = *input + delayed * p.feedback;
        delay.pushToBuffer(&feedbackLine);

        return *input + delayed * p.wet;
    }

 private:
    pa::dsp::RingBuffer<float> delay;

    Parameters parameters;

    void getDelay(float* input) {
        switch (parameters.interpType) {
            case pa::dsp::noInterp:
                delay.getFromBuffer(input, pa::dsp::noInterp);
                break;
            case pa::dsp::linearInterp:
                delay.getFromBuffer(input, pa::dsp::linearInterp);
                break;
            case pa::dsp::cubicInterp:
                delay.getFromBuffer(input, pa::dsp::cubicInterp);
                break;
            default:
                delay.getFromBuffer(input, pa::dsp::noInterp);
                break;
        }
    }
};

} // end namespace pa::dsp