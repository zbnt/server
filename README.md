# zbnt/server

Server for controlling ZBNT devices over TCP

## Related projects

* **Hardware cores and block designs:** [zbnt/hardware](https://github.com/zbnt/hardware)

## Requirements

* Bitstreams for the Programmable Logic (see [zbnt/hardware](https://github.com/zbnt/hardware))
* A working GNU/Linux system with the following libraries/tools:
	* Qt5
	* CMake
	* libfdt

## Building

1. Clone this repository, make sure all dependencies are installed before proceeding.
2. Initialize and clone the required submodules:

~~~
git submodule update --init
~~~

3. Create a directory for the build and `cd` to it:

~~~~
mkdir build && cd build
~~~~

4. Run `cmake`:

~~~~
cmake -DCMAKE_BUILD_TYPE=Release ..
~~~~

5. Run `make`, you can use the `-j` parameter to control the number of parallel jobs:

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
