#pragma once

#include "MultiFx.h"
#include "MMfilter.h"

class FilteredFx
{
protected:
    float sampleRate = 44100.0f;

    MMfilter filter;

public:
    MultiFx multiFx;
    float feedback = 0.0f;
    float fxAmount = 0.0f;
    float mix = 0.5f;

    // internal amounts
    float scaledClipping = 0.0f;

    FilteredFx()
    {
    }

    void init(float sampleRate, MultiFx::FXType type = MultiFx::FXType::FX_OFF)
    {
        this->sampleRate = sampleRate;
        multiFx.init(sampleRate, type);
    }

    void setCutoff(float amount)
    {
        amount = amount * 2 - 1.0f;
        filter.setCutoff(amount);
    }

    void setResonance(float amount)
    {
        filter.setResonance(amount);
    }

    void setFeedback(float amount)
    {
        feedback = clamp(amount, 0.0f, 1.0f);
    }

    void setFxType(MultiFx::FXType type)
    {
        multiFx.setFxType(type);
    }

    void setFxAmount(float amount)
    {
        fxAmount = clamp(amount, 0.0f, 1.0f);
    }

    void setMix(float amount)
    {
        mix = clamp(amount, 0.0f, 1.0f);
    }

    float sample(float input)
    {
        float filterOut = filter.process(input);
        float feedbackSignal = filterOut * feedback;

        filterOut = multiFx.apply(filterOut, fxAmount);

        float out = input * (1 - mix) + filterOut * mix;
        out += feedbackSignal;

        return out;
    }
};
