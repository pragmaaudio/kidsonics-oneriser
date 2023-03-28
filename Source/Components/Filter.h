#pragma once
#include "pa.h"

// Biquad lowpass and highpass filters (second order, 12 dB/oct roll-off)

namespace pa::dsp {

class Filter {
 public:
    enum FilterType {
        lowpass,
        highpass
    };

    struct Parameters {
        FilterType type = lowpass;
        double cutoff = 500.0, q = M_SQRT1_2;
        bool enabled = true;
    };

    void prepare(const uint& newSampleRate) {
        sampleRate = newSampleRate;
    }

    void setParameters(const Parameters& newParameters) {
        parameters = newParameters;

        if (!parameters.enabled) return;

        setCoefficients();
    }

    float process(const float* input) {
        Parameters& p = parameters; // just used for shorthand

        // avoid nullptr and check if disabled
        if (!p.enabled || input == nullptr) return *input;

        double out = *input * co.a0 + co.dly1;
        co.dly1 = *input * co.a1 + co.dly2 - co.b1 * out;
        co.dly2 = *input * co.a2 - co.b2 * out;
        return static_cast<float>(out);
    }

 private:
    void setCoefficients() {
        Parameters& p = parameters; // just used for shorthand

        // only process when relevant parameters have changed
        if (p.cutoff != co.prevCutoff || sampleRate != co.prevSampleRate) {
            jassert(sampleRate > 0);

            co.k = pa::math::fastTan(M_PI * (p.cutoff / sampleRate));
            co.k2 = co.k * co.k;

            co.prevCutoff = p.cutoff;
            co.prevSampleRate = sampleRate;
        }
        if (p.q != co.prevQ) {
            co.n = 1 / (1 + co.k / p.q + co.k2);

            co.prevQ = p.q;
        }

        // coefficient calculations
        switch (p.type) {
            case lowpass:
                co.a0 = co.k2 * co.n;
                co.a1 = 2 * co.a0;
                co.a2 = co.a0;
                co.b1 = 2 * (co.k2 - 1) * co.n;
                co.b2 = (1 - co.k / p.q + co.k2) * co.n;
                break;
            case highpass:
                co.a0 = co.n;
                co.a1 = -2 * co.a0;
                co.a2 = co.a0;
                co.b1 = 2 * (co.k2 - 1) * co.n;
                co.b2 = (1 - co.k / p.q + co.k2) * co.n;
                break;
        }
    }

    Parameters parameters;
    uint sampleRate = 0;

    // coefficients struct
    struct {
        // filter coefficients
        double a0 = 1.0, a1 = 0.0, a2 = 0.0, b1 = 0.0,
               b2 = 0.0, dly1 = 0.0, dly2 = 0.0;

        // used in calculating coefficients
        double k = 0.0, k2 = 0.0, n = 0.0;

        // used to check for parameter changes
        double prevQ = 0.0, prevCutoff = 0.0, prevSampleRate = 0.0;
    } co;

};

} // end namespace pa::dsp
