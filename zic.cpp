#include <string.h>

#include "libDaisy/src/dev/oled_ssd130x.h"
#include "daisy_seed.h"

#include "fonts/drawText.h"

#include "MultiFx.h"
#include "BandIsolatorFx.h"

using namespace daisy;

DaisySeed hw;
Encoder encoder;

MultiFx multiFx;
BandIsolatorFx bandFx;

SSD130xI2c64x32Driver display;
SSD130xI2c64x32Driver::Config displayCfg;

constexpr Pin KNOB_1 = seed::A8;
constexpr Pin KNOB_2 = seed::A11;
constexpr Pin KNOB_3 = seed::A10;
constexpr Pin KNOB_4 = seed::A4;
constexpr Pin KNOB_5 = seed::A5;
constexpr Pin KNOB_6 = seed::A6;
constexpr Pin KNOB_7 = seed::A3;
constexpr Pin KNOB_8 = seed::A2;
constexpr Pin KNOB_9 = seed::A0;
constexpr Pin KNOB_10 = seed::A1;

enum Knob
{
    BAND_MIX,
    BAND_FREQ,
    BAND_RANGE,
    BAND_FX,
    BAND_CUTOFF,
    BAND_RESONANCE,
    MASTER_FX1,
    MASTER_FX2,
    MASTER_FX3,
    MASTER_FX4,
    NUM_KNOBS
};

std::string pctStrValue(float f_value, uint16_t i_value)
{
    return std::to_string(i_value);
}

std::string freqStrValue(float f_value, uint16_t i_value)
{
    return std::to_string(static_cast<uint>(bandFx.centerFreq));
}

std::string rangeStrValue(float f_value, uint16_t i_value)
{
    return std::to_string(static_cast<uint>(bandFx.rangeHz));
}

std::string filterStrValue(float f_value, uint16_t i_value)
{
    int val = f_value * 200.0f - 100.0f;
    return val > 0
               ? "HPF " + std::to_string(val)
               : "LPF " + std::to_string(-val);
}

typedef std::string (*StrPtr)(float, uint16_t);
StrPtr strFn = &pctStrValue;

void actionNone(float f_value, uint16_t i_value) {}
void actionBandMix(float f_value, uint16_t i_value) { bandFx.setMix(f_value); }
void actionBandFreq(float f_value, uint16_t i_value) { bandFx.setCenterFreq(f_value); }
void actionBandRange(float f_value, uint16_t i_value) { bandFx.setRange(f_value); }
void actionBandFx(float f_value, uint16_t i_value) { bandFx.setFxAmount(f_value); }
void actionFilterCutoff(float f_value, uint16_t i_value)
{
    float amount = f_value * 2 - 1.0f;
    bandFx.filter.setCutoff(amount);
}
void actionFilterReso(float f_value, uint16_t i_value) { bandFx.filter.setResonance(f_value); }

typedef void (*ActionPtr)(float, uint16_t);
ActionPtr actionFn = &actionNone;

AdcChannelConfig knobCfgs[NUM_KNOBS];
uint16_t knobValues[NUM_KNOBS];
uint16_t knobValuesPrev[NUM_KNOBS];
float knobValuesFloat[NUM_KNOBS];
std::string knobNames[NUM_KNOBS] = {"Band Mix", "Band Freq", "Band Range", "Band Fx", "Filter Cutoff", "Resonance", "Master Fx 1", "Master Fx 2", "Master Fx 3", "Master Fx 4"};
std::string knobUnits[NUM_KNOBS] = {"%", "Hz", "Hz", "%", "%", "%", "%", "%", "%", "%"};
StrPtr knobGetString[NUM_KNOBS] = {&pctStrValue, &freqStrValue, &rangeStrValue, &pctStrValue, &filterStrValue, &pctStrValue, &pctStrValue, &pctStrValue, &pctStrValue, &pctStrValue};
ActionPtr knobActions[NUM_KNOBS] = {&actionBandMix, &actionBandFreq, &actionBandRange, &actionBandFx, &actionFilterCutoff, &actionFilterReso, &actionNone, &actionNone, &actionNone, &actionNone};

// Encoder
constexpr Pin ENC_A_PIN = seed::D8;
constexpr Pin ENC_B_PIN = seed::D10;
constexpr Pin ENC_CLICK_PIN = seed::D9;

uint8_t isFx = 0;

static void Callback(AudioHandle::InterleavingInputBuffer in,
                     AudioHandle::InterleavingOutputBuffer out,
                     size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        float output = in[i];
        output = bandFx.sample(output);
        output = multiFx.apply(output, knobValuesFloat[MASTER_FX1]);
        out[i] = clamp(output, -1.0f, 1.0f);
    }
}

void renderFx()
{
    display.Fill(false);
    if (isFx == 0)
    {
        text(display, 0, 0, "Band Fx", PoppinsLight_8);
        text(display, 0, 20, bandFx.multiFx.typeName, PoppinsLight_8);
    }
    // else if (isFx == 1)
    // {
    //     text(display, 0, 0, "Filter Fx", PoppinsLight_8);
    //     text(display, 0, 20, filteredFx.multiFx.typeName, PoppinsLight_8);
    // }
    else
    {
        text(display, 0, 0, "Master Fx", PoppinsLight_8);
        text(display, 0, 20, multiFx.typeName, PoppinsLight_8);
    }
    display.Update();
}

void renderKnob(std::string knobName, float f_value, uint16_t i_value, Knob knob)
{
    display.Fill(false);
    text(display, 0, 0, knobName, PoppinsLight_8);
    // int x = text(display, 0, 16, std::to_string(value), PoppinsLight_12);
    int x = text(display, 0, 16, knobGetString[knob](f_value, i_value), PoppinsLight_12);
    text(display, x + 2, 20, knobUnits[knob], PoppinsLight_8);
    display.Update();
}

float getKnobValue(Knob knob) { return clamp(hw.adc.GetFloat(knob), 0.0f, 0.96f) / 0.96f; }
uint16_t f2i(float f) { return static_cast<uint16_t>(static_cast<uint>(f * 100000) * 0.001); }

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4);
    hw.StartAudio(Callback);

    multiFx.init(hw.AudioSampleRate(), 0, MultiFx::FXType::BASS_BOOST);
    bandFx.init(hw.AudioSampleRate(), 1, MultiFx::FXType::COMPRESSION);

    encoder.Init(ENC_A_PIN, ENC_B_PIN, ENC_CLICK_PIN);
    displayCfg.transport_config.i2c_config.pin_config.sda = seed::D12;
    displayCfg.transport_config.i2c_config.pin_config.scl = seed::D11;
    display.Init(displayCfg);

    knobCfgs[BAND_MIX].InitSingle(KNOB_1);
    knobCfgs[BAND_FREQ].InitSingle(KNOB_2);
    knobCfgs[BAND_RESONANCE].InitSingle(KNOB_3);

    knobCfgs[BAND_FX].InitSingle(KNOB_4);
    knobCfgs[BAND_RANGE].InitSingle(KNOB_5);
    knobCfgs[BAND_CUTOFF].InitSingle(KNOB_6);

    knobCfgs[MASTER_FX1].InitSingle(KNOB_7);
    knobCfgs[MASTER_FX2].InitSingle(KNOB_8);
    knobCfgs[MASTER_FX3].InitSingle(KNOB_9);
    knobCfgs[MASTER_FX4].InitSingle(KNOB_10);

    hw.adc.Init(knobCfgs, 10);
    hw.adc.Start();

    for (int i = 0; i < NUM_KNOBS; i++)
    {
        float f_value = getKnobValue((Knob)i);
        uint16_t i_value = f2i(f_value);
        knobValues[i] = i_value;
        knobValuesFloat[i] = f_value;
        knobActions[i](f_value, i_value);
    }

    renderFx();
    while (1)
    {
        encoder.Debounce();
        int32_t inc = encoder.Increment();
        if (inc)
        {
            if (isFx == 0)
                bandFx.multiFx.setIncType(inc);
            // else if (isFx == 1)
            //     filteredFx.multiFx.setIncType(inc);
            else
                multiFx.setIncType(inc);

            renderFx();
        }
        if (encoder.RisingEdge())
        {
            isFx = (isFx + 1) % 3;
            renderFx();
        }

        for (int i = 0; i < NUM_KNOBS; i++)
        {
            float f_value = getKnobValue((Knob)i);
            uint16_t i_value = f2i(f_value);
            if (i_value != knobValues[i])
            {
                if (i_value != knobValuesPrev[i]) // To avoid ping pong of value
                {
                    knobValuesPrev[i] = knobValues[i];
                    knobValues[i] = i_value;
                    knobValuesFloat[i] = f_value;
                    knobActions[i](f_value, i_value);
                    renderKnob(knobNames[i], f_value, i_value, (Knob)i);
                }
            }
        }

        System::Delay(1);
    }
}
