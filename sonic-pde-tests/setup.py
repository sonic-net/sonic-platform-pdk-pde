# https://github.com/ninjaaron/fast-entry_points
# workaround for slow 'pkg_resources' import
#
# NOTE: this only has effect on console_scripts and no speed-up for commands
# under scripts/. Consider stop using scripts and use console_scripts instead
#
# https://stackoverflow.com/questions/18787036/difference-between-entry-points-console-scripts-and-scripts-in-setup-py
try:
    import fastentrypoints
except ImportError:
    from setuptools.command import easy_install
    import pkg_resources
    easy_install.main(['fastentrypoints'])
    pkg_resources.require('fastentrypoints')
    import fastentrypoints

import glob
from setuptools import setup

setup(
    name='sonic_pde_tests',
    version='1.0',
    description='PDE pytest runtime utilities for SONiC',
    license='Apache 2.0',
    author='Broadcom SONiC Team',
    packages=[
        'sonic_pde_tests',
        'sonic_pde_saiut',
    ],
    package_data={
         'sonic_pde_tests': ['data/platform/*.json','data/template/*.json'],
         'sonic_pde_saiut': ['*.so'],
    },
    tests_require = [
        'pytest'
    ],
    keywords='sonic PDE utilities',
)
