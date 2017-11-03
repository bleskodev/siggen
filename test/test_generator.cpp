#include <gmock/gmock.h>
#include "src/generator.h"
#include <linux/soundcard.h>
#include <cstring>

// vflg is required for generator.c to compile.
// TODO: remove this requirement.
int vflg = 0;

//**************************************************************
TEST(generator, mk8bit)
{
  const short valMax = std::numeric_limits<short>::max();
  const short valMin = std::numeric_limits<short>::min();

  short input[] = { valMax, valMax/2, 0, valMin/2, valMin };
  const int inputLen = sizeof(input)/sizeof(*input);
  const unsigned char expectedOutput[] = { 255, 255-128/2, 128, 128/2, 0 };
  
  unsigned char output[inputLen];
  EXPECT_EQ(0, mk8bit(output, input, inputLen));

  for (int i = 0; i < inputLen; ++i)
  {
    EXPECT_EQ(expectedOutput[i], output[i]);
  }
}

//**************************************************************
TEST(generator, mixplaybuf_8bit)
{
  unsigned char inputA[] = { 25, 15, 255, 128 };
  unsigned char inputB[] = { 66, 128, 15, 148 };
  const int inputLen = sizeof(inputA)/sizeof(*inputA);
  const unsigned char expectedOutput[] = { 25, 66, 15, 128, 255, 15, 128, 148 };
  const int outputLen = sizeof(expectedOutput)/sizeof(*expectedOutput);

  unsigned char output[outputLen];
  mixplaybuf(output, inputA, inputB, inputLen, AFMT_U8);
  
  for (int i = 0; i < outputLen; ++i)
  {
    EXPECT_EQ(expectedOutput[i], output[i]);
  }
}

TEST(generator, mixplaybuf_16bit)
{
  unsigned char inputA[] = { 25, 15, 255, 128 };
  unsigned char inputB[] = { 66, 128, 15, 148 };
  const int inputLen = sizeof(inputA)/sizeof(*inputA);
  const unsigned char expectedOutput[] = { 25, 15, 66, 128, 255, 128, 15, 148 };
  const int outputLen = sizeof(expectedOutput)/sizeof(*expectedOutput);

  unsigned char output[outputLen];
  mixplaybuf(output, inputA, inputB, inputLen/2, AFMT_S16_LE);
  
  for (int i = 0; i < outputLen; ++i)
  {
    EXPECT_EQ(expectedOutput[i], output[i]);
  }
}

//**************************************************************
/* enable when the code is fixed
TEST(generator, chanmix_8bit)
{
  unsigned char inputA[] = { 25, 15, 255, 128 };
  unsigned char inputB[] = { 66, 128, 145, 148 };
  const int inputLen = sizeof(inputA)/sizeof(*inputA);
  const int outputLen = inputLen;
  const int amplInputA = 100;
  const int amplInputB = 50;

  const unsigned char expectedOutput[inputLen] = { 0, 15, 255, 138 };
  unsigned char output[outputLen];
  chanmix(output, inputA, amplInputA, inputB, amplInputB,  inputLen, AFMT_U8);
  
  for (int i = 0; i < outputLen; ++i)
  {
    EXPECT_EQ(expectedOutput[i], output[i]);
  }
}
*/
/* enable when the code is fixed
TEST(generator, chanmix_16bit)
{
  short inputA[] = { -32700, 15, 32700, 0 };
  short inputB[] = { -500, 0, 500, 148 };
  const int inputLen = sizeof(inputA)/sizeof(*inputA);
  const int outputLen = inputLen;
  const int amplInputA = 100;
  const int amplInputB = 50;

  const short valMax = std::numeric_limits<short>::max();
  const short valMin = std::numeric_limits<short>::min();
  
  const short expectedOutput[inputLen] = { valMin, 15, valMax, 74 };
  short output[outputLen];
  chanmix(reinterpret_cast<unsigned char*>(output),
          reinterpret_cast<unsigned char*>(inputA), amplInputA,
          reinterpret_cast<unsigned char*>(inputB), amplInputB,
          inputLen, AFMT_S16_LE);
  
  for (int i = 0; i < outputLen; ++i)
  {
    EXPECT_EQ(expectedOutput[i], output[i]);
  }
}
*/

//**************************************************************
TEST(generator, do_antiphase_8bits)
{
  unsigned char input[] = { 25, 15, 255, 128 };
  const int inputLen = sizeof(input)/sizeof(*input);
  const int outputLen = inputLen;

  const unsigned char expectedOutput[inputLen] = { 231, 241, 1, 128 };
  unsigned char output[outputLen];
  do_antiphase(output, input, inputLen, AFMT_U8);
  
  for (int i = 0; i < outputLen; ++i)
  {
    EXPECT_EQ(expectedOutput[i], output[i]);
  }
}

TEST(generator, do_antiphase_16bits)
{
  short input[] = { -32700, 15, 32700, 0 };
  const int inputLen = sizeof(input)/sizeof(*input);
  const int outputLen = inputLen;

  const short expectedOutput[inputLen] = { 32700, -15, -32700, 0 };
  short output[outputLen];
  do_antiphase(reinterpret_cast<unsigned char*>(output),
               reinterpret_cast<unsigned char*>(input),
               inputLen, AFMT_S16_LE);
  
  for (int i = 0; i < outputLen; ++i)
  {
    EXPECT_EQ(expectedOutput[i], output[i]);
  }
}

//**************************************************************
TEST(generator, generate_fails_invalid_waveform)
{
  char wf[] = "toto";
  EXPECT_EQ(0, generate(wf, nullptr, 100, 50, 50, 100, 0, AFMT_U8));
}

TEST(generator, generate_fails_frequency_too_high_for_samples)
{
  char wf[] = "sine";
  unsigned int samplesPerSec = 100;
  unsigned int freq = 51;
  EXPECT_EQ(0, generate(wf, nullptr, samplesPerSec, freq, 50, samplesPerSec, 0, AFMT_U8));
}

//**************************************************************
TEST(generator, generate_silence_8bits)
{
  char wf[] = "off";
  const unsigned int samplesPerSec = 10;
  const unsigned int freq = 5;
  const unsigned int ampl = 50;
  char buff[samplesPerSec];
  std::memset(buff, 0xcd, samplesPerSec);

  EXPECT_EQ(samplesPerSec, generate(wf, buff, samplesPerSec, freq, ampl, samplesPerSec, 0, AFMT_U8));
  for (auto val : buff)
  {
    EXPECT_EQ(char(128), val);
  }
}

TEST(generator, generate_silence_16bits)
{
  char wf[] = "off";
  const unsigned int samplesPerSec = 10;
  const unsigned int bufferSize = samplesPerSec * 2;
  const unsigned int freq = 5;
  const unsigned int ampl = 50;
  char buff[bufferSize];
  std::memset(buff, 0xcd, bufferSize);

  EXPECT_EQ(samplesPerSec, generate(wf, buff, bufferSize, freq, ampl, samplesPerSec, 0, AFMT_S16_LE));
  for (auto val : buff)
  {
    EXPECT_EQ(char(0), val);
  }
}

TEST(generator, generate_square_8bits)
{
  char wf[] = "square";
  const unsigned int samplesPerSec = 100;
  const unsigned int bufferSize = samplesPerSec;
  const unsigned int freq = 1;
  const unsigned int ampl = 70;
  const unsigned int ratio = 50;
  char buff[bufferSize];
  char expected[bufferSize];
  const char maxValue = 128 + ampl * 127 / 100;
  const char minValue = 128 - ampl * 128 / 100;
  for(int i = 0; i < bufferSize; ++i)
  {
    expected[i] = i < ratio ? maxValue : minValue;
  }

  EXPECT_EQ(samplesPerSec, generate(wf, buff, bufferSize, freq, ampl, samplesPerSec, 0, AFMT_U8));
  for (int i = 0; i < bufferSize; ++i)
  {
    EXPECT_EQ(expected[i], buff[i]);
  }
}

TEST(generator, generate_square_16bits)
{
  char wf[] = "square";
  const unsigned int samplesPerSec = 100;
  const unsigned int bufferSize = samplesPerSec * 2;
  const unsigned int freq = 1;
  const unsigned int ampl = 70;
  const unsigned int ratio = 50;
  char buff[bufferSize];
  short int expected[samplesPerSec];
  const short int value = ampl * 32767 / 100;
  for(int i = 0; i < samplesPerSec; ++i)
  {
    expected[i] = i < ratio ? value : -value;
  }

  EXPECT_EQ(samplesPerSec, generate(wf, buff, bufferSize, freq, ampl, samplesPerSec, 0, AFMT_S16_LE));
  short int* buffAsShort = reinterpret_cast<short int*>(buff);
  for (int i = 0; i < samplesPerSec; ++i)
  {
    EXPECT_EQ(expected[i], buffAsShort[i]);
  }
}

TEST(generator, generate_pulse_8bits)
{
  char wf[] = "pulse";
  const unsigned int samplesPerSec = 100;
  const unsigned int bufferSize = samplesPerSec;
  const unsigned int freq = 1;
  const unsigned int ampl = 50;
  const unsigned int ratio = 40;
  char buff[bufferSize];
  char expected[bufferSize];
  const char maxValue = 128 + ampl * 127 / 100;
  const char minValue = 128 - ampl * 128 / 100;
  for(int i = 0; i < bufferSize; ++i)
  {
    expected[i] = i < ratio ? maxValue : minValue;
  }

  EXPECT_EQ(samplesPerSec, generate(wf, buff, bufferSize, freq, ampl, samplesPerSec, ratio, AFMT_U8));
  for (int i = 0; i < bufferSize; ++i)
  {
    EXPECT_EQ(expected[i], buff[i]);
  }
}

TEST(generator, generate_pulse_16bits)
{
  char wf[] = "pulse";
  const unsigned int samplesPerSec = 100;
  const unsigned int bufferSize = samplesPerSec * 2;
  const unsigned int freq = 1;
  const unsigned int ampl = 50;
  const unsigned int ratio = 40;
  char buff[bufferSize];
  short int expected[samplesPerSec];
  const short int value = ampl * 32767 / 100;
  for(int i = 0; i < samplesPerSec; ++i)
  {
    expected[i] = i < ratio ? value : -value;
  }

  EXPECT_EQ(samplesPerSec, generate(wf, buff, bufferSize, freq, ampl, samplesPerSec, ratio, AFMT_S16_LE));
  short int* buffAsShort = reinterpret_cast<short int*>(buff);
  for (int i = 0; i < samplesPerSec; ++i)
  {
    EXPECT_EQ(expected[i], buffAsShort[i]);
  }
}
