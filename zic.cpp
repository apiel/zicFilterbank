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

AdcChannelConfig knobBandMix;
AdcChannelConfig knobBandFreq;
AdcChannelConfig knobBandRange;
AdcChannelConfig knobBandFx;
AdcChannelConfig knobFilterMix;
AdcChannelConfig knobFilterCutoff;
AdcChannelConfig knobFilterResonance;
AdcChannelConfig knobFilterFeedback;
AdcChannelConfig knobFilterFx;
AdcChannelConfig knobMasterFx;

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
float knobValues[NUM_KNOBS];

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

void renderKnob(std::string knobName, float value)
{
    display.Fill(false);
    text(display, 0, 0, knobName, PoppinsLight_8);
    text(display, 0, 16, std::to_string((int)(value * 10000)), PoppinsLight_12);
    display.Update();
}

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
        knobValues[i] = hw.adc.GetFloat(i);
    }

    renderFx();
    // uint8_t fps = 30;
    // uint8_t frame = 0;
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

        float knob = hw.adc.GetFloat(MASTER_FX);
        if (knob != knobValues[MASTER_FX])
        {
            knobValues[MASTER_FX] = knob;
            renderKnob("Master Fx", knob);
        }

        // if (frame == fps)
        // {
        //     frame = 0;
        //     display.Fill(false);
        //     // draw random pixels
        //     display.DrawPixel(rand() % 64, rand() % 32, true);

        //     text(display, 0, 0, "Zic", Sinclair_S);
        //     display.Update();
        // }
        // frame++;

        System::Delay(1);
    }
}
