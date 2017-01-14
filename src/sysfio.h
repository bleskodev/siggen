/* sysfio.h
 * header file for system file i/o functions. All system file reading goes
 * through this interface in order to allow mocking file i/o when unit testing.
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
#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

FILE* sysfio_fopen(const char* filename, const char* mode);
int sysfio_fclose(FILE* stream);
char* sysfio_fgets(char *str, int count, FILE *stream);
void sysfio_rewind(FILE* stream);


typedef FILE* (*sysfio_fopen_cb)(const char*, const char*);
typedef int (*sysfio_fclose_cb)(FILE*);
typedef char* (*sysfio_fgets_cb)(char*, int, FILE*);
typedef void (*sysfio_rewind_cb)(FILE*);

void sysfio_set_callbacks(sysfio_fopen_cb fopen_cb,
                          sysfio_fclose_cb fclose_cb,
                          sysfio_fgets_cb fgets_cb,
                          sysfio_rewind_cb rewind_cb);
void sysfio_reset_callbacks();

#ifdef __cplusplus
} /* extern "C" */
#endif
