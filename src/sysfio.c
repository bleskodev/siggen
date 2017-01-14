/* sysfio.c
 * system file i/o functions.
 * All system file reading goes through this interface in order to allow mocking
 * file i/o when unit testing.
 */

/*
 * Copyright (C) 1998-2008 Jim Jackson                    jj@franjam.org.uk
 * Copyright (C) 2017 Blesko Dev                          bleskodev@gmail.com
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program - see the file COPYING; if not, write to 
 *  the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, 
 *  MA 02139, USA.
 */

#include "sysfio.h"

/* File i/o callbacks which are used to override system provided functions.
 * Can be set using sysfio_set_callbacks() function and reset using
 * sysfio_reset_callbacks().
 */
static sysfio_fopen_cb s_fopen_cb = NULL;
static sysfio_fclose_cb s_fclose_cb = NULL;
static sysfio_fgets_cb s_fgets_cb = NULL;
static sysfio_rewind_cb s_rewind_cb = NULL;


FILE* sysfio_fopen(const char* restrict filename, const char* restrict mode)
{
  if (s_fopen_cb) {
    return s_fopen_cb(filename, mode);
  }
  return fopen(filename, mode);
}

int sysfio_fclose(FILE* stream)
{
  if (s_fclose_cb) {
    return s_fclose_cb(stream);
  }
  return fclose(stream);
}

char* sysfio_fgets(char *restrict str, int count, FILE *restrict stream)
{
  if (s_fgets_cb) {
    return s_fgets_cb(str, count, stream);
  }
  return fgets(str, count, stream);
}

void sysfio_rewind(FILE* stream)
{
  if (s_rewind_cb) {
    s_rewind_cb(stream);
  }
  else {
    rewind(stream);
  }
}

void sysfio_set_callbacks(sysfio_fopen_cb fopen_cb,
                          sysfio_fclose_cb fclose_cb,
                          sysfio_fgets_cb fgets_cb,
                          sysfio_rewind_cb rewind_cb)
{
  s_fopen_cb = fopen_cb;
  s_fclose_cb = fclose_cb;
  s_fgets_cb = fgets_cb;
  s_rewind_cb = rewind_cb;
}

void sysfio_reset_callbacks()
{
  s_fopen_cb = NULL;
  s_fclose_cb = NULL;
  s_fgets_cb = NULL;
  s_rewind_cb = NULL;
}

