*****************************************************
Unit Tests for IEC 61850 south plugin
*****************************************************

Require Google Unit Test framework

Install with:
::
    sudo apt-get install libgtest-dev libgmock-dev

To build and run the unit tests:
::
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Coverage ..
    make
    ./tests/unitTests/RunTests


To analyze the unit test coverage :

Install Gcovr:
::
   sudo apt-get install gcovr

To generate the coverage report:
::
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Coverage ..
    make iec61850_coverage_html
