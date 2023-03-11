project('speex', 'c', version: '1.2.1',
  meson_version: '>= 0.59')

libversion = '1.5.2'

speex_version = meson.project_version()
version_arr = speex_version.split('.')
speex_version_major = version_arr[0].to_int()
speex_version_minor = version_arr[1].to_int()

# extra version might be "-svn" or ".2" or empty
if version_arr.length() == 4
  speex_version_micro = version_arr[2].to_int()
  speex_version_extra = '.' + version_arr[3]
elif version_arr[2].contains('-')
  speex_version_micro = version_arr[2].split('-')[0].to_int()
  speex_version_extra = '-' + version_arr[2].split('-')[1]
else
  speex_version_micro = version_arr[2].to_int()
  speex_version_extra = ''
endif

host_system = host_machine.system()
host_cpu_family = host_machine.cpu_family()

if host_machine.endian() == 'big'
  cdata.set('WORDS_BIGENDIAN', true)
endif

cc = meson.get_compiler('c')

config_inc = include_directories('.')

speex_incs = include_directories('include', 'include/speex')

cdata = configuration_data()

add_project_arguments('-DHAVE_CONFIG_H=1', language: 'c')

cdata.set_quoted('SPEEX_VERSION', speex_version,
                 description: 'Complete version string')

cdata.set('SPEEX_MAJOR_VERSION', speex_version_major,
          description: 'Version major')
cdata.set('SPEEX_MINOR_VERSION', speex_version_minor,
          description: 'Version minor')
cdata.set('SPEEX_MICRO_VERSION', speex_version_micro,
          description: 'Version micro')
cdata.set_quoted('SPEEX_EXTRA_VERSION', speex_version_extra,
          description: 'Version extra (version suffix string)')

# Test whether the compiler supports the 'long double' type.
have_var_arrays = cc.compiles('void func (int n) {int arr[n]; }',
                              name: 'C99 variable-size arrays')

cdata.set('VAR_ARRAYS', have_var_arrays,
          description: 'Use C99 variable-size arrays')

has_sse = false
if host_cpu_family in ['x86', 'x86_64'] and get_option('sse').allowed()
  intrin_check = '''#include <xmmintrin.h>
                    __m128 testfunc(float *a, float *b) {
                      return _mm_add_ps(_mm_loadu_ps(a), _mm_loadu_ps(b));
                    }
                    int main (int argc, char ** argv) {
                      float a = 2.0, b = 99.0;
                      testfunc(&a, &b);
                      return 0;
                    }
                 '''
  # Intrinsics arguments are not available with MSVC-like compilers
  intrin_args = cc.get_argument_syntax() == 'msvc' ? [] : ['-msse']
  if cc.links(intrin_check, name: 'Compiler supports SEE intrinsics', args: intrin_args)
    has_sse = true
  endif

  # Mimicing the logic in win32/config.h
  if cc.get_id() == 'msvc' and host_cpu_family == 'x86'
    m_ix86_fp = cc.get_define('_M_IX86_FP')
    if m_ix86_fp == '' or m_ix86_fp.to_int() < 1
      has_sse = false
    endif
  endif
endif

# Visual Studio supports alloca(), but it always aligns variables to 16-bit
# boundary, while SSE needs 128-bit alignment. So we disable alloca() when
# SSE is enabled (from win32/config.h)
allow_alloca = not (cc.get_id() == 'msvc' and has_sse)

use_alloca = false

if allow_alloca
  alloca_check = 'int main (int n) { char * array = alloca(n); }'

  if cc.has_header('alloca.h')
    cdata.set('HAVE_ALLOCA_H', true)

    if cc.links('#include <alloca.h>\n' + alloca_check, name: 'alloca (alloca.h)')
      use_alloca = true
    endif
  endif

  if cc.links('#include <malloc.h>\n#include <stdlib.h>\n' + alloca_check, name: 'alloca (malloc.h)')
    use_alloca = true
  endif
endif

cdata.set('USE_ALLOCA', use_alloca)

if cc.get_id() != 'msvc' and cc.has_argument('-fvisibility=hidden')
  add_project_arguments('-fvisibility=hidden', language: 'c')
  cdata.set('EXPORT', '__attribute__((visibility("default")))')
else
  cdata.set('EXPORT', '/* */')
endif

# FFT implementation

fft_dep = []

use_fixed_point = get_option('fixed-point').enabled()

if use_fixed_point
  default_fft = 'kiss'
  has_sse = false
  cdata.set('FIXED_POINT', true)
else
  default_fft = 'smallft'
  cdata.set('FLOATING_POINT', true)
endif

fft_opt = get_option('fft')

if fft_opt == 'auto'
  fft_used = default_fft
elif fft_opt == 'kiss'
  fft_used = 'kiss'
elif fft_opt == 'smallft'
  fft_used = 'smallft'
elif fft_opt == 'gpl-fftw3'
  fft_used = 'gpl-fftw3'
  fft_dep = dependency('fftw3f', required: true)
elif fft_opt == 'proprietary-intel-mkl'
  error('FIXME: add support for proprietary-intel-mkl FFT')
else
  error('Unexpected FFT option', fft_opt)
endif

if not has_sse and get_option('sse').enabled()
  error('No SSE support, but SSE support required via options')
endif

cdata.set('_USE_SSE', has_sse)

opt_arm4_asm = get_option('arm4-asm').enabled()
opt_arm5e_asm = get_option('arm5e-asm').enabled()
opt_blackfin_asm = get_option('blackfin-asm').enabled()
# opt_ti_c55x = get_option('ti-c55x').enabled()

cdata.set('ARM4_ASM', opt_arm4_asm)
cdata.set('ARM5E_ASM', opt_arm5e_asm)
cdata.set('BFIN_ASM', opt_blackfin_asm)
# cdata.set('TI_C55X', opt_ti_c55x) # has_char16=yes

opt_disable_vbr = get_option('vbr').disabled()
opt_disable_float_api = get_option('float-api').disabled()
opt_fixed_point_debug = get_option('fixed-point-debug').enabled()
opt_valgrind = get_option('valgrind').enabled()
opt_vorbis_psy = get_option('vorbis-psy').enabled()

cdata.set('DISABLE_VBR', opt_disable_vbr)
cdata.set('DISABLE_FLOAT_API', opt_disable_float_api)
cdata.set('FIXED_POINT_DEBUG', opt_fixed_point_debug)
cdata.set('ENABLE_VALGRIND', opt_valgrind)

subdir('doc')
subdir('include/speex')
subdir('libspeex')

if not get_option('tools').disabled()
  subdir('src')
endif

# config.h
configure_file(output: 'config.h', configuration: cdata)

# summary

building_speex_tools = get_variable('building_speex_tools', false)

opt_disable_vbr = get_option('vbr').disabled()
opt_disable_float_api = get_option('float-api').disabled()
opt_fixed_point_debug = get_option('fixed-point-debug').enabled()
opt_valgrind = get_option('valgrind').enabled()

asms = {
  'has_sse': 'SSE',
  'opt_arm4_asm': 'ARM4 assembly',
  'opt_arm5e_asm': 'ARMv5e assembly',
  'opt_blackfin_asm': 'Blackfin assembly',
  'opt_ti_c55x': 'TI C55X DSP',
}
optimizations = []
foreach var, desc : asms
  if get_variable(var, false)
    optimizations += [desc]
  endif
endforeach

summary(
  {
    'Assembly optimizations': optimizations.length() > 0 ? optimizations : false,
    'Floating point support': not use_fixed_point,
    'Fixed point debugging': use_fixed_point ? opt_fixed_point_debug : 'N/A',
    'Float API': not opt_disable_float_api,
    'FFT': fft_used,
    'VBR + VAD support': not opt_disable_vbr,
    'Vorbis psy model': opt_vorbis_psy,
    'Tools': building_speex_tools,
  },
  bool_yn: true,
  list_sep: ', ',
)
