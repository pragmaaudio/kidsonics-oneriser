#pragma once
#include "pa.h"
#include "CombFilter.h"
#include "Filter.h"
#include "Reverb.h"

class RiserProcessor {
 public:
    RiserProcessor() {
        // initialise parameter objects
        flangerParams.freq = 3000.0f;
        flangerParams.feedback = 0.5f;
        flangerParams.wet = 0.0f;
        flangerParams.interpType = pa::dsp::linearInterp;

        //  //  //  //  //

        lowpassParams.enabled = true;
        lowpassParams.q = 0.5f;
        lowpassParams.cutoff = 20000.0f;
        lowpassParams.type = pa::dsp::Filter::lowpass;

        highpassParams.enabled = true;
        highpassParams.q = M_SQRT1_2;
        highpassParams.cutoff = 10.0f;
        highpassParams.type = pa::dsp::Filter::highpass;

        //  //  //  //  //

        reverbParams.width = 1.0f;
        reverbParams.damping = 0.6f;
        reverbParams.mix = 0.0f;
        reverbParams.size = 0.2f;
        reverbParams.spread = 6.5f;

        vector<float> earlyTimes { 0.0053f, 0.0134f, 0.0229f, 0.030f, 0.0092f, 0.0158f, 0.0397f, 0.0184f };
        vector<float> lateTimes { 0.0111f, 0.0175f, 0.0076f, 0.0152f };

        for (uint i = 0; i < 8; i++) {
            reverb.setEarlyCombTime(earlyTimes[i], i);

            if (i < 4)
                reverb.setLateCombTime(lateTimes[i], i);
        }
    }

    void prepare(uint sampleRate) {
        // prepare the processors for playback
        reverb.prepare(sampleRate);

        for (size_t i = 0; i < 2; i++) {
            flanger[i].prepare(sampleRate);
            lowpass[i].prepare(sampleRate);
            highpass[i].prepare(sampleRate);
        }

        // map the correct values before playback too
        calculateValues();
    }

    void setParameters(const float& newDoublerAmount, const float& newFilterAmount,
                       const float& newReverbAmount, const float& newMasterAmount) {
        reverbAmount  = pa::math::clamp(newReverbAmount, 0.0f, 1.0f);
        filterAmount  = pa::math::clamp(newFilterAmount, 0.0f, 1.0f);
        flangerAmount = pa::math::clamp(newDoublerAmount, 0.0f, 1.0f);
        masterAmount  = pa::math::clamp(newMasterAmount, 0.0f, 1.0f);

        reverbAmount  *= masterAmount;
        filterAmount  *= masterAmount;
        flangerAmount *= masterAmount;

        calculateValues();
    }

    // Processes a block of samples, i.e. the current buffer
    // comb -> lowpass -> highpass -> reverb (in series)
    void process(float* left, float* right, const int& numSamples) {
        if (left == nullptr || right == nullptr || numSamples <= 0) return;

        // process loop
        for (int i = 0; i < numSamples; i++) {
            float outL = left[i], outR = right[i];

            outL = flanger[0].process(&outL);
            outR = flanger[1].process(&outR);

            outL = lowpass[0].process(&outL);
            outR = lowpass[1].process(&outR);

            outL = highpass[0].process(&outL);
            outR = highpass[1].process(&outR);

            reverb.process(&outL, &outR);

            // hard-clip for protection, intended for development only
            constexpr float ceil = 1.2f;
            pa::math::setClamp(&outL, -ceil, ceil);
            pa::math::setClamp(&outR, -ceil, ceil);

            left[i] = outL;
            right[i] = outR;
        }
    }

 private:
    float masterAmount = 0.0f, reverbAmount = 0.65f, filterAmount = 1.0f, flangerAmount = 0.7f;
    array<pa::dsp::CombFilter, 2> flanger;
    array<pa::dsp::Filter, 2> lowpass;
    array<pa::dsp::Filter, 2> highpass;
    pa::dsp::Reverb reverb;

    pa::dsp::CombFilter::Parameters flangerParams;
    pa::dsp::Filter::Parameters lowpassParams, highpassParams;
    pa::dsp::Reverb::Parameters reverbParams;

    // calculate the value mappings, and set the processors' values
    void calculateValues() {
        // flanger — map the wet, frequency, feedback
        flangerParams.wet      = mapValue(pa::math::expRounder(flangerAmount, 0.3f), 0, 0.75f);
        flangerParams.freq     = mapValue(flangerAmount, 20.0f, 280.0f);
        flangerParams.feedback = mapValue(flangerAmount, 0.0f, 0.55f);

        // filters — map the cutoff and q for both filters
        lowpassParams.cutoff  = mapValue(pa::math::expRounder(filterAmount, 0.3f), 20000.0f, 4000.0f);
        lowpassParams.q       = mapValue(pa::math::expRounder(filterAmount, -0.6f), 0.5f, 0.85f);
        highpassParams.cutoff = mapValue(pa::math::expRounder(filterAmount, -0.3f), 10.0f, 200.0f);
        highpassParams.q      = mapValue(pa::math::expRounder(filterAmount, -0.5f), static_cast<float>(M_SQRT1_2), 1.0f);

        // reverb — map the mix, size, width and stereo spread
        reverbParams.mix    = mapValue(reverbAmount, 0.0f, 0.75f);
        reverbParams.size   = mapValue(reverbAmount, 0.01f, 0.45f);
        reverbParams.width  = mapValue(reverbAmount, 1.0f, 0.6f);
        reverbParams.spread = mapValue(pa::math::expRounder(reverbAmount, 0.3f), 0.5f, 1.5f);

        //          //          //          //          //

        // set parameter objects
        flanger[0].setParameters(flangerParams, 0.0f);
        flanger[1].setParameters(flangerParams, 7.0f * pa::math::expRounder(flangerAmount, -0.4f));

        for (size_t i = 0; i < 2; i++) {
            lowpass[i].setParameters(lowpassParams);
            highpass[i].setParameters(highpassParams);
        }

        reverb.setParameters(reverbParams);
    }

    // Function to prevent having to type out the input range every time
    static inline float mapValue(const float& val, const float& min, const float& max) {
        return pa::math::map<float>(val, 0, 1, min, max);
    }
};