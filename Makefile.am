ACLOCAL_AMFLAGS = -I m4
AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = $(FFTW_CFLAGS)
AM_LDFLAGS = $(FFTW_LIBS)

lib_LIBRARIES = libfourier.a
libfourier_a_SOURCES = fourierfilter.cpp

TESTS = testfourier
bin_PROGRAMS = $(TESTS)

testfourier_SOURCES = testmain.cpp test-fourier.cpp yaffut.h
testfourier_LDADD = libfourier.a -lfftw3f

#pkgconfigdir = $(libdir)/pkgconfig
#pkgconfig_DATA = dyplofft.pc
