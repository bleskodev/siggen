/* configsubs.h
 * functions for dealing with configuration files and returning config values
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

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  init_conf_files(local,home,global,verb)    opens upto 3 configuration
 *     files - local, home and global - that may or may not exist.
 *        local   is typically just a file in the local directory
 *        home    has the contents of the HOME env variable prepended
 *        global  is typically a system wide readable global conf. file
 *     This function must be called before any other. If called a second 
 *     time all previous conf. files are closed automatically.
 *     The int verb sets the verbose/debug/level. 
 *      0 = no output, 1 = say what config files are opened/closed
 *      2 = also say where config values are found/matched.
 *     Returns 0 is all ok else returns an error code.
 */
int init_conf_files(const char *local, const char *home, const char *global, int verb);

/*
 *  close_conf_files()     simply closes down any open conf. files
 *     Not really necessary, but could be useful.
 */
void close_conf_files();

/*
 *  get_conf_value(sys,name,def)   try and get the config. value
 *     for parameter 'name', system 'sys', from the config. files.
 *     config files are searched in order home, local, global.
 *     If no specific value under system 'sys' found then any global
 *     setting for 'name' is returned. Finally if nothing is found the 
 *     'def' is returned.
 *     All values are strings, as is the returned value.
 */
const char *get_conf_value(const char *sys, const char *name, const char *def);

#ifdef __cplusplus
} /* extern "C" */
#endif
