
# zbnt_sw

Board software for ZBNT, a configurable network tester for the [ZedBoard](http://www.zedboard.org/product/zedboard)

## Related projects

* **GUI:** [zbnt_sw](https://github.com/oscar-rc1/zbnt_sw)
* **Hardware cores and block designs:** [zbnt_hw](https://github.com/oscar-rc1/zbnt_hw)

## Requirements

* Bitstreams for the Programmable Logic (see [zbnt_hw](https://github.com/oscar-rc1/zbnt_hw))
* A working GNU/Linux system running on the board, with the following libraries/tools:
	* Qt5Core
	* Qt5Network
	* GNU Scientific Library (GSL)
	* CMake

## Building

1. Clone this repository, make sure all dependencies are installed before proceeding.
2. Create a directory named `hw` and copy the bitstream binaries there.
2. Create a directory for the build, `cd` to it and run `cmake`:

~~~~
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
~~~~

3. Run `make`, you can use the `-j` parameter to control the number of parallel jobs:

~~~
make -j16
~~~

## License

![GPLv3 Logo](https://www.gnu.org/graphics/gplv3-with-text-84x42.png)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
