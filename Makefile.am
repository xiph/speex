## Process this file with automake to produce Makefile.in. -*-Makefile-*-

# To disable automatic dependency tracking if using other tools than
# gcc and gmake, add the option 'no-dependencies'
AUTOMAKE_OPTIONS = 1.8
ACLOCAL_AMFLAGS = -I m4

m4datadir = $(datadir)/aclocal
m4data_DATA = speex.m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = speex.pc

MESON_FILES = meson.build meson_options.txt doc/meson.build src/meson.build libspeex/meson.build libspeex/speex.def include/speex/meson.build

EXTRA_DIST = Speex.spec Speex.spec.in Speex.kdevelop speex.m4 speex.pc.in README.blackfin README.symbian README.TI-DSP $(MESON_FILES)

#Fools KDevelop into including all files
SUBDIRS = libspeex include doc win32 symbian ti

if BUILD_BINARIES
SUBDIRS += src
endif

DIST_SUBDIRS = libspeex include src doc win32 symbian ti

rpm: dist
	rpmbuild -ta ${PACKAGE}-${VERSION}.tar.gz
