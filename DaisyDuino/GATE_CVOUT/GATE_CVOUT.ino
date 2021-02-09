#include "DaisyDuino.h"
#include <U8x8lib.h>

U8X8_SSD1309_128X64_NONAME0_4W_SW_SPI oled8x8(/* clock=*/8, /* data=*/10, /* cs=*/7, /* dc=*/9, /* reset=*/30);

DaisyHardware hw;
float sampleRate;

Metro tick;
bool gate;
float hz;
unsigned prev_digit;

uint32_t rand_cv;

void setup()
{
    hw = DAISY.init(DAISY_PATCH, AUDIO_SR_48K);
    sampleRate = DAISY.get_samplerate();

    oled8x8.setFont(u8x8_font_chroma48medium8_r);
    oled8x8.begin();

    analogReadResolution(12);
    analogWriteResolution(12);
    pinMode(PIN_PATCH_GATE_OUT, OUTPUT);
    pinMode(PIN_PATCH_CV_1, OUTPUT);

    tick.Init(1.0f, sampleRate);    


    DAISY.begin(AudioCallback);

}

void loop()
{

    uint32_t val = (4095 - analogRead(PIN_PATCH_CTRL_1));
    val = map(val, 0, 4095, 30, 300);
    String str = String(val);
    UpdateDisp("Tempo", str);

    hz = (float)val / 60.0f;
    tick.SetFreq(hz * 2.0f);

    
    digitalWrite(PIN_PATCH_GATE_OUT, gate ? HIGH:LOW);


    analogWrite(PIN_PATCH_CV_1, rand_cv);

}

void UpdateDisp(String titleStr, String valueStr)
{

    if (valueStr.length() != prev_digit) {
        oled8x8.clear();
        prev_digit = valueStr.length();
    }

    char *c1 = &titleStr[0];
    oled8x8.drawString(1, 1, c1);

    char *c2 = &valueStr[0];
    oled8x8.draw2x2String(8 - valueStr.length(), 4, c2);
}

static void AudioCallback(float **in, float **out, size_t size)
{

    for (size_t i = 0; i < size; i++)
    {

        if (tick.Process())
        {
            gate = !gate;
            rand_cv = random(4096);
        }
  
    }

}
