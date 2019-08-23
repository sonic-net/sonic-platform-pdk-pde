import os
import pytest
import json
import subprocess

def pytest_configure(config):
    config.addinivalue_line("markers", "semiauto: this marks test will run in semiauto mode.")

def pytest_addoption(parser):
    parser.addoption(
                "--semiauto", action="store_true", default=False, help="run semiautomated tests")

def pytest_collection_modifyitems(config,items):
    if config.getoption("--semiauto"):
        return
    skip_semiauto = pytest.mark.skip(reason="need --semiauto option to run")
    for item in items:
        if "semiauto" in item.keywords:
            item.add_marker(skip_semiauto)

# Get platform name from ONIE
def get_onie_pname():
    cmd = "cat /host/machine.conf | grep onie_platform | cut -d '=' -f 2"
    pin = subprocess.Popen(cmd,
                           shell=True,
                           close_fds=True,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT)
    id = pin.communicate()[0]
    id = id.strip()
    return id


@pytest.fixture(scope='function',autouse='True')
def json_config_data():
    """ Loads json file """
    INPUT_DIR = os.path.dirname(os.path.abspath(__file__))
    fname = os.path.join(INPUT_DIR, 'data/platform', "platform-" + get_onie_pname() + "-config.json")
    try:
        with open('/home/pde/platform-config.json') as file_object:
            contents=json.load(file_object)
    except:
        with open(fname) as file_object:
            contents=json.load(file_object)

    return contents

@pytest.fixture(scope='function',autouse='True')
def json_test_data():
    """ Loads json file """
    INPUT_DIR = os.path.dirname(os.path.abspath(__file__))
    fname = os.path.join(INPUT_DIR, 'data/platform', "test-" + get_onie_pname() + "-config.json")
    try:
        with open('/home/pde/test-config.json') as file_object:
            contents=json.load(file_object)
    except:
        with open(fname) as file_object:
            contents=json.load(file_object)

    return contents

