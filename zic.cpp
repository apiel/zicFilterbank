#include <string.h>

#include "libDaisy/src/dev/oled_ssd130x.h"
#include "daisy_seed.h"

#include "effects/MultiFx.h"

using namespace daisy;

DaisySeed hw;
MultiFx multiFx;
Encoder encoder;

SSD130xI2c64x48Driver display;
SSD130xI2c64x48Driver::Config displayCfg;

// Encoder
constexpr Pin ENC_A_PIN = seed::D8;
constexpr Pin ENC_B_PIN = seed::D10;
constexpr Pin ENC_CLICK_PIN = seed::D9;

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
    while (1)
    {
        encoder.Debounce();
        int32_t inc = encoder.Increment();
        if (inc)
        {
            multiFx.setIncType(inc);
        }

        display.Fill(false);
        // draw random pixels
        display.DrawPixel(rand() % 48, rand() % 64, true);
        display.Update();

        System::Delay(1);
    }
}
