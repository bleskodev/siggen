cmake_minimum_required(VERSION 2.8)
project(siggen)

find_package(qibuild)
#qi_sanitize_compile_flags()

list(APPEND CMAKE_FIND_ROOT_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
 
# simple command line programs......
SET(tones_src tones.c tonesgen.c generator.c misc.c wavfile.c wavsubs.c DAC.c configsubs.c)
SET(sgen_src sgen.c generator.c misc.c wavfile.c wavsubs.c DAC.c configsubs.c)
SET(swgen_src swgen.c generator.c misc.c wavfile.c wavsubs.c DAC.c configsubs.c)
SET(smix_src smix.c mixer.c configsubs.c)
SET(soundinfo_src soundinfo.c)
#
# curses based programs......
SET(fsynth_src fsynth.c fsynscr.c scrsubs.c generator.c misc.c DAC.c scfio.c configsubs.c)
SET(siggen_src siggen.c sigscr.c scrsubs.c generator.c misc.c DAC.c scfio.c configsubs.c)
SET(sweepgen_src sweepgen.c sweepscr.c scrsubs.c generator.c misc.c DAC.c scfio.c configsubs.c)

# export build commands for YouCompleteMe
set(CMAKE_EXPORT_COMPILE_FLAGS 1)

# targets...
# tones
qi_create_bin(tones ${tones_src})
qi_use_lib(tones m)

# sgen
qi_create_bin(sgen ${sgen_src})
qi_use_lib(sgen m)

# swgen
qi_create_bin(swgen ${swgen_src})
qi_use_lib(swgen m)

# smix
qi_create_bin(smix ${smix_src})

# soundinfo
qi_create_bin(soundinfo ${soundinfo_src})

# siggen
qi_create_bin(siggen ${siggen_src})
qi_use_lib(siggen ncurses m)

# sweepgen
qi_create_bin(sweepgen ${sweepgen_src})
qi_use_lib(sweepgen ncurses m)

# fsynth
qi_create_bin(fsynth ${fsynth_src})
qi_use_lib(fsynth ncurses m)
