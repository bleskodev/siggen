#pragma once

#include <gmock/gmock.h>

class SysFIOMock
{
  public: 
    MOCK_METHOD2(mocked_fopen, FILE*(const char*, const char*));
    MOCK_METHOD1(mocked_fclose, int(FILE*));
    MOCK_METHOD3(mocked_fgets, char*(char *, int, FILE *));
    MOCK_METHOD1(mocked_rewind, void(FILE*));

    SysFIOMock() { mInstance = this; }
    ~SysFIOMock() { mInstance = nullptr; }

    static FILE* fopen(const char* filename, const char* mode)
    {
      if (mInstance)
        return mInstance->mocked_fopen(filename, mode);
      return nullptr;
    }

    static int fclose(FILE* stream)
    {
      if (mInstance)
        return mInstance->mocked_fclose(stream);
      return 1;
    }

    static char* fgets(char *str, int count, FILE *stream)
    {
      if (mInstance)
        return mInstance->mocked_fgets(str, count, stream);
      return nullptr;
    }

    static void rewind(FILE* stream)
    {
      if (mInstance)
        mInstance->mocked_rewind(stream);
    }

  private:
    static SysFIOMock* mInstance;
};
