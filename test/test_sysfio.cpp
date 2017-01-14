#include <gmock/gmock.h>
#include "src/sysfio.h"
#include "test/sysfio_mock.hpp"

using ::testing::_;
using ::testing::Return;

TEST(sysfio, set_callbacks_are_called)
{
  SysFIOMock sysfioMock;
  EXPECT_CALL(sysfioMock, mocked_fopen(_, _)).Times(1).WillOnce(Return(nullptr));
  EXPECT_CALL(sysfioMock, mocked_fclose(_)).Times(1).WillOnce(Return(1));
  EXPECT_CALL(sysfioMock, mocked_fgets(_, _, _)).Times(1).WillOnce(Return(nullptr));
  EXPECT_CALL(sysfioMock, mocked_rewind(_)).Times(1);

  sysfio_set_callbacks(SysFIOMock::fopen, SysFIOMock::fclose, SysFIOMock::fgets, SysFIOMock::rewind);
  EXPECT_EQ(nullptr, sysfio_fopen("blabla", "r"));
  EXPECT_EQ(1, sysfio_fclose(nullptr));
  char buffer[10];
  EXPECT_EQ(nullptr, sysfio_fgets(buffer, 5, nullptr));
  sysfio_rewind(nullptr);

  sysfio_reset_callbacks();
}

