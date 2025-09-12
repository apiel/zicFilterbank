#include <string.h>

#include "libDaisy/src/dev/oled_ssd130x.h"
#include "daisy_seed.h"

#include "fonts/drawText.h"

#include "MultiFx.h"
#include "BandIsolatorFx.h"

using namespace daisy;

struct StorageSettings
{
    MultiFx::FXType bandFx1;
    MultiFx::FXType bandFx2;
    MultiFx::FXType masterFx1;
    MultiFx::FXType masterFx2;
    MultiFx::FXType masterFx3;

    bool operator!=(const StorageSettings &a) const
    {
        return !(a.bandFx1 == bandFx1 && a.bandFx2 == bandFx2 && a.masterFx1 == masterFx1 && a.masterFx2 == masterFx2 && a.masterFx3 == masterFx3);
    }
};

DaisySeed hw;
Encoder encoder;
PersistentStorage<StorageSettings> storage(hw.qspi);

MultiFx multiFx1;
MultiFx multiFx2;
MultiFx multiFx3;
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
    BAND_FX2,
    BAND_CUTOFF,
    BAND_RESONANCE,
    MASTER_FX1,
    MASTER_FX2,
    MASTER_FX3,
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
void actionBandFx2(float f_value, uint16_t i_value) { bandFx.setFx2Amount(f_value); }
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
std::string knobNames[NUM_KNOBS] = {"Band Mix", "Band Freq", "Band Range", "Band Fx 1", "Band Fx 2", "Filter Cutoff", "Resonance", "Master Fx 1", "Master Fx 2", "Master Fx 3"};
std::string knobUnits[NUM_KNOBS] = {"%", "Hz", "Hz", "%", "%", "%", "%", "%", "%", "%"};
StrPtr knobGetString[NUM_KNOBS] = {&pctStrValue, &freqStrValue, &rangeStrValue, &pctStrValue, &pctStrValue, &filterStrValue, &pctStrValue, &pctStrValue, &pctStrValue, &pctStrValue};
ActionPtr knobActions[NUM_KNOBS] = {&actionBandMix, &actionBandFreq, &actionBandRange, &actionBandFx, &actionBandFx2, &actionFilterCutoff, &actionFilterReso, &actionNone, &actionNone, &actionNone};

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
        output = multiFx1.apply(output, knobValuesFloat[MASTER_FX1]);
        output = multiFx2.apply(output, knobValuesFloat[MASTER_FX2]);
        output = multiFx3.apply(output, knobValuesFloat[MASTER_FX3]);
        out[i] = clamp(output, -1.0f, 1.0f);
    }
}

void renderFx()
{
    display.Fill(false);
    if (isFx == 0)
    {
        text(display, 0, 0, "Band Fx 1", PoppinsLight_8);
        text(display, 0, 20, bandFx.multiFx.typeName, PoppinsLight_8);
    }
    else if (isFx == 1)
    {
        text(display, 0, 0, "Band Fx 2", PoppinsLight_8);
        text(display, 0, 20, bandFx.multiFx2.typeName, PoppinsLight_8);
    }
    else if (isFx == 2)
    {
        text(display, 0, 0, "Master Fx 1", PoppinsLight_8);
        text(display, 0, 20, multiFx1.typeName, PoppinsLight_8);
    }
    else if (isFx == 3)
    {
        text(display, 0, 0, "Master Fx 2", PoppinsLight_8);
        text(display, 0, 20, multiFx2.typeName, PoppinsLight_8);
    }
    else
    {
        text(display, 0, 0, "Master Fx 3", PoppinsLight_8);
        text(display, 0, 20, multiFx3.typeName, PoppinsLight_8);
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

    StorageSettings defaultConfig = {MultiFx::FXType::COMPRESSION, MultiFx::FXType::FX_OFF, MultiFx::FXType::BASS_BOOST, MultiFx::FXType::FX_OFF, MultiFx::FXType::FX_OFF};
    storage.Init(defaultConfig);

    bandFx.init(hw.AudioSampleRate(), 0, 1, MultiFx::FXType::COMPRESSION);
    multiFx1.init(hw.AudioSampleRate(), 2, MultiFx::FXType::BASS_BOOST);
    multiFx2.init(hw.AudioSampleRate(), 3, MultiFx::FXType::FX_OFF);
    multiFx3.init(hw.AudioSampleRate(), 4, MultiFx::FXType::FX_OFF);

    StorageSettings &config = storage.GetSettings();
    bandFx.multiFx.setFxType(config.bandFx1);
    bandFx.multiFx2.setFxType(config.bandFx2);
    multiFx1.setFxType(config.masterFx1);
    multiFx2.setFxType(config.masterFx2);
    multiFx3.setFxType(config.masterFx3);

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

    knobCfgs[BAND_FX2].InitSingle(KNOB_7);
    knobCfgs[MASTER_FX1].InitSingle(KNOB_8);
    knobCfgs[MASTER_FX2].InitSingle(KNOB_9);
    knobCfgs[MASTER_FX3].InitSingle(KNOB_10);

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
            {
                bandFx.multiFx.setIncType(inc);
                config.bandFx1 = bandFx.multiFx.fxType;
                storage.Save();
            }
            else if (isFx == 1)
            {
                bandFx.multiFx2.setIncType(inc);
                config.bandFx2 = bandFx.multiFx2.fxType;
                storage.Save();
            }
            else if (isFx == 2)
            {
                multiFx1.setIncType(inc);
                config.masterFx1 = multiFx1.fxType;
                storage.Save();
            }
            else if (isFx == 3)
            {
                multiFx2.setIncType(inc);
                config.masterFx2 = multiFx2.fxType;
                storage.Save();
            }
            else
            {
                multiFx3.setIncType(inc);
                config.masterFx3 = multiFx3.fxType;
                storage.Save();
            }

            renderFx();
        }
        if (encoder.RisingEdge())
        {
            isFx = (isFx + 1) % 5;
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
