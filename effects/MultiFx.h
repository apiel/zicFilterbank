#pragma once

#include "applyReverb.h"
#include "../range.h"

#include <algorithm>
#include <string>

class MultiFx
{
protected:
    float sampleRate = 44100.0f;

    typedef float (MultiFx::*FnPtr)(float, float);
    FnPtr fxFn = &MultiFx::fxOff;

    float fxOff(float input, float) { return input; }

    // TODO switch to 64MB ram...

    // static constexpr int REVERB_BUFFER_SIZE = 48000;                 // 1 second buffer at 48kHz
    // static constexpr int DELAY_BUFFER_SIZE = REVERB_BUFFER_SIZE * 3; // 3 second

    static constexpr int REVERB_BUFFER_SIZE = 20000;
    static constexpr int DELAY_BUFFER_SIZE = REVERB_BUFFER_SIZE * 3;

    float buffer[DELAY_BUFFER_SIZE] = {0.0f};
    int bufferIndex = 0;
    float fxReverb(float signal, float amount)
    {
        float reverbAmount = amount;
        return applyReverb(signal, reverbAmount, buffer, bufferIndex, REVERB_BUFFER_SIZE);
    }

    float fxShimmerReverb(float input, float amount)
    {
        return applyShimmerReverb(input, amount, buffer, bufferIndex, REVERB_BUFFER_SIZE);
    }

    int shimmerTime = 0;
    float fxShimmer2Reverb(float input, float amount)
    {
        return applyShimmer2Reverb(input, amount, buffer, bufferIndex, REVERB_BUFFER_SIZE, shimmerTime);
    }

    float fxReverb2(float signal, float amount)
    {
        return applyReverb2(signal, amount, buffer, bufferIndex, REVERB_BUFFER_SIZE);
    }

    float fxReverb3(float signal, float amount)
    {
        return applyReverb3(signal, amount, buffer, bufferIndex, REVERB_BUFFER_SIZE);
    }

    float fxDelay(float input, float amount)
    {
        return applyDelay(input, amount, buffer, bufferIndex, DELAY_BUFFER_SIZE);
    }

    float fxDelay2(float input, float amount)
    {
        return applyDelay2(input, amount, buffer, bufferIndex, DELAY_BUFFER_SIZE);
    }

    float fxDelay3(float input, float amount)
    {
        return applyDelay3(input, amount, buffer, bufferIndex, DELAY_BUFFER_SIZE);
    }

    float prevInput = 0;
    float prevOutput = 0;
    float fxBoost(float input, float amount)
    {
        if (amount == 0.0f)
        {
            return input;
        }
        float bassFreq = 0.2f + 0.8f * amount;
        float bassBoosted = (1.0f - bassFreq) * prevOutput + bassFreq * (input + prevInput) * 0.5f;
        prevInput = input;
        prevOutput = bassBoosted;
        bassBoosted *= 1.0f + amount * 2.0f;

        return bassBoosted;
    }

    float fxDrive(float input, float amount)
    {
        if (amount == 0.0f)
        {
            return input;
        }
        return tanh(input * (1.0f + amount * 5.0f));
    }

    float fxCompressor(float input, float amount)
    {
        if (amount == 0.0f)
        {
            return input;
        }
        // How about?
        // return (input * (1 - amount)) + (range(std::pow(input, amount * 0.8f), -1.0f, 1.0f) * fxAmount.pct());
        if (input > 0.0f)
        {
            return std::pow(input, 1.0f - amount * 0.8f);
        }
        return -std::pow(-input, 1.0f - amount * 0.8f);
    }

    float fxWaveshaper(float input, float amount)
    {
        float sineValue = sinf(input);
        return input + amount * sineValue * 2;
    }

    float fxClipping(float input, float amount)
    {
        if (amount == 0.0f)
        {
            return input;
        }
        float scaledClipping = amount * amount * 20;
        return range(input + input * scaledClipping, -1.0f, 1.0f);
    }

    float sampleSqueeze;
    int samplePosition = 0;
    float fxSampleReducer(float input, float amount)
    {
        if (amount == 0.0f)
        {
            return input;
        }
        if (samplePosition < amount * 100 * 2)
        {
            samplePosition++;
        }
        else
        {
            samplePosition = 0;
            sampleSqueeze = input;
        }

        return sampleSqueeze;
    }

    float sampleHold = 0.0f;
    int sampleCounter = 0;
    float fxBitcrusher(float input, float amount)
    {
        if (amount == 0.0f)
        {
            return input;
        }

        // Reduce Bit Depth
        int bitDepth = 2 + amount * 10;             // Stronger effect
        float step = 1.0f / (1 << bitDepth);        // Quantization step
        float crushed = round(input / step) * step; // Apply bit reduction

        // Reduce Sample Rate
        int sampleRateDivider = 1 + amount * 20; // Reduces update rate
        if (sampleCounter % sampleRateDivider == 0)
        {
            sampleHold = crushed; // Hold the value for "stepping" effect
        }
        sampleCounter++;

        if (amount < 0.1f)
        {
            // mix with original signal
            return sampleHold * (amount * 10) + input * (1.0f - (amount * 10));
        }

        return sampleHold;
    }

    float fxInverter(float input, float amount)
    {
        if (input > amount || input < -amount)
        {
            return -input;
        }
        return input;
    }

    float tremoloPhase = 0.0f;
    float fxTremolo(float input, float amount)
    {
        if (amount == 0.0f)
        {
            return input;
        }

        float speed = 1.0f; // Tremolo speed in Hz
        tremoloPhase += 0.05f * speed;
        float mod = (sin(tremoloPhase) + 1.0f) / 2.0f; // Modulation between 0-1

        return input * (1.0f - amount + amount * mod);
    }

    float ringPhase = 0.0f; // Phase for the sine wave oscillator
    float fxRingMod(float input, float amount)
    {
        if (amount == 0.0f)
        {
            return input;
        }

        float ringFreq = 200.0f + amount * 800.0f; // Modulation frequency (200Hz - 1000Hz)
        ringPhase += 2.0f * M_PI * ringFreq / sampleRate;

        // Keep phase in the [0, 2π] range
        if (ringPhase > 2.0f * M_PI)
        {
            ringPhase -= 2.0f * M_PI;
        }

        float modulator = sin(ringPhase);    // Sine wave oscillator
        float modulated = input * modulator; // Apply ring modulation

        return (1.0f - amount) * input + amount * modulated;
    }

    float fxFeedback(float input, float amount)
    {
        if (amount == 0.0f)
        {
            return input;
        }

        float decay = 0.98f + 0.01f * (1.0f - amount); // decay rate based on amount
        float feedbackSample = buffer[bufferIndex];    // read from buffer

        // Simple one-pole lowpass to emphasize bass
        static float lowpassState = 0.0f;
        float cutoff = 80.0f + 100.0f * amount; // Low-pass around 80-180 Hz
        float alpha = cutoff / sampleRate;
        lowpassState += alpha * (feedbackSample - lowpassState);

        // Mix input with feedback and write to buffer
        float out = input + lowpassState * amount;
        buffer[bufferIndex] = out * decay;

        // Increment circular buffer index
        bufferIndex = (bufferIndex + 1) % REVERB_BUFFER_SIZE;

        return tanh(out); // Add soft saturation
    }

    float revGateEnv = 0.0f;
    float fxReverseGate(float input, float amount)
    {
        revGateEnv += 0.01f;
        if (revGateEnv > 1.0f)
            revGateEnv = 0.0f; // reset every short cycle

        float env = powf(revGateEnv, 3.0f - amount * 2.0f); // fade-in shape
        return input * env;
    }

    float decimHold = 0.0f;
    int decimCounter = 0;
    float fxDecimator(float input, float amount)
    {
        int interval = 1 + (int)(amount * 30.0f); // Downsample ratio
        if (decimCounter % interval == 0)
        {
            decimHold = input;
        }
        decimCounter++;
        return decimHold;
    }

    //////////
    // Filters
    //--------

    float lp_z1 = 0.0f; // 1-pole lowpass state
    float hp_z1 = 0.0f; // 1-pole highpass state
    float fxHighPassFilterDistorted(float input, float amount)
    {
        if (amount <= 0.0f)
            return input;

        amount = 1.0f - powf(amount, 0.25f);

        float cutoff = 200.0f + (sampleRate * 0.48f - 200.0f) * amount;
        float alpha = sampleRate / (cutoff + sampleRate);
        hp_z1 = alpha * (hp_z1 + input - (lp_z1 = lp_z1 + (cutoff / (cutoff + sampleRate)) * (input - lp_z1)));
        return hp_z1;
    }

    // Filter 2
    float filterBuf = 0.0;
    float lp = 0.0;
    float hp = 0.0;
    float bp = 0.0;
    void setSampleData(float inputValue, float cutoff, float &_buf, float &_hp, float &_bp, float &_lp)
    {
        _hp = inputValue - _buf;
        _bp = _buf - _lp;
        _buf = _buf + cutoff * _hp;
        _lp = _lp + cutoff * (_buf - _lp);
    }

    float fxLowPass(float input, float amount)
    {
        if (amount <= 0.0f)
            return input;

        amount = (1.0f - amount * 0.95f) + 0.05f;

        setSampleData(input, amount, filterBuf, hp, bp, lp);
        return lp;
    }

    float fxHighPass(float input, float amount)
    {
        if (amount <= 0.0f)
            return input;

        amount = amount * 0.95f;

        setSampleData(input, amount, filterBuf, hp, bp, lp);
        return hp;
    }

public:
    enum FXType
    {
        FX_OFF,
        REVERB,
        REVERB2,
        REVERB3,
        DELAY,
        DELAY2,
        DELAY3,
        BASS_BOOST,
        DRIVE,
        COMPRESSION,
        WAVESHAPER,
        CLIPPING,
        SAMPLE_REDUCER,
        BITCRUSHER,
        INVERTER,
        TREMOLO,
        RING_MOD,
        FX_SHIMMER_REVERB,
        FX_SHIMMER2_REVERB,
        FX_FEEDBACK,
        DECIMATOR,
        LPF,
        HPF,
        HPF_DIST,
        FX_COUNT
    };

    std::string typeName = "OFF";
    FXType fxType = FXType::FX_OFF;

    void setIncType(int8_t direction)
    {
        int8_t newType = fxType + direction;
        if (newType < 0)
            newType = FX_COUNT - 1;
        else if (newType >= FX_COUNT)
            newType = 0;
        setFxType((FXType)newType);
    }

    void setFxType(MultiFx::FXType type)
    {
        if (type < 0 || type >= FX_COUNT)
            return;

        fxType = type;
        if (type == MultiFx::FXType::FX_OFF)
        {
            typeName = "OFF";
            fxFn = &MultiFx::fxOff;
        }
        else if (type == MultiFx::FXType::REVERB)
        {
            typeName = "Reverb";
            fxFn = &MultiFx::fxReverb;
        }
        else if (type == MultiFx::FXType::REVERB2)
        {
            typeName = "Reverb2";
            fxFn = &MultiFx::fxReverb2;
        }
        else if (type == MultiFx::FXType::REVERB3)
        {
            typeName = "Reverb3";
            fxFn = &MultiFx::fxReverb3;
        }
        else if (type == MultiFx::FXType::DELAY)
        {
            typeName = "Delay";
            fxFn = &MultiFx::fxDelay;
        }
        else if (type == MultiFx::FXType::DELAY2)
        {
            typeName = "Delay2";
            fxFn = &MultiFx::fxDelay2;
        }
        else if (type == MultiFx::FXType::DELAY3)
        {
            typeName = "Delay3";
            fxFn = &MultiFx::fxDelay3;
        }
        else if (type == MultiFx::FXType::BASS_BOOST)
        {
            typeName = "Bass boost";
            fxFn = &MultiFx::fxBoost;
        }
        else if (type == MultiFx::FXType::DRIVE)
        {
            typeName = "Drive";
            fxFn = &MultiFx::fxDrive;
        }
        else if (type == MultiFx::FXType::COMPRESSION)
        {
            typeName = "Compressor";
            fxFn = &MultiFx::fxCompressor;
        }
        else if (type == MultiFx::FXType::WAVESHAPER)
        {
            typeName = "Waveshap.";
            fxFn = &MultiFx::fxWaveshaper;
        }
        else if (type == MultiFx::FXType::CLIPPING)
        {
            typeName = "Clipping";
            fxFn = &MultiFx::fxClipping;
        }
        else if (type == MultiFx::FXType::SAMPLE_REDUCER)
        {
            typeName = "Sample red.";
            fxFn = &MultiFx::fxSampleReducer;
        }
        else if (type == MultiFx::FXType::BITCRUSHER)
        {
            typeName = "Bitcrusher";
            fxFn = &MultiFx::fxBitcrusher;
        }
        else if (type == MultiFx::FXType::INVERTER)
        {
            typeName = "Inverter";
            fxFn = &MultiFx::fxInverter;
        }
        else if (type == MultiFx::FXType::TREMOLO)
        {
            typeName = "Tremolo";
            fxFn = &MultiFx::fxTremolo;
        }
        else if (type == MultiFx::FXType::RING_MOD)
        {
            typeName = "Ring mod.";
            fxFn = &MultiFx::fxRingMod;
        }
        else if (type == MultiFx::FXType::FX_SHIMMER_REVERB)
        {
            typeName = "Shimmer";
            fxFn = &MultiFx::fxShimmerReverb;
        }
        else if (type == MultiFx::FXType::FX_SHIMMER2_REVERB)
        {
            typeName = "Shimmer2";
            fxFn = &MultiFx::fxShimmer2Reverb;
        }
        else if (type == MultiFx::FXType::FX_FEEDBACK)
        {
            typeName = "Feedback";
            fxFn = &MultiFx::fxFeedback;
        }
        else if (type == MultiFx::FXType::DECIMATOR)
        {
            typeName = "Decimator";
            fxFn = &MultiFx::fxDecimator;
        }
        else if (type == MultiFx::FXType::LPF)
        {
            typeName = "LPF";
            fxFn = &MultiFx::fxLowPass;
        }
        else if (type == MultiFx::FXType::HPF)
        {
            typeName = "HPF";
            fxFn = &MultiFx::fxHighPass;
        }
        else if (type == MultiFx::FXType::HPF_DIST)
        {
            typeName = "HPF dist.";
            fxFn = &MultiFx::fxHighPassFilterDistorted;
        }
        // TODO: add fx sample reducer
    }

    MultiFx(float sampleRate)
        : sampleRate(sampleRate)
    {
    }

    MultiFx()
    {
    }

    void init(float sampleRate, MultiFx::FXType type = MultiFx::FXType::FX_OFF)
    {
        this->sampleRate = sampleRate;
        setFxType(type);
    }

    float apply(float in, float amount)
    {
        return (this->*fxFn)(in, amount);
    }
};
