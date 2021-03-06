# Project 1B: Adding a System Call to xv6

## Overview
We'll be doing kernel hacking projects in xv6, a port of a classic version of UNIX to a modern processor, Intel's x86. It is a clean and beautiful little kernel, and thus a perfect object for our study and usage.

This first project is just a warmup, and thus relatively light on work. The goal of the project is simple: adding a system call to xv6.

## Task
You are going to add a new system call to xv6; your new syscall should look like this:

	int getwritecount(void)
which returns the number of times that write() syscall has been invoked by the calling process.

## Hints
- You could add a variable as a counter, which is incremented every time write() is invoked.
- Get started by reading other syscall source code. getpid() can be a good candidate due to its simplicity.
- You probably would (and should) spend most of the time reading xv6 source code, trying to understand the structure, and (relatively) little time writing new code.

## The xv6 Source Code
The source code (and an associated README) for xv6 can be found in ~cs537-10/ta/xv6/. To copy the source code into your working directory:

	$ cd ~/private/             # assume you are using ~/private as the working directory
	$ cp -r ~cs537-10/ta/xv6/ . # copy xv6 source code into the working directory
	$ ls xv6
	README xv6.tar.gz
You can find many helpful instructions on how to build, run, and even debug xv6 kernel in the README. Below we list some of them, but you are strongly encouraged to read this README by yourself.

	$ vim xv6/README
To begin with, you should extract source code from the tarball:

	$ cd xv6
	$ tar -xvzf xv6.tar.gz
Then you will see a new directory xv6-fa20, which contains these files:

	$ cd xv6-fa20
	$ ls
	bootother  fs      include   kernel    README  user     xv6.img
	FILES      fs.img  initcode  Makefile  tools   version
To build the xv6 kernel:

	$ make
To run the xv6 kernel, we will use the emulator QEMU (qemu.org). QEMU will emulate an x86 processor and devices like the display and disks. QEMU also provides support for debugging via gdb. The xv6 makefile has a target set up for running in QEMU. To start xv6 in QEMU, just run:

	$ make qemu-nox
This will create disk images, start QEMU, and boot xv6. Test out the unmodified code by running a few of the existing user-level applications, like ls and forktest. To quit the emulator, type Ctrl-a x (press ctrl and a together, release both, and then press x).

Using gdb (the debugger) can be helpful in understanding code, doing code traces, and is helpful for later projects too. Get familiar with this fine tool!

	$ make qemu-nox-gdb
You are encouraged to look at the makefile to understand how commands above work. You may see there are also make target "qemu" and "qemu-gdb". These targets will run xv6 on an X Window which might makes things a little complicated; you don't have to worry about them, just use "-nox" versions.

You may also find the following book about xv6 useful, written by the same team that ported xv6 to x86: book (Links to an external site.). However, note that the kernel version we use is a little different than the book.

We will provide the same automated tests that we will use to grade the program a week before the due date.

## Turning it in
For the xv6 part of the project, copy all of your source files (but not .o files, please, or binaries!) into the @xv6/@ subdirectory of your p1 directory. A simple way to do this is to copy everything into the destination directory, then type make to make sure it builds, and then type make clean to remove unneeded files.a

	$ ls          # you are in xv6-fa20
	bootother  fs      include   kernel    README  user     xv6.img
	FILES      fs.img  initcode  Makefile  tools   version
	$ make clean
	$ ls          # check after cleaning
	FILES  include  kernel  Makefile  README  tools  user  version
	$ cp -r . ~cs537-10/handin/$USER/p1/xv6
	$ cd ~cs537-10/handin/$USER/p1/xv6
	$ make
	$ make clean
Finally, into your p1 directory, please include a README file. In there, describe what you did a little bit (especially if you ran into problems and did not implement something). The most important bit, at the top, however, should be the authorship (name and cs login) of the project.
