/*
	zbnt_sw
	Copyright (C) 2019 Oscar R.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <WorkerThread.hpp>

volatile void *axi_addr = NULL;

int main()
{
	int fd = open("/dev/mem", O_RDWR | O_SYNC);

	if(fd == -1)
		return 1;

	axi_addr = mmap(NULL, 0xC0000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x43C00000);

	if(!axi_addr)
		return 1;

	// TODO: Program PL, create QCoreApplication, start worker thread, etc.

	return 0;
}

