#EXTRA_DIST = cps.so

#Script files
bin_SCRIPTS = scripts/lib/yang_tools/py/*.py scripts/lib/yang_tools/*.sh \
              scripts/lib/*.py scripts/bin/*

#All exported headers
nobase_include_HEADERS = sonic/cps_api_errors.h sonic/cps_api_event_init.h \
    sonic/cps_api_events.h sonic/cps_api_key.h sonic/cps_api_object_attr.h \
    sonic/cps_api_object_category.h sonic/cps_api_object.h sonic/cps_api_object_key.h \
    sonic/cps_api_object_tools.h sonic/cps_api_operation.h sonic/cps_api_operation_stats.h \
    sonic/cps_api_operation_tools.h sonic/cps_api_service.h sonic/cps_class_map.h \
    sonic/cps_key_internals.h

lib_LTLIBRARIES = libsonic_object_library.la libsonic_cps_class_map.la

libsonic_object_library_la_SOURCES = src/cps_api_base_event_adapter.cpp \
    src/cps_api_client_ipc.cpp src/cps_api_events.cpp src/cps_api_key.cpp \
    src/cps_api_object_attr.cpp src/cps_api_object.cpp src/cps_api_object_tools.cpp \
    src/cps_api_operation_common.cpp src/cps_api_operation_service.cpp \
    src/cps_api_operation_tools.cpp src/cps_api_utils.cpp src/cps_dictionary.cpp \
    src/cps_dictionary_loader.cpp src/cps_ns.cpp

libsonic_object_library_la_LDFLAGS = -lxml2 -lpthread -lrt \
    -lsonic_logging -lsonic_common -lstdc++
libsonic_object_library_la_CPPFLAGS = -std=c++11 -I$(top_srcdir)/sonic \
    -I$(includedir)/sonic 

libsonic_cps_class_map_la_SOURCES = src/cps_class_map_query.cpp
libsonic_cps_class_map_la_CPPFLAGS = -std=c++11 -I$(top_srcdir)/sonic \
                                     -I$(includedir)/sonic
libsonic_cps_class_map_la_LDFLAGS = -lpthread -lsonic_common
libsonic_cps_class_map_la_LIBADD=libsonic_object_library.la

#The CLI used to change levels at runtime
bin_PROGRAMS = sonic_object_service

sonic_object_service_SOURCES=src/cps_api_service.cpp
sonic_object_service_CPPFLAGS=-I$(top_srcdir)/sonic -I$(includedir)/sonic
sonic_object_service_LDFLAGS= -lpthread -lsonic_logging 
sonic_object_service_LDADD=libsonic_object_library.la

#cps module
pyexec_LTLIBRARIES = cps.la
cps_la_LDFLAGS = -module -avoid-version -export-symbols-regex initpoppler -lsonic_common -lsonic_logging -lpython2.7 -lstdc++
cps_la_SOURCES = src/python_extension/cps_api_python.cpp \
    src/python_extension/cps_api_python_operation.cpp \
  src/python_extension/cps_api_python_utils.cpp \
  src/python_extension/cps_api_python_events.cpp 
cps_la_CXXFLAGS=-I$(top_srcdir)/src/python_extension -I$(top_srcdir)/sonic -I$(includedir)/sonic -I$(includedir)/python2.7  -std=c++11 
cps_la_LIBADD=libsonic_object_library.la

systemdconfdir=/lib/systemd/system
systemdconf_DATA = $(top_srcdir)/scripts/init/sonic-object-library.service

