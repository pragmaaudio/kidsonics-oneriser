#pragma once
#include "pa.h"

// Stereo reverb processor (simply a collection of comb filters)

namespace pa::dsp {

class Reverb {
 public:
    // StereoReverb parameter object
    struct Parameters {
        float damping = 0.0f, size = 0.0f, mix = 0.0f,
              width = 0.0f, spread = 0.0f;
        uint numEarlyCombs = 8, numLateCombs = 4;
    };

    // Constructor, which initialises default filter values
    Reverb() {
        // set comb vectors
        earlyCombs.resize(2);
        lateCombs.resize(2);

        for (auto& ch : earlyCombs)
            ch.resize(8);
        for (auto& ch : lateCombs)
            ch.resize(4);

        // arbitrary default comb values, in case none are passed
        earlyCombTimes = { 0.06f, 0.04f, 0.02f, 0.01f, 0.052f, 0.036f, 0.042f, 0.024f };
        lateCombTimes = { 0.011f, 0.054f, 0.033f, 0.023f };
        setCombs();

        // "initialise"
        prepare(44100);
    }

    // Prepare the reverb for playback
    void prepare(uint newSampleRate) {
        // set internal sample rate
        if (newSampleRate != sampleRate && newSampleRate != 0)
            sampleRate = newSampleRate;

        // prepare all filters
        prepareCombs();

        // set all smoothed values
        dampingSmooth.reset(sampleRate, 0.05);
        feedbackSmooth.reset(sampleRate, 0.05);
        drySmooth.reset(sampleRate, 0.05);
        wet1.reset(sampleRate, 0.05);
        wet2.reset(sampleRate, 0.05);

        clear();
    }

    // Clears the reverb's buffers
    void clear() {
        for (uint ch = 0; ch < 2; ch++) {
            for (auto& filter : earlyCombs[ch])
                filter.clear();

            for (auto& filter : lateCombs[ch])
                filter.clear();
        }
    }

    // Sets the reverb's parameters to the argument object, and updates them appropriately
    void setParameters(const Parameters& newParameters) {
        // store current values
        const float oldMix = parameters.mix, oldSpread = parameters.spread,
                    oldDamp = parameters.damping, oldSize = parameters.size;

        // update parameter object
        parameters = newParameters;

        // update mix values, if necessary
        if (parameters.mix != oldMix)
            setMixValues();

        // set gain values
        preGain = 0.1f / float(parameters.numEarlyCombs + parameters.numLateCombs);
        drySmooth.setTargetValue(dry);
        wet1.setTargetValue(wetGainScale * wet * (1 + parameters.width));
        wet2.setTargetValue(wetGainScale * wet * (1 - parameters.width));

        // update comb frequencies, if necessary
        if (parameters.spread != oldSpread)
            setCombs();

        // set damping/feedback values
        if (parameters.damping != oldDamp || parameters.size != oldSize)
            setDamping();
    }

    // Set a particular early comb's frequency
    void setEarlyCombTime(const float& newDelayTime, const uint& combIndex) {
        earlyCombTimes[pa::math::clamp<uint>(combIndex, 0, 7)] = newDelayTime;

        setCombs();
    }

    // Set a particular late comb's frequency
    void setLateCombTime(const float& newDelayTime, const uint& combIndex) {
        lateCombTimes[pa::math::clamp<uint>(combIndex, 0, 3)] = newDelayTime;

        setCombs();
    }

    // Process a both stereo samples
    void process(float* left, float* right) {
        // guard-check for nullptr
        if (left == nullptr || right == nullptr) return;

        // create variables
        const float input = (*left + *right) * preGain;
        const float damp = dampingSmooth.getNextValue(),
                    feed = feedbackSmooth.getNextValue();
        float outL = 0.0f,
              outR = 0.0f;

        // accumulate damping combs in parallel
        for (uint j = 0; j < parameters.numEarlyCombs; j++) {
            outL += earlyCombs[0][j].processEarly(&input, damp, feed);
            outR += earlyCombs[1][j].processEarly(&input, damp, feed);
        }
        // send to non-damping combs in series
        for (uint j = 0; j < parameters.numLateCombs; j++) {
            outL = lateCombs[0][j].processLate(&outL);
            outR = lateCombs[1][j].processLate(&outR);
        }

        // set gain values and send to output
        const float d = drySmooth.getNextValue();
        const float w1 = wet1.getNextValue(),
                    w2 = wet2.getNextValue();

        *left  = d * (*left)  + w1 * outL + w2 * outR;
        *right = d * (*right) + w1 * outR + w2 * outL;
    }

 private:
    uint sampleRate = 44100;
    float preGain = 0.0f, wet = 0.0f, dry = 0.0f;
    static constexpr float wetGainScale = 1.2f;
    juce::SmoothedValue<float> dampingSmooth, feedbackSmooth, wet1, wet2, drySmooth;
    Parameters parameters;

    void setCombs() {
        auto spreadAmount = pa::math::clamp<float>(parameters.spread, 0.0f, 0.01f) / 2;

        for (uint ch = 0; ch < 2; ch++) {
            float spread = (ch == 0) ? spreadAmount : -spreadAmount;

            for (uint i = 0; i < 8; i++)
                earlyCombs[ch][i].SetTime(earlyCombTimes[i] + spread);

            for (uint i = 0; i < 4; i++)
                lateCombs[ch][i].SetTime(lateCombTimes[i] + spread);
        }
    }

    void prepareCombs() {
        for (uint ch = 0; ch < 2; ch++) {
            for (auto& cmb : earlyCombs[ch])
                cmb.prepare(sampleRate);

            for (auto& cmb : lateCombs[ch])
                cmb.prepare(sampleRate);
        }
    }

    void setMixValues() {
        auto mixAmount = pa::math::clamp<float>(parameters.mix, 0.0f, 1.0f);
        dry = 1.0f - mixAmount;
        wet = pa::math::expRounder(mixAmount, 0.8f) * 1.55f;
    }

    void setDamping() {
        constexpr float fbScale = 0.78f, fbOffset = 0.2f,
                        dampScale = 0.9f;

        dampingSmooth.setTargetValue(parameters.damping * dampScale);
        feedbackSmooth.setTargetValue(parameters.size * fbScale + fbOffset);
    }

    class Comb {
     public:
        Comb() { prepare(44100); }

        // prepare the filter's buffer and smoothed values
        void prepare(const uint& newSampleRate) {
            buffer.prepare(0.1f, newSampleRate);
            clear();
        }

        // clear the filter's buffer
        void clear() {
            previousValue = 0.0f;
            buffer.clear();
        }

        // set the delay time of the buffer (filter frequency)
        void SetTime(const float& delayTimeInSeconds) {
            buffer.setDelayTime(pa::math::clamp(delayTimeInSeconds, 0.001f, 1.0f));
        }

        // process early (damped) reflections
        float processEarly(const float* const input, const float& damp, const float& feed) {
            // get delayed signal
            float delayLine = buffer.getFromBuffer();
            // apply low pass (weighted average), used for hf damping:
            previousValue = delayLine + damp * (previousValue - delayLine);

            // multiply by feed, sum with input
            float temp = *input + previousValue * feed;
            // add to buffer
            buffer.pushToBuffer(&temp);

            // send to output
            return delayLine;
        }

        // process late reflections
        float processLate(const float* const input) {
            // get delayed signal
            float delayLine = buffer.getFromBuffer();

            // multiply by fixed feedback, sum with input
            float temp = *input + delayLine * 0.5f;
            // add to buffer
            buffer.pushToBuffer(&temp);

            // send to output, input polarity inverted
            return delayLine - *input;
            // equivalent to: return -(*input) + delayLine;
        }

     private:
        pa::dsp::RingBuffer<float> buffer;
        float previousValue = 0.0f;
    };

    // combs[channel][instance]
    vector<vector<Comb>> earlyCombs, lateCombs;
    vector<float> earlyCombTimes, lateCombTimes;
};

} // end namespace pa::dsp
