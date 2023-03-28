/*
 * ~ Pragma Audio namespace ~
 * General purpose "helper" functions and some handy DSP stuff all chucked in one file.
 * Includes some maths stuff like transfer/interpolation functions, as
 * well as some trigonometric approximations.
 *
 * The built-in std/JUCE stuff is certainly better in most cases. By no
 * means are these meant to be more optimised or robust... but it's fun to
 * make them yourself :)
 *
 * The processors for this device are included in separate files:
 * CombFilter.h
 * Filter.h
 * Reverb.h
*/

#pragma once
#include <vector>
#include <array>
using namespace juce;
using std::array, std::vector;
using uint = unsigned int;

namespace pa {
                // Maths stuff
namespace math {
                // Value mapping
// Returns the clamped value
template <typename NumType>
static inline NumType clamp(const NumType& val, const NumType& min, const NumType& max) {
    return (val < min) ? min : (val > max) ? max : val;
}

// Clamps the input via passing by pointer
template <typename NumType>
static inline void setClamp(NumType* val, const NumType& min, const NumType& max) {
    *val = (*val < min) ? min : (*val > max) ? max : *val;
}

// Returns a mapped value based on input and output ranges
template <typename NumType>
static inline NumType map(const NumType& val, const NumType& inMin, const NumType& inMax,
                          const NumType& outMin = 0, const NumType& outMax = 1) {
    return ((val - inMin) / (inMax - inMin)) * (outMax - outMin) + outMin;
}

// Returns the decimal part of a value
template <typename FloatType>
static inline FloatType mod1(const FloatType& input) {
    // NOTE does this work for negative numbers?
    return input - FloatType(int(input));
}

                // Trig approximations
// Wrap input within -pi - pi, used by the approximations here
template <typename FloatType>
static inline void WrapPi(FloatType& input, bool useHalfPi = false) noexcept {
    static constexpr auto pi = static_cast<FloatType>(M_PI);

    if (useHalfPi)
        input = static_cast<FloatType>(std::fmod(input - pi * 0.5, pi) - pi * 0.5);
    else
        input = static_cast<FloatType>(std::fmod(input - pi, 2 * pi) - pi);
}

// Fast sin() approximation
template <typename FloatType>
[[maybe_unused]] static FloatType FastSin(FloatType x) noexcept {
    WrapPi<FloatType>(x);
    const double x2 = x * x;
    const double num = -x * (-11511339840 + x2 * (1640635920 + x2 * (-52785432 + x2 * 479249)));
    const double den = 11511339840 + x2 * (277920720 + x2 * (3177720 + x2 * 18361));
    return static_cast<FloatType>(num / den);
}

// Fast cos() approximation
template <typename FloatType>
[[maybe_unused]] static FloatType FastCos(FloatType x) noexcept {
    WrapPi<FloatType>(x);
    const double x2 = x * x;
    const double num = -(-39251520 + x2 * (18471600 + x2 * (-1075032 + 14615 * x2)));
    const double den = 39251520 + x2 * (1154160 + x2 * (16632 + x2 * 127));
    return static_cast<FloatType>(num / den);
}

// Fast tan() approximation
template <typename FloatType>
static FloatType FastTan(FloatType x) noexcept {
    WrapPi<FloatType>(x, true);
    const double x2 = x * x;
    const double num = x * (-135135 + x2 * (17325 + x2 * (-378 + x2)));
    const double den = -135135 + x2 * (62370 + x2 * (-3150 + 28 * x2));
    return static_cast<FloatType>(num / den);
}

                // Interpolation
// Linear interpolation (clamped: use 0 <= t <= 1)
template <typename FloatType>
inline static FloatType LinearInterp(const FloatType& a, const FloatType& b, FloatType t) {
    if (t <= 0) return a;
    if (t >= 1) return b;

    return a + t * (b - a);
}

// Shorthand for the above
template <typename FloatType>
inline static FloatType Lerp(const FloatType& a, const FloatType& b, FloatType t) {
    return LinearInterp(a, b, t);
}

// Cubic interpolation (useSmoother = true uses catmull-rom method)
template <typename FloatType>
static FloatType CubicInterp(const FloatType& a, const FloatType& b, const FloatType& c,
                             const FloatType& d, FloatType t, const bool& useCatmullRom = false) {
    if (t <= 0) return b;
    if (t >= 1) return c;

    FloatType t2 = t * t, a0, a1, a2;

    if (useCatmullRom) { // catmull-rom method
        a0 = static_cast<FloatType>(-0.5 * a + 1.5 * b - 1.5 * c + 0.5 * d);
        a1 = static_cast<FloatType>(a - 2.5 * b + 2 * c - 0.5 * d);
        a2 = static_cast<FloatType>(-0.5 * a + 0.5 * c);
    }
    else { // standard method
        a0 = d - c - a + b;
        a1 = a - b - a0;
        a2 = c - a;
    }

    return a0 * t * t2 + a1 * t2 + a2 * t + b;
}

// A rounded exponential transfer function
// Higher c values mean greater values toward zero (s-curve), and vice versa
template <typename FloatType>
static FloatType ExpRounder(const FloatType &input, const FloatType& curveValue) {
    FloatType x = clamp<FloatType>(input, -1.0, 1.0),
              c = clamp<FloatType>(curveValue, -1.0, 1.0);

    // map c to equal range
    if (c >= 0)
        c = map<FloatType>(c, 0.0, 1.0, 0.0, 20.0);
    else
        c = map<FloatType>(c, -1.0, 0.0, -0.95, 0.0);

    // positive
    if (0 < x && x <= 1)
        return (x * (1 + c)) / (c * x + 1);
    // negative
    else if (-1 <= x && x <= 0)
        return (-x * (1 + c)) / (c * x - 1);

    return 0;
}

} // end namespace math

                // DSP
namespace dsp {
// Different interpolation enums for general use
enum InterpolationType {
    noInterp,
    linearInterp,
    cosineInterp,
    cubicInterp,
    hermiteInterp
};

// A block of memory on the heap, useful for buffers
// Mostly a piece-for-piece copy of the JUCE version minus some
// stuff so I could see how it worked
template <typename ElementType>
class HeapBlock {
 public:
    HeapBlock() = default;

    template <typename SizeType>
    HeapBlock(const SizeType& numElements, const bool& initialise) {
        this->allocate(numElements, initialise);
    }

    HeapBlock(HeapBlock&& copy) noexcept : data(copy.data), size(copy.size) {
        copy.data = nullptr;
    }

    ~HeapBlock() { std::free(data); }

    HeapBlock& operator= (HeapBlock&& copy) noexcept {
        std::swap(data, copy.data);
        size = copy.size;
        return *this;
    }

    // return the data pointer
    [[nodiscard]] inline ElementType* get() const noexcept {
        return data;
    }

    inline ElementType* operator-> () const noexcept {
        return data;
    }

    // return the block size
    inline uint getSize() const noexcept {
        return size;
    }

    // return an element at an index
    template <typename IndexType>
    inline ElementType& operator[] (IndexType index) const noexcept {
        if (index >= size) return data[size];

        return data[index];
    }

    // allocate new memory
    template <typename SizeType>
    void allocate(const SizeType& newNumElements, const bool& initialise = true) {
        if (data != nullptr)
            std::free(data);

        data = (ElementType*)(initialise ? std::calloc(size_t(newNumElements), sizeof(ElementType))
                                         : std::malloc(size_t(newNumElements) * sizeof(ElementType)));
        size = newNumElements;
    }

    // reallocate memory - retain as much existing data as possible
    template <typename SizeType>
    void reallocate(const SizeType& newNumElements) {
        data = (ElementType*)(data == nullptr ? std::malloc(size_t(newNumElements) * sizeof(ElementType))
                                              : std::realloc(data, size_t(newNumElements) * sizeof(ElementType)));
        size = newNumElements;
    }

    // free the memory
    void free() noexcept {
        std::free(data);
        data = nullptr;
        size = 0;
    }

    // set all elements to 0
    void initialise() {
        for (uint i = 0; i < size; i++)
            data[i] = 0;
    }

 private:
    ElementType* data = nullptr;
    uint size = 0;

};

// Ring (AKA circular) buffer, a classic method for creating delay
// uint is only used to prevent negative values
template <typename FloatType>
class RingBuffer {
 public:
    // Set the buffer's size for memory allocation
    // - resizing at runtime is available, but *NOT* recommended for real-time audio
    void prepare(uint newBufferSizeSamples, const uint& newSampleRate) {
        pa::math::setClamp<uint>(&newBufferSizeSamples, 0, newBufferSizeSamples * newSampleRate * 10);

        if (newBufferSizeSamples != size) {
            pa::math::setClamp<uint>(&newBufferSizeSamples, 0, newSampleRate * 600);
            writeIndex = 0;
            buffer.reallocate(newBufferSizeSamples);
            size = newBufferSizeSamples;
        }

        if (newSampleRate != sampleRate)
            sampleRate = newSampleRate;

        if (sampleRate == 0)
            sampleRate = 1;

        clear();
    }

    void prepare(FloatType newBufferSizeSeconds, const uint& newSampleRate) {
        auto bufferSize = static_cast<uint>(newBufferSizeSeconds * FloatType(newSampleRate));
        // beware of recursion here!
        prepare(bufferSize, newSampleRate);
    }

    // Set all elements to 0
    void clear() {
        buffer.initialise();
    }

    // Set the delay created within the buffer
    void setDelayTime(FloatType newDelayTime, FloatType smoothTime = 0.1f) {
        if (smoothTime != delaySmoothTime) {
            pa::math::setClamp(&smoothTime, 0.0f, smoothTime);
            delaySmoothTime = smoothTime;
            delayTime.reset(sampleRate, delaySmoothTime);
        }

        newDelayTime = pa::math::clamp(abs(newDelayTime), 0.0f, FloatType(size) / FloatType(sampleRate));

        delayTime.setTargetValue(newDelayTime);
    }

    // Simultaneously read from and push to the buffer
    void getAndPush(FloatType* input) {
        if (input == nullptr) return;

        pushToBuffer(input);
        getFromBuffer(input);
    }

    // Get the latest delayed sample from the buffer
    void getFromBuffer(FloatType* sample, const InterpolationType interp = noInterp) {
        if (sample == nullptr) return;

        switch (interp) {
            case noInterp:
                *sample = getFromBufferNoInterp();
                break;
            case linearInterp:
                *sample = getFromBufferLinearInterp();
                break;
            case cubicInterp:
                *sample = getFromBufferCubicInterp();
                break;
            default:
                *sample = getFromBufferNoInterp();
                break;
        }
    }

    // Return the latest delayed sample from the buffer
    FloatType getFromBuffer(const InterpolationType interp = noInterp) {
        switch (interp) {
            case noInterp:
                return getFromBufferNoInterp();
            case linearInterp:
                return getFromBufferLinearInterp();
            case cubicInterp:
                return getFromBufferCubicInterp();
            default:
                return getFromBufferNoInterp();
        }
    }

    // Push a new sample to the buffer (increments write pointer)
    void pushToBuffer(const FloatType* input) {
        if (input == nullptr) return;

        buffer[writeIndex] = *input;
        incrementWritePointer();
    }

    // Force the write pointer to increment, if needed
    void forceIncrementWritePointer() {
        incrementWritePointer();
    }

 private:
    HeapBlock<FloatType> buffer;
    uint size = 0, writeIndex = 0, sampleRate = 44100;
    FloatType delaySmoothTime = 0.0;
    juce::SmoothedValue<FloatType> delayTime = 0.0;

    FloatType getFromBufferNoInterp() {
        const FloatType delay = delayTime.getNextValue();
        uint readOffset = uint(FloatType(sampleRate) * delay);
        uint readIndex = getReadIndex(readOffset);

        return buffer[readIndex];
    }

    FloatType getFromBufferLinearInterp() {
        const FloatType delay = delayTime.getNextValue();
        FloatType readOffset = static_cast<FloatType>(sampleRate) * delay;
        uint readIndex = getReadIndex(readOffset);

        return pa::math::LinearInterp(buffer[readIndex], buffer[readIndex - 1],
                                              pa::math::mod1(readOffset));
    }

    FloatType getFromBufferCubicInterp() {
        const FloatType delay = delayTime.getNextValue();
        FloatType readOffset = FloatType(sampleRate) * delay;
        if (readOffset < 2)
            readOffset = 2;

        uint readIndex = getReadIndex(readOffset);

        FloatType s1 = buffer[readIndex + 1],
                  s2 = buffer[readIndex],
                  s3 = buffer[readIndex - 1],
                  s4 = buffer[readIndex - 2];

        return pa::math::CubicInterp(s1, s2, s3, s4, pa::math::mod1(readOffset), true);
    }

    uint getReadIndex(const uint& readOffset) const {
        uint tmp = writeIndex - readOffset + size;
        if (tmp >= size)
            tmp -= size;

        return tmp;
    }

    void incrementWritePointer() {
        // writeIndex = (writeIndex + 1) % size;
        if (++writeIndex >= size)
            writeIndex -= size;
    }
};

template <typename FloatType, InterpolationType Interp = noInterp>
class SmoothValue {

};

} // end namespace dsp
} // end namespace pa
