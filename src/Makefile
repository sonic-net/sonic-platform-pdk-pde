SYSROOT_PREFIX=../../platform-root/opt/ngos
DEVKIT_HEADER=${SYSROOT_PREFIX}/inc
INC_CFLAGS=-I${DEVKIT_HEADER}
INC_DIR = -I ../inc -I ../inc/private ${INC_CFLAGS}
LIB_PATH= ${SYSROOT_PREFIX}/lib

all:cps-api-common cps-api-service cps-class-map-util cps_py_extension

install: all
	cp ../inc/*.h ${DEVKIT_HEADER}/
   


cps-api-common_SRCS=cps_api_object.cpp cps_api_events.cpp cps_api_base_event_adapter.cpp cps_api_operation_common.cpp\
                       cps_api_key.cpp cps_api_object_attr.cpp cps_ns.cpp cps_api_operation_service.cpp\
                       cps_api_client_ipc.cpp cps_dictionary.cpp cps_dictionary_loader.cpp cps_api_object_tools.cpp\
                       cps_api_operation_tools.cpp cps_api_utils.cpp

cps-api-common_OBJS=cps_api_object.o cps_api_events.o cps_api_base_event_adapter.o cps_api_operation_common.o\
                    cps_api_key.o cps_api_object_attr.o cps_ns.o cps_api_operation_service.o\
                    cps_api_client_ipc.o cps_dictionary.o cps_dictionary_loader.o cps_api_object_tools.o\
                    cps_api_operation_tools.o cps_api_utils.o

cps-api-common:
	g++ -c -fPIC -std=c++11 $(INC_DIR) $(cps-api-common_SRCS)
	g++ -shared -o libcps-api-common.so $(cps-api-common_OBJS) -L $(LIB_PATH) -levent_log -ldn_common -lpthread
	mv libcps-api-common.so $(LIB_PATH)

cps-api-service:
	g++ -c -std=c++11 $(INC_DIR) cps_api_service.cpp
	g++ -o cps_api_service cps_api_service.o -L $(LIB_PATH) -lcps-api-common -levent_log -ldn_common
	mv cps_api_service $(SYSROOT_PREFIX)/bin/

cps-class-map-util:
	g++ -c -fPIC -std=c++11 $(INC_DIR) cps_class_map_query.cpp
	g++ -shared -o libcps-class-map-util.so cps_class_map_query.o -L $(LIB_PATH) -levent_log -ldn_common -lpthread
	mv libcps-class-map-util.so $(LIB_PATH)


PY_OBJ_PATH=../py_extension
cps_python_CFLAGS=-DMAJOR_VERSION=1 -DMINOR_VERSION=0 $(INC_DIR) -Ipython2.7 -std=c++11 -I.

cps_py_extension: $(wildcard python_extension/*) $(LIB_PATH)/libcps-class-map-util.so $(LIB_PATH)/libcps-api-common.so | ${PY_OBJ_PATH} $(LIB_PATH)
	cp -f $(wildcard python_extension/*) ${PY_OBJ_PATH}
	mkdir -p ${PY_OBJ_PATH}/private && cp -f $(wildcard ../inc/private/*.h) ${PY_OBJ_PATH}/private
	cd ${PY_OBJ_PATH} && DISTUTILS_DEBUG=1 LDFLAGS="-L $(LIB_PATH)" CFLAGS="${cps_python_CFLAGS}" python setup.py build -v --build-lib ${PY_OBJ_PATH}
	cp ${PY_OBJ_PATH}/cps.so $(LIB_PATH)

clean:
	rm *.o
	rm -rf ../py_extension/*
	rm ../lib/libcps-*
	rm ../lib/cps.so	
	rm ../bin/cps_api_service 	
