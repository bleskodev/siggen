cmake_minimum_required(VERSION 2.8)
project(siggen)

find_package(qibuild)
#qi_sanitize_compile_flags()

list(APPEND CMAKE_FIND_ROOT_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
 
# simple command line programs...
SET(tones_src tones.c tonesgen.c generator.c misc.c wavfile.c wavsubs.c DAC.c configsubs.c)
SET(sgen_src sgen.c generator.c misc.c wavfile.c wavsubs.c DAC.c configsubs.c)
SET(swgen_src swgen.c generator.c misc.c wavfile.c wavsubs.c DAC.c configsubs.c)
SET(smix_src smix.c mixer.c configsubs.c)
SET(soundinfo_src soundinfo.c)
#
# curses based programs...
SET(fsynth_src fsynth.c fsynscr.c scrsubs.c generator.c misc.c DAC.c scfio.c configsubs.c)
SET(siggen_src siggen.c sigscr.c scrsubs.c generator.c misc.c DAC.c scfio.c configsubs.c)
SET(sweepgen_src sweepgen.c sweepscr.c scrsubs.c generator.c misc.c DAC.c scfio.c configsubs.c)

#
# man pages...
SET(man1_pages fsynth.1 sgen.1 siggen.1 smix.1 soundinfo.1 sweepgen.1 swgen.1 tones.1)
SET(man5_pages siggen.conf.5)

#
# doc pages...
SET(doc_output_dir ${CMAKE_CURRENT_BINARY_DIR}/sdk/share/doc)
SET(tones_doc_in ${CMAKE_CURRENT_SOURCE_DIR}/tones.1)
SET(tones_doc ${doc_output_dir}/tones.txt)
SET(sgen_doc_in ${CMAKE_CURRENT_SOURCE_DIR}/sgen.1)
SET(sgen_doc ${doc_output_dir}/sgen.txt)
SET(swgen_doc_in ${CMAKE_CURRENT_SOURCE_DIR}/swgen.1)
SET(swgen_doc ${doc_output_dir}/swgen.txt)
SET(smix_doc_in ${CMAKE_CURRENT_SOURCE_DIR}/smix.1)
SET(smix_doc ${doc_output_dir}/smix.txt)
SET(soundinfo_doc_in ${CMAKE_CURRENT_SOURCE_DIR}/soundinfo.1)
SET(soundinfo_doc ${doc_output_dir}/soundinfo.txt)
SET(fsynth_doc_in ${CMAKE_CURRENT_SOURCE_DIR}/fsynth.1)
SET(fsynth_doc ${doc_output_dir}/fsynth.txt)
SET(siggen_doc_in ${CMAKE_CURRENT_SOURCE_DIR}/siggen.1)
SET(siggen_doc ${doc_output_dir}/siggen.txt)
SET(siggen_conf_doc_in ${CMAKE_CURRENT_SOURCE_DIR}/siggen.conf.5)
SET(siggen_conf_doc ${doc_output_dir}/siggen.conf.txt)
SET(sweepgen_doc_in ${CMAKE_CURRENT_SOURCE_DIR}/sweepgen.1)
SET(sweepgen_doc ${doc_output_dir}/sweepgen.txt)

# export build commands for YouCompleteMe
set(CMAKE_EXPORT_COMPILE_FLAGS 1)

# targets...
# tones
qi_generate_src(${tones_doc} COMMAND nroff -man ${tones_doc_in} | col -b -x > ${tones_doc})
qi_create_bin(tones ${tones_src} ${tones_doc})
qi_use_lib(tones m)

# sgen
qi_generate_src(${sgen_doc} COMMAND nroff -man ${sgen_doc_in} | col -b -x > ${sgen_doc})
qi_create_bin(sgen ${sgen_src} ${sgen_doc})
qi_use_lib(sgen m)

# swgen
qi_generate_src(${swgen_doc} COMMAND nroff -man ${swgen_doc_in} | col -b -x > ${swgen_doc})
qi_create_bin(swgen ${swgen_src} ${swgen_doc})
qi_use_lib(swgen m)

# smix
qi_generate_src(${smix_doc} COMMAND nroff -man ${smix_doc_in} | col -b -x > ${smix_doc})
qi_create_bin(smix ${smix_src} ${smix_doc})

# soundinfo
qi_generate_src(${soundinfo_doc} COMMAND nroff -man ${soundinfo_doc_in} | col -b -x > ${soundinfo_doc})
qi_create_bin(soundinfo ${soundinfo_src} ${soundinfo_doc})

# siggen
qi_generate_src(${siggen_doc} COMMAND nroff -man ${siggen_doc_in} | col -b -x > ${siggen_doc})
qi_generate_src(${siggen_conf_doc} COMMAND nroff -man ${siggen_conf_doc_in} | col -b -x > ${siggen_conf_doc})
qi_create_bin(siggen ${siggen_src} ${siggen_doc} ${siggen_conf_doc})
qi_use_lib(siggen ncurses m)

# sweepgen
qi_generate_src(${sweepgen_doc} COMMAND nroff -man ${sweepgen_doc_in} | col -b -x > ${sweepgen_doc})
qi_create_bin(sweepgen ${sweepgen_src} ${sweepgen_doc})
qi_use_lib(sweepgen ncurses m)

# fsynth
qi_generate_src(${fsynth_doc} COMMAND nroff -man ${fsynth_doc_in} | col -b -x > ${fsynth_doc})
qi_create_bin(fsynth ${fsynth_src} ${fsynth_doc})
qi_use_lib(fsynth ncurses m)

# man1
qi_install_data(${man1_pages} SUBFOLDER man/man1)
qi_install_data(${man5_pages} SUBFOLDER man/man5)

