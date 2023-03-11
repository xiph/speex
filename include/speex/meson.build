install_headers([
  'speex.h',
  'speex_bits.h',
  'speex_callbacks.h',
  'speex_header.h',
  'speex_stereo.h',
  'speex_types.h',
], subdir: 'speex')

# speex_config_types.h
ctypes_cdata = configuration_data()
ctypes_cdata.set('INCLUDE_STDINT', '#include <stdint.h>')
ctypes_cdata.set('SIZE16', 'int16_t')
ctypes_cdata.set('USIZE16', 'uint16_t')
ctypes_cdata.set('SIZE32', 'int32_t')
ctypes_cdata.set('USIZE32', 'uint32_t')

speex_config_types_h = configure_file(input: 'speex_config_types.h.in',
  output: 'speex_config_types.h',
  configuration: ctypes_cdata,
  install_dir: get_option('includedir') / 'speex',
  install: true)
