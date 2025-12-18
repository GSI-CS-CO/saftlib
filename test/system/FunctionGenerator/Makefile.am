
AM_CPPFLAGS = -Wall -g  $(SIGCPP_CFLAGS) $(EB_CFLAGS) -I $(top_srcdir)/ -I $(top_srcdir)/saftbus -I $(top_srcdir)/src -I $(top_builddir)/src -I $(top_srcdir)/src/interfaces -I $(top_builddir)/src/interfaces -DDATADIR='"$(datadir)/saftlib"'

bin_PROGRAMS = test-fg test-mfg test-fg-performance test-mfg-duration

test_fg_SOURCES = test-fg.cpp CommonHelpers.cpp
test_fg_LDADD   =   $(SIGCPP_LIBS) $(top_builddir)/libsaftbus.la $(top_builddir)/libsaft-proxy.la $(top_builddir)/libfg-firmware-proxy.la -lpthread -ldl #-lltdl

test_mfg_SOURCES = test-mfg.cpp CommonHelpers.cpp
test_mfg_LDADD   =   $(SIGCPP_LIBS) $(top_builddir)/libsaftbus.la $(top_builddir)/libsaft-proxy.la $(top_builddir)/libfg-firmware-proxy.la -lpthread -ldl #-lltdl

test_fg_performance_SOURCES = test-fg-performance.cpp CommonHelpers.cpp FunctionGeneratorSharedMemory.cpp
test_fg_performance_LDADD   =   $(SIGCPP_LIBS) $(top_builddir)/libsaftbus.la $(top_builddir)/libsaft-proxy.la $(top_builddir)/libfg-firmware-proxy.la -lpthread -ldl #-lltdl

test_mfg_duration_SOURCES   = test-mfg-duration.cpp CommonHelpers.cpp
test_mfg_durationLDADD      = $(SIGCPP_LIBS) $(top_builddir)/libsaftbus.la $(top_builddir)/libsaft-proxy.la $(top_builddir)/libfg-firmware-proxy.la -lpthread -ldl #-lltdl
