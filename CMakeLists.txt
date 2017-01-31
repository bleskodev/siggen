cmake_minimum_required(VERSION 2.8)
project(siggen)

find_package(qibuild)
#qi_sanitize_compile_flags()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(".")

list(APPEND CMAKE_FIND_ROOT_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

# common lib
SET(common_src src/configsubs.c src/sysfio.c src/misc.c src/generator.c)
SET(common_hdr src/configsubs.h src/sysfio.h src/misc.h src/generator.h)

# simple command line programs...
SET(tones_src src/tones.c src/tonesgen.c src/wavfile.c src/wavsubs.c src/DAC.c)
SET(sgen_src src/sgen.c src/wavfile.c src/wavsubs.c src/DAC.c)
SET(swgen_src src/swgen.c src/wavfile.c src/wavsubs.c src/DAC.c)
SET(smix_src src/smix.c src/mixer.c)
SET(soundinfo_src src/soundinfo.c)

# curses based programs...
SET(fsynth_src src/fsynth.c src/fsynscr.c src/scrsubs.c src/DAC.c src/scfio.c)
SET(siggen_src src/siggen.c src/sigscr.c src/scrsubs.c src/DAC.c src/scfio.c)
SET(sweepgen_src src/sweepgen.c src/sweepscr.c src/scrsubs.c src/DAC.c src/scfio.c)

# tests
SET(test_common_src
  test/test_configsubs.cpp
  test/test_sysfio.cpp
  test/test_misc.cpp
  test/test_generator.cpp
  test/sysfio_mock.cpp
)

# man pages...
SET(man1_pages
  doc/man/fsynth.1 doc/man/sgen.1 doc/man/siggen.1 doc/man/smix.1
  doc/man/soundinfo.1 doc/man/sweepgen.1 doc/man/swgen.1 doc/man/tones.1)
SET(man5_pages doc/man/siggen.conf.5)

# doc pages...
SET(doc_output_dir ${CMAKE_CURRENT_BINARY_DIR}/sdk/share/doc)
SET(tones_doc_in ${CMAKE_CURRENT_SOURCE_DIR}/doc/man/tones.1)
SET(tones_doc ${doc_output_dir}/tones.txt)
SET(sgen_doc_in ${CMAKE_CURRENT_SOURCE_DIR}/doc/man/sgen.1)
SET(sgen_doc ${doc_output_dir}/sgen.txt)
SET(swgen_doc_in ${CMAKE_CURRENT_SOURCE_DIR}/doc/man/swgen.1)
SET(swgen_doc ${doc_output_dir}/swgen.txt)
SET(smix_doc_in ${CMAKE_CURRENT_SOURCE_DIR}/doc/man/smix.1)
SET(smix_doc ${doc_output_dir}/smix.txt)
SET(soundinfo_doc_in ${CMAKE_CURRENT_SOURCE_DIR}/doc/man/soundinfo.1)
SET(soundinfo_doc ${doc_output_dir}/soundinfo.txt)
SET(fsynth_doc_in ${CMAKE_CURRENT_SOURCE_DIR}/doc/man/fsynth.1)
SET(fsynth_doc ${doc_output_dir}/fsynth.txt)
SET(siggen_doc_in ${CMAKE_CURRENT_SOURCE_DIR}/doc/man/siggen.1)
SET(siggen_doc ${doc_output_dir}/siggen.txt)
SET(siggen_conf_doc_in ${CMAKE_CURRENT_SOURCE_DIR}/doc/man/siggen.conf.5)
SET(siggen_conf_doc ${doc_output_dir}/siggen.conf.txt)
SET(sweepgen_doc_in ${CMAKE_CURRENT_SOURCE_DIR}/doc/man/sweepgen.1)
SET(sweepgen_doc ${doc_output_dir}/sweepgen.txt)

# export build commands for YouCompleteMe
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# targets...
# common lib
qi_create_lib(siggen_common ${common_src} ${common_hdr} STATIC)
qi_stage_lib(siggen_common)

# tones
qi_generate_src(${tones_doc} COMMAND nroff -man ${tones_doc_in} | col -b -x > ${tones_doc})
qi_create_bin(tones ${tones_src} ${tones_doc})
qi_use_lib(tones siggen_common m)

# sgen
qi_generate_src(${sgen_doc} COMMAND nroff -man ${sgen_doc_in} | col -b -x > ${sgen_doc})
qi_create_bin(sgen ${sgen_src} ${sgen_doc})
qi_use_lib(sgen siggen_common m)

# swgen
qi_generate_src(${swgen_doc} COMMAND nroff -man ${swgen_doc_in} | col -b -x > ${swgen_doc})
qi_create_bin(swgen ${swgen_src} ${swgen_doc})
qi_use_lib(swgen siggen_common m)

# smix
qi_generate_src(${smix_doc} COMMAND nroff -man ${smix_doc_in} | col -b -x > ${smix_doc})
qi_create_bin(smix ${smix_src} ${smix_doc})
qi_use_lib(smix siggen_common)

# soundinfo
qi_generate_src(${soundinfo_doc} COMMAND nroff -man ${soundinfo_doc_in} | col -b -x > ${soundinfo_doc})
qi_create_bin(soundinfo ${soundinfo_src} ${soundinfo_doc})

# siggen
qi_generate_src(${siggen_doc} COMMAND nroff -man ${siggen_doc_in} | col -b -x > ${siggen_doc})
qi_generate_src(${siggen_conf_doc} COMMAND nroff -man ${siggen_conf_doc_in} | col -b -x > ${siggen_conf_doc})
qi_create_bin(siggen ${siggen_src} ${siggen_doc} ${siggen_conf_doc})
qi_use_lib(siggen siggen_common ncurses m)

# sweepgen
qi_generate_src(${sweepgen_doc} COMMAND nroff -man ${sweepgen_doc_in} | col -b -x > ${sweepgen_doc})
qi_create_bin(sweepgen ${sweepgen_src} ${sweepgen_doc})
qi_use_lib(sweepgen siggen_common ncurses m)

# fsynth
qi_generate_src(${fsynth_doc} COMMAND nroff -man ${fsynth_doc_in} | col -b -x > ${fsynth_doc})
qi_create_bin(fsynth ${fsynth_src} ${fsynth_doc})
qi_use_lib(fsynth siggen_common ncurses m)

# man1
qi_install_data(${man1_pages} SUBFOLDER man/man1)
qi_install_data(${man5_pages} SUBFOLDER man/man5)

# tests
qi_create_gmock(test_common ${test_common_src})
qi_use_lib(test_common siggen_common)

