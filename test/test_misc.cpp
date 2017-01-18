#include <gmock/gmock.h>
#include "src/misc.h"

// err_rpt() requires extern char* sys to be defined somewhere
// TODO: remove this requirement
char* sys;

TEST(misc, hcf)
{
  EXPECT_EQ(6, hcf(54, 24));
  EXPECT_EQ(14, hcf(42, 56));

  EXPECT_EQ(1, hcf(5, 7));

  // TODO: handle O argument correctly
  //EXPECT_EQ(6, hcf(0, 6));
}

//**************************************************************
TEST(misc, parse_1_param)
{
  const char* output[MAX_ARGS];
  char sep = ' ';
  char input[] = "parameter";
  const int paramCount = parse(input, output, sep);
  EXPECT_EQ(1, paramCount);
  EXPECT_EQ(output[0], input);
}

TEST(misc, parse_1_param_with_quotes)
{
  const char* output[MAX_ARGS];
  char sep = ' ';
  char input[] = "'this is one parameter'";
  const int paramCount = parse(input, output, sep);
  EXPECT_EQ(1, paramCount);
  EXPECT_EQ(output[0], input);
}

TEST(misc, parse_2_params)
{
  const char* output[MAX_ARGS];
  char sep = ' ';
  char input[] = "parameter1 parameter2";
  const char* expectedParam1 = input;
  const char* expectedParam2 = strchr(input, sep) + 1;
  const int paramCount = parse(input, output, sep);
  EXPECT_EQ(2, paramCount);
  EXPECT_EQ(output[0], expectedParam1);
  EXPECT_EQ(output[1], expectedParam2);
}

TEST(misc, parse_2_params_with_quotes)
{
  const char* output[MAX_ARGS];
  char sep = ' ';
  char input[] = "parameter1 'this is parameter2'";
  const char* expectedParam1 = input;
  const char* expectedParam2 = strchr(input, sep) + 1;
  const int paramCount = parse(input, output, sep);
  EXPECT_EQ(2, paramCount);
  EXPECT_EQ(output[0], expectedParam1);
  EXPECT_EQ(output[1], expectedParam2);
}

TEST(misc, parse_with_custom_separator)
{
  const char* output[MAX_ARGS];
  char sep = '_';
  char input[] = "parameter1_parameter2";
  const char* expectedParam1 = input;
  const char* expectedParam2 = strchr(input, sep) + 1;
  const int paramCount = parse(input, output, sep);
  EXPECT_EQ(2, paramCount);
  EXPECT_EQ(output[0], expectedParam1);
  EXPECT_EQ(output[1], expectedParam2);
}

TEST(misc, parse_ignores_leading_separators)
{
  const char* output[MAX_ARGS];
  char sep = '_';
  char input[] = " \t_parameter1_ \tparameter2";
  const char* expectedParam1 = strchr(input, 'p');
  const char* expectedParam2 = strchr(expectedParam1 + 1, 'p');
  const int paramCount = parse(input, output, sep);
  EXPECT_EQ(2, paramCount);
  EXPECT_EQ(output[0], expectedParam1);
  EXPECT_EQ(output[1], expectedParam2);
}

//**************************************************************
TEST(misc, mstosamples)
{
  EXPECT_EQ(48000, mstosamples(1000, 48000));
  EXPECT_EQ(44100, mstosamples(1000, 44100));

  EXPECT_EQ(48, mstosamples(1, 48000));
  EXPECT_EQ(44, mstosamples(1, 44100));
}
