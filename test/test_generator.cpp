#include <gmock/gmock.h>
#include "src/generator.h"
#include <linux/soundcard.h>

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

