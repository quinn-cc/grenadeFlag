lib_LTLIBRARIES = grenadeFlag.la

grenadeFlag_la_SOURCES = grenadeFlag.cpp
grenadeFlag_la_CPPFLAGS= -I$(top_srcdir)/include -I$(top_srcdir)/plugins/plugin_utils
grenadeFlag_la_LDFLAGS = -module -avoid-version -shared
grenadeFlag_la_LIBADD = $(top_builddir)/plugins/plugin_utils/libplugin_utils.la

AM_CPPFLAGS = $(CONF_CPPFLAGS)
AM_CFLAGS = $(CONF_CFLAGS)
AM_CXXFLAGS = $(CONF_CXXFLAGS)

EXTRA_DIST = \
	README.grenadeFlag.txt \
	grenadeFlag.sln \
	grenadeFlag.vcxproj	\
	grenadeFlag.vcxproj.filters

MAINTAINERCLEANFILES =	\
	Makefile.in
