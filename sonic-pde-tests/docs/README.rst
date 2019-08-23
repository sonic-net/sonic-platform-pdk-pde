sonic_pde_tests
================

This package is a part of the SONiC Platform Development Kit (PDK) and packaged
under the SONiC Platform Development Environment (PDE).

build
=====
- get source

- cd <sonic_pde_tests sourcedir> && ./build.sh

  (generates sonic_pde_tests-<ver>.deb)

install
=======

- remove existing ifupdown package
  dpkg -r sonic_pde_tests

- install sonic_pde_tests using `dpkg -i`

- or install from deb
    dpkg -i sonic_pde_tests-<ver>.deb


