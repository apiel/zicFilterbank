#pragma once

#include "MultiFx.h"
#include "MMfilter.h"

class BandIsolatorFx
{
protected:
    float sampleRate = 44100.0f;

    struct BQ
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        float process(float x)
        {
            float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1;
            x1 = x;
            y2 = y1;
            y1 = y;
            return y;
        }
    } lowpass, highpass;

    void updateCoeffs()
    {
        // Compute min/max freq from centerFreq and rangeHz
        float minFreq = std::max(20.0f, centerFreq - rangeHz * 0.5f);
        float maxFreq = std::min(sampleRate * 0.5f, centerFreq + rangeHz * 0.5f);

        setHighpass(highpass, minFreq, sampleRate);
        setLowpass(lowpass, maxFreq, sampleRate);
    }

    void setLowpass(BQ &bq, float freq, float fs)
    {
        float w0 = 2.0f * M_PI * freq / fs;
        float cosw0 = cosf(w0);
        float sinw0 = sinf(w0);
        float alpha = sinw0 / (2.0f * 0.707f); // Q=0.707

        float b0n = (1 - cosw0) / 2;
        float b1n = 1 - cosw0;
        float b2n = (1 - cosw0) / 2;
        float a0n = 1 + alpha;
        float a1n = -2 * cosw0;
        float a2n = 1 - alpha;

        bq.b0 = b0n / a0n;
        bq.b1 = b1n / a0n;
        bq.b2 = b2n / a0n;
        bq.a1 = a1n / a0n;
        bq.a2 = a2n / a0n;
    }

    void setHighpass(BQ &bq, float freq, float fs)
    {
        float w0 = 2.0f * M_PI * freq / fs;
        float cosw0 = cosf(w0);
        float sinw0 = sinf(w0);
        float alpha = sinw0 / (2.0f * 0.707f);

        float b0n = (1 + cosw0) / 2;
        float b1n = -(1 + cosw0);
        float b2n = (1 + cosw0) / 2;
        float a0n = 1 + alpha;
        float a1n = -2 * cosw0;
        float a2n = 1 - alpha;

        bq.b0 = b0n / a0n;
        bq.b1 = b1n / a0n;
        bq.b2 = b2n / a0n;
        bq.a1 = a1n / a0n;
        bq.a2 = a2n / a0n;
    }

public:
    MultiFx multiFx;
    MultiFx multiFx2;
    MultiFx multiFx3;

    MMfilter filter;
    float centerFreq = 1000.0f;
    float rangeHz = 2000.0f;
    float fxAmount = 0.0f;
    float fx2Amount = 0.0f;
    float fx3Amount = 0.0f;
    float mix = 0.5f;

    BandIsolatorFx()
    {
    }

    void init(float sampleRate, uint8_t bufferId1, uint8_t bufferId2, uint8_t bufferId3, MultiFx::FXType type = MultiFx::FXType::FX_OFF)
    {
        this->sampleRate = sampleRate;
        multiFx.init(sampleRate, bufferId1, type);
        multiFx2.init(sampleRate, bufferId2, MultiFx::FXType::FX_OFF);
        multiFx3.init(sampleRate, bufferId3, MultiFx::FXType::FX_OFF);
        updateCoeffs();
    }

    void setCenterFreq(float amount)
    {
        float freq = 20.0f * powf(10000.0f / 20.0f, amount);
        centerFreq = clamp(freq, 20.0f, 20000.0f);
        updateCoeffs();
    }

    void setRange(float amount)
    {
        float freq = 10.0f * powf(5000.0f / 10.0f, amount);
        rangeHz = clamp(freq, 50.0f, 10000.0f);
        updateCoeffs();
    }

    void setFxType(MultiFx::FXType type)
    {
        multiFx.setFxType(type);
    }

    void setFxAmount(float amount)
    {
        fxAmount = clamp(amount, 0.0f, 1.0f);
    }

    void setFx2Type(MultiFx::FXType type)
    {
        multiFx2.setFxType(type);
    }

    void setFx2Amount(float amount)
    {
        fx2Amount = clamp(amount, 0.0f, 1.0f);
    }

    void setFx3Type(MultiFx::FXType type)
    {
        multiFx3.setFxType(type);
    }

    void setFx3Amount(float amount)
    {
        fx3Amount = clamp(amount, 0.0f, 1.0f);
    }

    void setMix(float amount)
    {
        mix = clamp(amount, 0.0f, 1.0f);
    }

    float sample(float input)
    {
        float band = highpass.process(input);
        band = lowpass.process(band);
        band = multiFx.apply(band, fxAmount);
        band = multiFx2.apply(band, fx2Amount);
        band = multiFx3.apply(band, fx3Amount);
        band = filter.process(band);

        return input * (1.0f - mix) + band * mix;
    }
};
