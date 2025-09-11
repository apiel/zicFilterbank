#include <string.h>

#include "libDaisy/src/dev/oled_ssd130x.h"
#include "daisy_seed.h"

#include "fonts/drawText.h"

#include "effects/MultiFx.h"

using namespace daisy;

DaisySeed hw;
MultiFx multiFx;
Encoder encoder;

SSD130xI2c64x32Driver display;
SSD130xI2c64x32Driver::Config displayCfg;

enum knob
{
    BAND_MIX,
    BAND_FREQ,
    BAND_RANGE,
    BAND_FX,
    FILTER_MIX,
    FILTER_CUTOFF,
    FILTER_RESONANCE,
    FILTER_FEEDBACK,
    FILTER_FX,
    MASTER_FX,
    NUM_KNOBS
};
AdcChannelConfig knobCfgs[NUM_KNOBS];
uint16_t knobValues[NUM_KNOBS];
uint16_t knobValuesPrev[NUM_KNOBS];
std::string knobNames[NUM_KNOBS] = {"Band Mix", "Band Freq", "Band Range", "Band Fx", "Filter Mix", "Filter Cutoff", "F. Reso.", "F. Feedback", "Filter Fx", "Master Fx"};

// Encoder
constexpr Pin ENC_A_PIN = seed::D8;
constexpr Pin ENC_B_PIN = seed::D10;
constexpr Pin ENC_CLICK_PIN = seed::D9;

bool isFx1 = true;

static void Callback(AudioHandle::InterleavingInputBuffer in,
                     AudioHandle::InterleavingOutputBuffer out,
                     size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        float input = in[i];
        float output = multiFx.apply(input, 1.0f);
        out[i] = output;
    }
}

void renderFx()
{
    display.Fill(false);
    if (isFx1)
    {
        text(display, 0, 0, "Fx 1", PoppinsLight_8);
        text(display, 0, 20, multiFx.typeName, PoppinsLight_8);
    }
    else
    {
        text(display, 0, 0, "Fx 2", PoppinsLight_8);
    }
    display.Update();
}

void renderKnob(std::string knobName, uint16_t value)
{
    display.Fill(false);
    text(display, 0, 0, knobName, PoppinsLight_8);
    int x = text(display, 0, 16, std::to_string(value), PoppinsLight_12);
    text(display, x + 2, 22, "%", PoppinsLight_8);
    display.Update();
}

float getKnobValue(knob knob) { return range(hw.adc.GetFloat(knob), 0.0f, 0.96f) / 0.96f; }
uint16_t f2i(float f) { return static_cast<uint16_t>(static_cast<uint>(f * 100000) * 0.001); }

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4);
    hw.StartAudio(Callback);
    multiFx.init(hw.AudioSampleRate(), MultiFx::FXType::REVERB);
    encoder.Init(ENC_A_PIN, ENC_B_PIN, ENC_CLICK_PIN);
    displayCfg.transport_config.i2c_config.pin_config.sda = seed::D12;
    displayCfg.transport_config.i2c_config.pin_config.scl = seed::D11;
    display.Init(displayCfg);

    knobCfgs[MASTER_FX].InitSingle(seed::A8);

    knobCfgs[FILTER_MIX].InitSingle(seed::A5);
    knobCfgs[FILTER_FX].InitSingle(seed::A4);
    knobCfgs[FILTER_CUTOFF].InitSingle(seed::A6);
    knobCfgs[FILTER_RESONANCE].InitSingle(seed::A10);
    knobCfgs[FILTER_FEEDBACK].InitSingle(seed::A11);

    knobCfgs[BAND_MIX].InitSingle(seed::A2);
    knobCfgs[BAND_FX].InitSingle(seed::A3);
    knobCfgs[BAND_FREQ].InitSingle(seed::A0);
    knobCfgs[BAND_RANGE].InitSingle(seed::A1);

    hw.adc.Init(knobCfgs, 10);
    hw.adc.Start();

    for (int i = 0; i < NUM_KNOBS; i++)
    {
        float f_knob = getKnobValue(MASTER_FX);
        uint16_t i_knob = f2i(f_knob);
        knobValues[i] = i_knob;
    }

    renderFx();
    while (1)
    {
        encoder.Debounce();
        int32_t inc = encoder.Increment();
        if (inc)
        {
            multiFx.setIncType(inc);
            renderFx();
        }
        if (encoder.RisingEdge())
        {
            isFx1 = !isFx1;
            renderFx();
        }

        for (int i = 0; i < NUM_KNOBS; i++)
        {
            float f_knob = getKnobValue((knob)i);
            uint16_t i_knob = f2i(f_knob);
            if (i_knob != knobValues[i])
            {
                if (i_knob != knobValuesPrev[i]) // To avoid ping pong of value
                {
                    knobValuesPrev[i] = knobValues[i];
                    knobValues[i] = i_knob;
                    renderKnob(knobNames[i], i_knob);
                }
            }
        }

        System::Delay(1);
    }
}
