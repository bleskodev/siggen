#include <gmock/gmock.h>
#include "src/configsubs.h"
#include "src/sysfio.h"
#include "test/sysfio_mock.hpp"

using ::testing::Return;
using ::testing::StrEq;
using ::testing::_;

class Configsubs_FixtureWithStrictSysfioMock : public ::testing::Test
{
  protected:
    void SetUp() override
    {
      sysfio_set_callbacks(SysFIOMock::fopen, SysFIOMock::fclose, SysFIOMock::fgets, SysFIOMock::rewind);
    }
    
    void TearDown() override
    {
      sysfio_reset_callbacks();
    }

    ::testing::StrictMock<SysFIOMock> sysfioMock;
};

class Configsubs_FixtureWithSysfioMock : public ::testing::Test
{
  protected:
    void SetUp() override
    {
      sysfio_set_callbacks(SysFIOMock::fopen, SysFIOMock::fclose, SysFIOMock::fgets, SysFIOMock::rewind);

      ON_CALL(sysfioMock,
          mocked_fopen(_, _)).WillByDefault(Return(reinterpret_cast<FILE*>(1)));
      ON_CALL(sysfioMock,
          mocked_fclose(_)).WillByDefault(Return(0));

      char local[] = "local";
      init_conf_files(local, NULL, NULL, 0);
    }
    
    void TearDown() override
    {
      close_conf_files();
      sysfio_reset_callbacks();
    }

    ::testing::NiceMock<SysFIOMock> sysfioMock;
};

TEST_F(Configsubs_FixtureWithStrictSysfioMock, init_config_files_does_nothing_if_no_config_specified)
{
  // no functions should be called -> no expectations
  
  EXPECT_EQ(0, init_conf_files(NULL, NULL, NULL, 0));
}

TEST_F(Configsubs_FixtureWithStrictSysfioMock, close_conf_files_does_nothing_if_no_files_were_open)
{
  // no functions should be called -> no expectations
  
  close_conf_files();
}

TEST_F(Configsubs_FixtureWithStrictSysfioMock, init_config_files_local)
{
  EXPECT_CALL(sysfioMock, mocked_fopen(StrEq("local"), StrEq("r"))).Times(1).WillOnce(Return(nullptr));
  
  char localName[] = "local";
  EXPECT_EQ(0, init_conf_files(localName, NULL, NULL, 0));
}

TEST_F(Configsubs_FixtureWithStrictSysfioMock, init_config_files_home)
{
  const char* homeDir = getenv("HOME");
  std::string homeConfigPath(homeDir ? homeDir : "");
  homeConfigPath += "/";
  homeConfigPath += "homeConfig";

  EXPECT_CALL(sysfioMock,
              mocked_fopen(StrEq(homeConfigPath.c_str()), StrEq("r"))).Times(1).WillOnce(Return(nullptr));
  
  char homeName[] = "homeConfig";
  EXPECT_EQ(0, init_conf_files(NULL, &homeName[0], NULL, 0));
}

TEST_F(Configsubs_FixtureWithStrictSysfioMock, init_config_files_global)
{
  EXPECT_CALL(sysfioMock, mocked_fopen(StrEq("global"), StrEq("r"))).Times(1).WillOnce(Return(nullptr));
  
  char globalName[] = "global";
  EXPECT_EQ(0, init_conf_files(globalName, NULL, NULL, 0));
}

TEST_F(Configsubs_FixtureWithStrictSysfioMock, close_config_files)
{
  EXPECT_CALL(sysfioMock,
      mocked_fopen(_, _)).Times(1).WillOnce(Return(reinterpret_cast<FILE*>(1)));
  EXPECT_CALL(sysfioMock,
      mocked_fclose(reinterpret_cast<FILE*>(1))).Times(1).WillOnce(Return(0));
  
  char globalName[] = "global";
  EXPECT_EQ(0, init_conf_files(NULL, NULL, globalName, 0));
  close_conf_files();
}

TEST_F(Configsubs_FixtureWithStrictSysfioMock, get_conf_value_returns_default)
{
  char expectedValue[] = "defaultValue";
  char sys[] = "mySystem";
  char name[] = "myName";
  char* defaultVal = expectedValue;
  EXPECT_STREQ(expectedValue, get_conf_value(sys, name, defaultVal));
}

ACTION_P(CopyStringToArg0, str)
{
  strncpy(arg0, str, arg1);
  return arg0;
}

TEST_F(Configsubs_FixtureWithSysfioMock, get_conf_value_returns_system_value)
{
  char expectedValue[] = "10";
  char sys[] = "sys";
  char name[] = "name";
  char defaultVal[] = "5";

  ON_CALL(sysfioMock, mocked_fgets(_, _, _)).WillByDefault(CopyStringToArg0("sys:name   10"));

  EXPECT_STREQ(expectedValue, get_conf_value(sys, name, defaultVal));
}

TEST_F(Configsubs_FixtureWithSysfioMock, get_conf_value_returns_global_value)
{
  char expectedValue[] = "10";
  char sys[] = "sys";
  char name[] = "name";
  char defaultVal[] = "5";

  EXPECT_CALL(sysfioMock, mocked_fgets(_, _, _))
    .WillOnce(CopyStringToArg0("name   10"))
    .WillOnce(Return(nullptr))
    .WillOnce(CopyStringToArg0("name   10"));

  EXPECT_STREQ(expectedValue, get_conf_value(sys, name, defaultVal));
}

TEST_F(Configsubs_FixtureWithSysfioMock, get_conf_value_returns_second_value)
{
  char expectedValue[] = "10";
  char sys[] = "sys";
  char name[] = "name";
  char defaultVal[] = "5";

  EXPECT_CALL(sysfioMock, mocked_fgets(_, _, _))
    .WillOnce(CopyStringToArg0("toto   3"))
    .WillOnce(Return(nullptr))
    .WillOnce(CopyStringToArg0("toto   7"))
    .WillOnce(CopyStringToArg0("name   10"));

  EXPECT_STREQ(expectedValue, get_conf_value(sys, name, defaultVal));
}

TEST_F(Configsubs_FixtureWithSysfioMock, get_conf_value_returns_default_value)
{
  char expectedValue[] = "5";
  char sys[] = "sys";
  char name[] = "name";
  char defaultVal[] = "5";

  EXPECT_CALL(sysfioMock, mocked_fgets(_, _, _))
    .WillOnce(CopyStringToArg0("sys:toto   3"))
    .WillOnce(Return(nullptr))
    .WillOnce(CopyStringToArg0("pic:toto   7"))
    .WillOnce(CopyStringToArg0("namey   10"))
    .WillOnce(CopyStringToArg0("nam   10"))
    .WillOnce(Return(nullptr));

  EXPECT_STREQ(expectedValue, get_conf_value(sys, name, defaultVal));
}
