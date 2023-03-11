have_getopt_h = cc.has_header('getopt.h')
cdata.set('HAVE_GETOPT_H', have_getopt_h)

winmm_lib = []
if host_system == 'windows' and cc.get_id() == 'gcc'
  winmm_lib = cc.find_library('winmm', required: true)
endif

ogg_dep = dependency('ogg', required: get_option('tools'))

if not ogg_dep.found()
  message('Not building command line tools, libogg not found.')
  subdir_done()
endif

speexdsp_dep = dependency('speexdsp', required: false)

cdata.set('USE_SPEEXDSP', speexdsp_dep.found())

cdata.set('HAVE_SYS_SOUNDCARD_H', cc.has_header('sys/soundcard.h'))
cdata.set('HAVE_SYS_AUDIOIO_H', cc.has_header('sys/audioio.h'))

gnugetopt_lib = []

if have_getopt_h
  if cc.has_function('getopt_long', prefix: '#include <getopt.h>')
    cdata.set('HAVE_GETOPT_LONG', true)
  else
    # FreeBSD has a gnugetopt library
    gnugetopt_lib = cc.find_library('gnugetopt', required: false)
    if gnugetopt_lib.found()
      if cc.has_function('getopt_long', prefix: '#include <getopt.h>', dependencies: gnugetopt_lib)
        cdata.set('HAVE_GETOPT_LONG', true)
      endif
    endif
  endif
endif

getopt_sources = []
if not cdata.get('HAVE_GETOPT_LONG', false)
  # Use the GNU replacement.
  getopt_sources = ['getopt.c', 'getopt1.c']
endif

executable('speexdec', 'speexdec.c', 'wav_io.c', getopt_sources,
  include_directories: config_inc,
  dependencies: [speex_dep, ogg_dep, fft_dep, winmm_lib],
  install: true)

executable('speexenc', 'speexenc.c', 'wav_io.c', 'skeleton.c', getopt_sources,
  include_directories: config_inc,
  dependencies: [speex_dep, ogg_dep, speexdsp_dep],
  install: true)

install_man('speexenc.1', 'speexdec.1')

building_speex_tools = true