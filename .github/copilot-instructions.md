# Copilot Instructions for sonic-platform-pdk-pde

## Project Overview

sonic-platform-pdk-pde (Platform Development Environment) provides a test framework for validating SONiC platform drivers and modules. It enables platform vendors to verify their hardware abstraction layer implementations (fans, PSUs, thermals, SFPs, LEDs, etc.) against the SONiC Platform API specifications.

## Architecture

```
sonic-platform-pdk-pde/
├── sonic-pde-tests/        # PDE test suite
│   ├── sonic_pde_tests/    # Python test modules
│   │   ├── test_fan.py     # Fan platform API tests
│   │   ├── test_psu.py     # PSU platform API tests
│   │   ├── test_thermal.py # Thermal sensor tests
│   │   ├── test_sfp.py     # SFP/transceiver tests
│   │   ├── test_led.py     # LED tests
│   │   └── ...             # Additional platform tests
│   ├── setup.py            # Package setup
│   └── data/               # Test data and configurations
├── debian/                 # Debian packaging
├── .github/                # GitHub configuration
└── LICENSE
```

### Key Concepts
- **Platform API**: SONiC defines abstract Python APIs for hardware components (sonic-platform-common)
- **PDE tests**: Validate that platform implementations correctly implement these APIs
- **Hardware abstraction**: Tests verify fans, PSUs, thermals, SFPs, LEDs, watchdog, etc.
- **Vendor validation**: Platform vendors run these tests to certify their SONiC platform support

## Language & Style

- **Primary language**: Python
- **Test framework**: pytest
- **Indentation**: 4 spaces
- **Naming conventions**:
  - Test files: `test_<component>.py`
  - Test functions: `test_<component>_<behavior>`
  - Classes: `PascalCase`

## Build & Testing

```bash
# Install the PDE test package
cd sonic-pde-tests
pip install .

# Run tests on a SONiC switch with platform drivers loaded
pytest sonic_pde_tests/ -v

# Run specific component tests
pytest sonic_pde_tests/test_fan.py -v
pytest sonic_pde_tests/test_psu.py -v
```

## PR Guidelines

- **Signed-off-by**: Required on all commits
- **CLA**: Sign Linux Foundation EasyCLA
- **Testing**: New tests must be runnable on real hardware with proper platform drivers
- **API compatibility**: Tests must align with the sonic-platform-common API definitions

## Gotchas

- **Hardware required**: Most tests require actual switch hardware — they can't run in VS
- **Platform variations**: Different vendors may have varying capabilities — tests should handle optional features
- **API versions**: Tests must stay in sync with sonic-platform-common API changes
- **Permissions**: Tests may require root access to read hardware sensors
- **Transient failures**: Hardware-dependent tests may have timing sensitivities
