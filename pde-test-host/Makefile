SHELL = /bin/bash
.ONESHELL:
	.SHELLFLAGS += -e


MAIN_TARGET = $(PDE_TEST_HOST)
DERIVED_TARGETS = pde-test-host-dev_$(PDE_TEST_HOST_VERSION_FULL)_all.deb 

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	pushd pde-test-host
	dpkg-buildpackage -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS)
	popd

	mv $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)
