#include "usermodfx.h"
#include "buffer_ops.h"
#include <atomic>

#define BUFFER_LEN 200 //1024

static bool rateChange, gainChange;
static uint8_t sampleLoop;
static uint16_t gain, rateLength;
static float sampleSumL, sampleSumR;

static __sdram float audioSample[BUFFER_LEN];
static std::atomic<uint16_t> rateVal(0);
static std::atomic<uint16_t> gainVal(0);

//Initialization process
void MODFX_INIT(uint32_t platform, uint32_t api)
{
  buf_clr_f32(audioSample, BUFFER_LEN);
  gain = 0;
  rateLength = 1;

  rateChange = false;
  gainChange = false;

  rateVal = 0;
  gainVal = 0;

  sampleLoop = 0;
  sampleSumL = 0.0f;
  sampleSumR = 0.0f;
}

/*
    Goal: Do a distortion
    A: Set Tone
    B: Set Gain
*/
void MODFX_PROCESS(const float *main_xn, float *main_yn,
                   const float *sub_xn, float *sub_yn,
                   uint32_t frames)
{
    for(uint32_t i = 0; i < frames; i++)
    {  
      if(rateLength < 1) // No Tone
      {
        main_yn[i * 2]     = clip1m1f(main_xn[i * 2]     * gain) * 0.95;
        main_yn[i * 2 + 1] = clip1m1f(main_xn[i * 2 + 1] * gain) * 0.95;
      }
      else // Tone
      {
        if(sampleLoop > rateLength)
        {
          sampleLoop = 0;
        }
        audioSample[sampleLoop * 2]     = clip1m1f(main_xn[i * 2]     * gain) * 0.95;
        audioSample[sampleLoop * 2 + 1] = clip1m1f(main_xn[i * 2 + 1] * gain) * 0.95;
        sampleLoop++;

        sampleSumL = 0.0f;
        sampleSumR = 0.0f;
        for(uint8_t j = 0; j < rateLength; j++)
        {
          sampleSumL += audioSample[j * 2]; 
          sampleSumR += audioSample[j * 2 + 1];  
        }

        main_yn[i * 2]     = sampleSumL / rateLength;
        main_yn[i * 2 + 1] = sampleSumR / rateLength;
      }
      // Handle parameters changing without undefined behavior.
      if(rateChange)
      {
        rateLength = rateVal;
        rateChange = false;
      }

      if(gainChange)
      {
        gain = gainVal;
        gainChange = false;
      }
    }
}

// Knobs.
void MODFX_PARAM(uint8_t index, int32_t value)
{
  //Convert fixed point q31 format to float
  //Makes the knobs 0.0f - 1.0f
  const float valf = q31_to_f32(value);
  switch (index)
  {
    //A knob
    case k_user_modfx_param_time:
      rateVal = (uint16_t)(valf * 20);
      rateChange = true;
      break;
    //B Knob. Gain needs to be at least 1 or you will get silence.
    case k_user_modfx_param_depth:
      gainVal = (uint16_t)(valf * 300) + 1;
      gainChange = true;
      break;
    default:
      break;
  }
}