# Project 1A: UNIX Utilities
Changelog (marked with underline):
**[Update Sept 13]** add wis-untar program and test instruction
**[Update Sept 13]** add specification on file size type
**[Update Sept 17]** add clarifications
**[Update Sept 19]** remove trailing space in "Usage: wis-tar ..."
**[Update Sept 19]** add test script instructions

---

This is part A of project 1. In this part, we will write two small programs to get familiar with file IO and string manipulation.

## I. Reversing
You will write a reversing program. (Don't worry, we promise there won't be any spoiler of the movie Tenet in this project.) It should read input from a file, reverse the line order, then reverse the character order within each line, and finally print the result to another file. For example, with this input:

```
first-1
second-2
third-3
fourth-4
fifth-5
```
the program should produce this output:
```
5-htfif
4-htruof
3-driht
2-dnoces
1-tsrif
```
This program should be invoked as follows:
```
$ ./reverse -i inputfile -o outputfile
```
The above line means the user will type in the name of the program `./reverse` and give two arguments: an input file to read lines from called `inputfile` and an output file to put the reversed results into called `outputfile`.

Input files can be any text file - you can try it on your program, even. 

#### Assumptions and Error Handling
- **Line length:** You can assume that the lines in the file will be less or equal to 511 characters (so 512 with the trailing null `'\0'`)
- **File length:** Input files can be any length, but will fit in memory. Two copies of the file may not fit in memory, so however you store file data in memory, you should only have one copy
- **[Update Sept 17] File size:** You may assume the file size is small enough to fit in memory.
- **Number of lines:** there can be any number of lines, but the number of lines will be less than the size of the file. You can use the fstat function to determine how long the file is, and this will tell you the maximum number of lines in the file. It is also reasonable to re-allocate data structures if the file is longer than you initially expect.
- **Line format:** All lines in the input will end in a trailing newline '\n'
- **Invalid files:** If the user specifies an input or output file that you cannot open (for whatever reason), the reverse program should print (assuming the file named "foo"):
```
reverse: Cannot open file: foo
```
and then exit with `exit(1)` to return an error.

- **Too few or many arguments passed to the program**: If the user runs `reverse` without any arguments, or in some other way passes incorrect flags and such to `reverse`, print
```
Usage: reverse -i inputfile -o outputfile
```
and exit with `exit(1)` to return an error.

- **Printing errors:** On any error code, you should print the error to the screen using `fprintf()`, and send the error message to stderr (standard error) and not stdout (standard output). This can be accomplished in your C code as follows: `fprintf(stderr, “whatever the error message is\n”);`

#### Hints
- If you want to figure out how big the input file is before reading it in, use the `stat()` or `fstat()` calls.
- You might need to create a data structure in memory, such as an array, to hold the file contents.
- To exit, call `exit()` with a single argument. This argument to `exit()` is then available to the user to see if the program returned an error (i.e., return 1 by calling `exit(1)` ) or exited cleanly (i.e., returned 0 by calling `exit(0)` ).
- The routine `malloc()` is useful for memory allocation. Make sure to exit cleanly if malloc fails, and call free() when you're done with the allocated memory!
- Some other functions you may find helpful when dealing with files: fopen(3), fclose(3), getline(3), fread(3), fwrite(3), and fprintf(3). The numbers in parentheses indicate which section in the manual these functions can be found. To access manual pages in Linux, use the command "man [section number] [function name]". For example, for the manual page for getline, run "man 3 getline". (Although most of the time the section number can be omitted -- "man getline" should work as well.)
- Another good reference page you could turn to is [cppreference.com](http://cppreference.com "cppreference.com"). Just search the functions you want to know and select the C version (instead of C++ version) of reference pages. These references usually come with some examples, which can be useful.
 

## II. Wis-tar
The next utility you build will be a simpler version of tar, which is a commonly used UNIX utility to combine a collection of files into one file. 

The input to your wis-tar program will be the name of the tar file followed by a list of files that need to be archived. (Fun Fact: The name tar comes from tape archives!)

Example:

```
$ echo "CS 537, Fall 2020" > a.txt
$ echo "Operating Systems" > b.txt
$ ./wis-tar test.tar a.txt b.txt
```
The first two commands create two text files -- a.txt containing the string "CS 537, Fall 2020" and b.txt containing "Operating Systems". The third executes the wis-tar program to combine these two text files into one archive file test.tar.

The archive file must follow a specific format: Before writing the contents of each file to the archive, we first need to create a fix-sized header containing information (metadata) about the file. This header will have two fix-sized fields: 256 bytes for the file name string and 8 bytes for the file's size in bytes. Unused space in the file name field should be filled with NULL characters, i.e. '\0'. After this header is written to the tar file, we can proceed to write the corresponding file's contents to the tar file. 

Any number of files may be combined into an output tar file. There should be 8 bytes of NULL padding after each file, including the last one.

**[Update Sept 17] **The following example shows what needs to be written to the archive file in the correct order. The actual tar file is not a text file. Do not add new lines (\n) after each component. "As binary" means write the integer directly to the file as is; there is no need to convert it to a number string.

```
file1 name [256 bytes in ASCII] 
file1 size [8 bytes as binary]
contents of file1 [in ASCII]
\0\0\0\0\0\0\0\0 [8 bytes of NULL padding]
file2 name [256 bytes in ASCII]
file2 size [8 bytes as binary]
contents of file2 [in ASCII]
\0\0\0\0\0\0\0\0 [8 bytes of NULL padding]
file3 name [256 bytes in ASCII]
file3 size [8 bytes as binary]
\0\0\0\0\0\0\0\0 [8 bytes of NULL padding]
```
#### Assumptions and Error Handling
- **Input file location:** You may assume that the files provided as an input exist in the same directory as the program itself.
- **Filename length:** You may assume that file names will fit into the reserved 256 bytes in the header. (Linux only allows a maximum of 255 bytes for file names anyway.)
- **Too few arguments:** The program can accept one or more input files, and one and only one output file must be specified. If fewer than two arguments are supplied to your program, print
```
Usage: wis-tar ARCHIVE [FILE ...]
```
and exit with return value 1.

- **Cannot open file:** If any files cannot be opened, print (assuming the file named "bar"):
```
wis-tar: Cannot open file: bar
```
and exit with status 1.

- **Archive already exists:** If the specified output archive file already exists, just overwrite it. There is no need to report an error.
- **Printing errors:** On any error code, you should print the error to the screen using `fprintf()`, and send the error message to stderr (standard error) and not stdout (standard output).

#### Hints:
- For NULL padding, check out memset(3).
- **[Update Sept 17]** It may be helpful to view a raw hexadecimal dump of the generated tar file for debugging purposes. To do this, check out hexdump(1). Example usage: hexdump -C archive.tar
- **[Update Sept 13]** You could use uint64_t as file size, which guarantees will take 8 bytes. (Need to #include <stdint.h>)
- General hints in the reversing program can also be applicable.

## General Advice
**Start small, and get things working incrementally.** For example, first get a program that simply reads in the input file, one line at a time, and prints out what it reads in. Then, slowly add features and test them as you go.

**Testing is critical.** Testing your code to make sure it works is crucial. Write tests to see if your code handles all the cases you think it should. Be as comprehensive as you can be. Of course, when grading your projects, we will be. Thus, it is better if you find your bugs first before we do.

**Keep old versions around.** Keep copies of older versions of your program around, as you may introduce bugs and not be able to easily undo them. A simple way to do this is to keep copies around, by explicitly making copies of the file at various points during development. For example, let's say you get a simple version of `reverse.c` working (say, that just reads in the file); type `cp reverse.c reverse.v1.c` to make a copy into the file `reverse.v1.c` . More sophisticated developers use version control systems like git , but we'll not get into that here (yet).

**Keep your source code in a private directory.** An easy way to do this is to log into your account and first change directories into `private/` and then make a directory therein (say `p1` , by typing `mkdir p1` after you've typed `cd private/` to change into the private directory). However, you can always check who can read the contents of your AFS directory by using the `fs` command. For example, by typing in `fs listacl` . you will see who can access files in your current directory. If you see that `system:anyuser` can read (r) files, your directory contents are readable by anybody. To fix this, you would type `fs setacl . system:anyuser ""` in the directory you wish to make private. The dot “.” referred to in both of these examples is just shorthand for the current working directory.

**Read the documentation.** Even the best programmer can't remember every detail of how to use these library functions. Read man page can be extremely helpful.

## Turning it in
You should only turn in two files: reverse.c and wis-tar.c . We will compile them in the following way:

```
$ gcc -Wall -Werror -o reverse reverse.c
$ gcc -Wall -Werror -o wis-tar wis-tar.c
```
so make sure it compiles in such a manner. If there is any warning message, you might want to fix it.

You should copy these files into your handin directory. You can copy the file with the cp program, as follows:

```c
$ cp reverse.c ~cs537-10/handin/$USER/p1/linux        
# $USER will be replaced with your CS login username automatically by the shell
$ cp wis-tar.c ~cs537-10/handin/$USER/p1/linux
```
## WHAT WE WILL LOOK FOR
- General implementation: Does the code work on simple files?
- Boundary case handling: Does the code work on more complex files (e.g. zero-length lines, long lines, very long files)

If you have any questions, feel free to ask in the piazza or come to office hours.
