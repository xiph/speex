# Sources for compilation in the library
speex_sources = files([
  'cb_search.c',
  'exc_10_32_table.c',
  'exc_8_128_table.c',
  'filters.c',
  'gain_table.c',
  'hexc_table.c',
  'high_lsp_tables.c',
  'lsp.c',
  'ltp.c',
  'speex.c',
  'stereo.c',
  'vbr.c',
  'vq.c',
  'bits.c',
  'exc_10_16_table.c',
  'exc_20_32_table.c',
  'exc_5_256_table.c',
  'exc_5_64_table.c',
  'gain_table_lbr.c',
  'hexc_10_32_table.c',
  'lpc.c',
  'lsp_tables_nb.c',
  'modes.c',
  'modes_wb.c',
  'nb_celp.c',
  'quant_lsp.c',
  'sb_celp.c',
  'speex_callbacks.c',
  'speex_header.c',
  'window.c',
])

if fft_used == 'kiss'
  # Use KISS Fast Fourier Transform
  speex_sources += files(['kiss_fft.c', 'kiss_fftr.c'])
  cdata.set('USE_KISS_FFT', true)
elif fft_used == 'smallft'
  # Use FFT from OggVorbis
  speex_sources += files(['smallft.c'])
  cdata.set('USE_SMALLFT', true)
endif

if opt_vorbis_psy
  speex_sources += files(['vorbis_psy.c'])
  cdata.set('VORBIS_PSYCHO', true)
endif

libm = cc.find_library('m', required: false)

libspeex = library('speex', speex_sources, speex_config_types_h,
  include_directories: [config_inc, speex_incs],
  dependencies: [fft_dep, libm],
  gnu_symbol_visibility: 'hidden',
  vs_module_defs: 'speex.def',
  version: libversion,
  install: true)

speex_dep = declare_dependency(link_with: libspeex,
  include_directories: speex_incs)

meson.override_dependency('speex', speex_dep)

# pkg-config file
pkg = import('pkgconfig')
pkg.generate(libspeex,
  name: 'speex',
  description: 'Speex is an audio codec tuned for speech',
  version: meson.project_version())

if not get_option('test-binaries').disabled()
  foreach bin : ['testenc', 'testenc_wb', 'testenc_uwb']
    executable(bin, '@0@.c'.format(bin),
      include_directories: config_inc,
      dependencies: [speex_dep, libm],
      install: false)
  endforeach
endif
