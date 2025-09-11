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

void renderFx()
{
    display.Fill(false);
    text(display, 0, 0, "Fx", Sinclair_S);
    text(display, 0, 20, multiFx.typeName, Sinclair_S);
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
