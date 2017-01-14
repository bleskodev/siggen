#include <gmock/gmock.h>
#include "src/configsubs.h"
#include "test/sysfio_mock.hpp"

TEST(configsubs, init_config_files_does_nothing_if_no_config_specified)
{
  ::testing::StrictMock<SysFIOMock> sysfioMock;
  // no functions should be called -> no expectations
  
  EXPECT_EQ(0, init_conf_files(NULL, NULL, NULL, 0));
}

TEST(configsubs, close_conf_files_does_nothing_if_no_files_were_open)
{
  ::testing::StrictMock<SysFIOMock> sysfioMock;
  // no functions should be called -> no expectations
  
  EXPECT_EQ(0, close_conf_files());
}
