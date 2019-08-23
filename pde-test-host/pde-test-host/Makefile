#*********************************************************************
#
# (C) Copyright Broadcom Corporation 2013-2014
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
#
#*********************************************************************

export BCM_PHY_LIST := 82764 84752 84328 84740 8750 84328 542XX
BOARD_LIST=EMPTY

export BCM_PTL_SPT=1
export BCM_56850_A0=1
export BCM_56860_A0=1
export BCM_56960_A0=1
export BCM_56970_A0=1
export BCM_56970_B0=1
export BCM_56980_A0=1
export BCM_56980_B0=1
export BCM_56870_A0=1
export BCM_56340_A0=1

CFLAGS += -DBCM_ESW_SUPPORT
CFLAGS += -DINCLUDE_L3


ifeq ($(SAI_ROOT),)
   SAI_ROOT := $(shell echo $(dir $(abspath $(firstword $(MAKEFILE_LIST)))) | sed -e 's/\(.*\)\/.*output\/.*/\1/')
endif
export SAI_ROOT

ifeq ($(OUTPATH),)
   OUTPATH := $(dir $(abspath $(firstword $(MAKEFILE_LIST))))
endif
export OUTPATH


ifeq ($(DEBIAN_PKG),)
INCLUDE_FLAGS += -I$(SAI_ROOT)include -I$(SAI_ROOT)/unittests -I$(SAI_ROOT)/include/pcap -I$(SAI_ROOT)/include/hiredis -I$(SAI_ROOT)../../../sonic-sairedis/SAI/inc
endif



UNITTEST_OBJS := $(OUTPATH)unittest_objs
CUNIT_OBJS := $(OUTPATH)cunit_objs
PCAP_LIBS := $(SAI_ROOT)libpcap
HIREDIS_LIB := $(SAI_ROOT)libhiredis

unittest_source_files = $(strip $(wildcard $(SAI_ROOT)/unittests/*.c))
cunit_source_files = $(strip $(wildcard $(SAI_ROOT)/cunit/*.c))

test_source_files :=  $(wildcard $(SAI_ROOT)/test_adapter_host.c)
wb_src_files := $(wildcard $(SAI_ROOT)/wb_test.c)
ptf_setup_file :=  $(wildcard $(SAI_ROOT)/ptf_setup.c)
sai_includes := $(wildcard $(SAI_ROOT)/include/*.h)


test_host:=$(SAI_ROOT)/pde-test-host

vpath %.h $(sai_includes)

ifneq (,$(unittest_source_files))
vpath %.c $(realpath $(SAI_ROOT)/unittests)
vpath %.o $(UNITTEST_OBJS)/
endif

ifneq (,$(cunit_source_files))
vpath %.c $(realpath $(SAI_ROOT)/cunit)
vpath %.o $(CUNIT_OBJS)/
endif

all: $(test_host)

unittest_objs := $(addprefix $(UNITTEST_OBJS)/, $(notdir $(unittest_source_files:.c=.o)))
cunit_objs := $(addprefix $(CUNIT_OBJS)/, $(notdir $(cunit_source_files:.c=.o)))

cunit_archive = $(abspath $(CUNIT_OBJS)/cunit.a)
unittest_archive = $(abspath $(UNITTEST_OBJS)/unit_test.a)

$(unittest_objs): $(UNITTEST_OBJS)/%.o : %.c
	mkdir -p $(UNITTEST_OBJS)
	$(CC) $(INCLUDE_FLAGS) $(CFLAGS_SAI_SDK) $(CFLAGS) -c $^ -o $@

$(cunit_objs): $(CUNIT_OBJS)/%.o : %.c
	mkdir -p $(CUNIT_OBJS)
	$(CC) $(INCLUDE_FLAGS) $(CFLAGS_SAI_SDK) $(CFLAGS) -c $^ -o $@

$(unittest_archive): $(unittest_objs)
	$(RM) -f $@
	$(AR) cq $@ $^

$(cunit_archive): $(cunit_objs)
	$(RM) -f $@
	$(AR) cq $@ $^

$(test_host): $(test_source_files:.o=.c) $(unittest_archive) $(cunit_archive)

	$(CC) -g -O0 $(INCLUDE_FLAGS) $(CLAGS_SAI_SDK) $^ -lpthread -l:libpcap.so -l:libsai.so.1 -ldl -rdynamic -o $@

clean:
	@echo "Cleaning test only files."
	$(RM) -f $(test_host)
	$(RM) -f $(UNITTEST_OBJS)/*
	$(RM) -f $(CUNIT_OBJS)/*

cleantestall:
	@echo "Cleaning test files including dirs."
	$(RM) -f $(test_host)
	$(RM) -rf $(UNITTEST_OBJS)
	$(RM) -rf $(CUNIT_OBJS)
