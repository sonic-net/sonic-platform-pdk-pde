EXTRA_DIST = cps.so

#Script files
bin_SCRIPTS = scripts/lib/yang_tools/py/*.py scripts/lib/yang_tools/*.sh \
              scripts/lib/*.py scripts/bin/*
scriptsdir = /etc/init
scripts_DATA = scripts/init/cps_api_service.conf

#All exported headers
nobase_include_HEADERS = sonic/cps_api_errors.h sonic/cps_api_event_init.h \
    sonic/cps_api_events.h sonic/cps_api_key.h sonic/cps_api_object_attr.h \
    sonic/cps_api_object_category.h sonic/cps_api_object.h sonic/cps_api_object_key.h \
    sonic/cps_api_object_tools.h sonic/cps_api_operation.h sonic/cps_api_operation_stats.h \
    sonic/cps_api_operation_tools.h sonic/cps_api_service.h sonic/cps_class_map.h \
    sonic/cps_key_internals.h

lib_LTLIBRARIES = libsonic_cps_api.la libsonic_cps_class_map.la

libsonic_cps_api_la_SOURCES = src/cps_api_base_event_adapter.cpp \
    src/cps_api_client_ipc.cpp src/cps_api_events.cpp src/cps_api_key.cpp \
    src/cps_api_object_attr.cpp src/cps_api_object.cpp src/cps_api_object_tools.cpp \
    src/cps_api_operation_common.cpp src/cps_api_operation_service.cpp \
    src/cps_api_operation_tools.cpp src/cps_api_utils.cpp src/cps_dictionary.cpp \
    src/cps_dictionary_loader.cpp src/cps_ns.cpp

libsonic_cps_api_la_LDFLAGS = -lxml2 -lpthread -lrt
libsonic_cps_api_la_CPPFLAGS = -std=c++11 -I$(top_srcdir)/sonic -I$(includedir)/sonic \
     			       -I$(includedir)/libxml2

libsonic_cps_class_map_la_SOURCES = src/cps_class_map_query.cpp
libsonic_cps_class_map_la_CPPFLAGS = -std=c++11 -I$(top_srcdir)/sonic \
                                     -I$(includedir)/sonic
libsonic_cps_class_map_la_LDFLAGS = -lpthread

#The CLI used to change levels at runtime
bin_PROGRAMS = cps_api_service

cps_api_service_SOURCES=src/cps_api_service.cpp
cps_api_service_CPPFLAGS=-I$(top_srcdir)/sonic -I$(includedir)/sonic
cps_api_service_LDFLAGS=-lsonic_cps_api -lsonic_common -lsonic_logging

all-local: cps.so

clean-local:
	rm cps.so

cps.so: src/python_extension/*
	cd src/python_extension && python setup.py build -v --build-lib ../..
