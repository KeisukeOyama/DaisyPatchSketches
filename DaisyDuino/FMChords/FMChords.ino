#include "DaisyDuino.h"
#include <U8x8lib.h>

//the magic incantation
U8X8_SSD1309_128X64_NONAME0_4W_SW_SPI oled8x8(/* clock=*/8, /* data=*/10, /* cs=*/7, /* dc=*/9, /* reset=*/30);

const int CHORD_TYPE_COUNT = 16;

const int CHORD_TYPE[CHORD_TYPE_COUNT][4] = {
    // Maj7
    {36, 40, 43, 47},
    // Maj9
    {36, 40, 47, 50},
    // Maj#11
    {36, 40, 47, 54},
    // Maj13
    {36, 40, 47, 57},
    // m7
    {36, 39, 43, 46},
    // m9
    {36, 39, 46, 50},
    //m11
    {36, 39, 46, 53},
    //m13
    {36, 39, 46, 57},
    // mMaj7
    {36, 39, 43, 47},
    //m7b5
    {36, 39, 42, 46},
    // 7
    {36, 40, 43, 46},
    // 7b9
    {36, 40, 46, 49},
    // 7#9
    {36, 40, 46, 51},
    // 7sus4
    {36, 41, 43, 46},
    // dim7
    {36, 39, 42, 45},
    // aug7
    {36, 40, 44, 46}};

const String CHORD_NAMES[CHORD_TYPE_COUNT] = {"Maj7", "Maj9", "Maj#11", "Maj13", "m7", "m9",
                                              "m11", "m13", "mMaj7", "m7b5", "7", "7b9", "7#9", "7sus4", "dim7", "aug7"};

const float FREQ_C2 = 65.406;

const String FREQ_TITLE = "Freq";
const String CHORD_TYPE_TITLE = "Chord";
const String CHORD_INVERSION_TITLE = "Inv";
const String FM_AMOUNT_TITLE = "Fm";

DaisyHardware hw;
float sampleRate;
Oscillator car[4];
Oscillator mod[4];
float freq[4];

int chordIndex;
float baseFreq;
int inversion;
float modIndex;

float rc = 0;
static float a = 0.8;
float prevRc = 0;
int prevCtrl1;

unsigned prev_digit[4];

unsigned long curTime, prevTime;

void setup()
{
    hw = DAISY.init(DAISY_PATCH, AUDIO_SR_48K);
    sampleRate = DAISY.get_samplerate();

    for (int i = 0; i < 4; i++)
    {
        car[i].Init(sampleRate);
        mod[i].Init(sampleRate);
    }

    oled8x8.setFont(u8x8_font_chroma48medium8_r);
    oled8x8.begin();

    analogReadResolution(12);
    analogWriteResolution(12);

    DAISY.begin(AudioCallback);
}

void loop()
{

    int ctrl1 = (GetCtrl(0) / 4095.0f) * 16;

    baseFreq = mtof(((GetCtrl(0) / 4095.0f) * 96) + 12);
    String str = String((int)baseFreq) + "Hz";
    DrawParam(FREQ_TITLE, str, 0, 1);

    chordIndex = (GetCtrl(1) / 4095.0f) * 16;
    DrawParam(CHORD_TYPE_TITLE, CHORD_NAMES[chordIndex], 1, 2);

    inversion = (GetCtrl(2) / 4095.0f) * 8;
    DrawParam(CHORD_INVERSION_TITLE, String(inversion),2, 3);

    modIndex = (GetCtrl(3) / 4095.0f) * 1.0f;
    DrawParam(FM_AMOUNT_TITLE, String(modIndex), 3, 4);

    CalcChordFreq(baseFreq, chordIndex, inversion);
    SetFreqs(2.0f);
}

static void AudioCallback(float **in, float **out, size_t size)
{

    for (size_t i = 0; i < size; i++)
    {

        float mix = 0;

        for (size_t chn = 0; chn < 4; chn++)
        {
            float modval = mod[chn].Process();
            car[chn].PhaseAdd(modval * (modIndex*0.2f));
            mix += car[chn].Process() * 0.25f;
        }

        out[0][i] = mix;
    }
}

uint32_t GetCtrl(int num)
{

    uint32_t val;

    switch (num)
    {
    case 0:
        val = 4095 - analogRead(PIN_PATCH_CTRL_1);
        break;
    case 1:
        val = 4095 - analogRead(PIN_PATCH_CTRL_2);
        break;
    case 2:
        val = 4095 - analogRead(PIN_PATCH_CTRL_3);
        break;
    case 3:
        val = 4095 - analogRead(PIN_PATCH_CTRL_4);
        break;
    }

    return val;
}

void CalcChordFreq(float base_freq, int chordIndex, int inversion)
{
    switch (inversion)
    {
    case 0:
        freq[0] = base_freq;
        freq[1] = base_freq;
        freq[2] = base_freq;
        freq[3] = base_freq;
        break;
    case 1:
        freq[0] = base_freq;
        freq[1] = base_freq;
        freq[2] = (mtof(CHORD_TYPE[chordIndex][1]) / FREQ_C2) * base_freq;
        freq[3] = (mtof(CHORD_TYPE[chordIndex][1]) / FREQ_C2) * base_freq;
        break;
    case 2:
        freq[0] = base_freq;
        freq[1] = (mtof(CHORD_TYPE[chordIndex][1]) / FREQ_C2) * base_freq;
        freq[2] = (mtof(CHORD_TYPE[chordIndex][2]) / FREQ_C2) * base_freq;
        freq[3] = (mtof(CHORD_TYPE[chordIndex][2]) / FREQ_C2) * base_freq;
        break;
    case 3:
        freq[0] = base_freq;
        freq[1] = (mtof(CHORD_TYPE[chordIndex][1]) / FREQ_C2) * base_freq;
        freq[2] = (mtof(CHORD_TYPE[chordIndex][2]) / FREQ_C2) * base_freq;
        freq[3] = (mtof(CHORD_TYPE[chordIndex][3]) / FREQ_C2) * base_freq;
        break;
    case 4:
        freq[0] = base_freq * 2.0f;
        freq[1] = (mtof(CHORD_TYPE[chordIndex][1]) / FREQ_C2) * base_freq;
        freq[2] = (mtof(CHORD_TYPE[chordIndex][2]) / FREQ_C2) * base_freq;
        freq[3] = (mtof(CHORD_TYPE[chordIndex][3]) / FREQ_C2) * base_freq;
        break;
    case 5:
        freq[0] = base_freq * 2.0f;
        freq[1] = (mtof(CHORD_TYPE[chordIndex][1]) / FREQ_C2) * base_freq * 2.0f;
        freq[2] = (mtof(CHORD_TYPE[chordIndex][2]) / FREQ_C2) * base_freq;
        freq[3] = (mtof(CHORD_TYPE[chordIndex][3]) / FREQ_C2) * base_freq;
        break;
    case 6:
        freq[0] = base_freq * 2.0f;
        freq[1] = (mtof(CHORD_TYPE[chordIndex][1]) / FREQ_C2) * base_freq * 2.0f;
        freq[2] = (mtof(CHORD_TYPE[chordIndex][2]) / FREQ_C2) * base_freq * 2.0f;
        freq[3] = (mtof(CHORD_TYPE[chordIndex][3]) / FREQ_C2) * base_freq;
        break;
    case 7:
        freq[0] = base_freq;
        freq[1] = (mtof(CHORD_TYPE[chordIndex][1]) / FREQ_C2) * base_freq;
        freq[2] = (mtof(CHORD_TYPE[chordIndex][2]) / FREQ_C2) * base_freq * 2.0f;
        freq[3] = (mtof(CHORD_TYPE[chordIndex][3]) / FREQ_C2) * base_freq * 2.0f;
        break;
    }
}

void SetFreqs(float ratio)
{
    for (int i = 0; i < 4; i++)
    {
        car[i].SetFreq(freq[i]);
        mod[i].SetFreq(freq[i] * ratio);
    }
}

void DrawParam(String titleStr, String valueStr, int index, int y)
{
    
    if (valueStr.length() != prev_digit[index]) {
        oled8x8.clearLine(y);
        prev_digit[index] = valueStr.length();
    }

    char *c1 = &titleStr[0];
    oled8x8.drawString(1, y, c1);

    char *c2 = &valueStr[0];
    oled8x8.drawString(8, y, c2);
}
