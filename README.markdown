# C Compiler for YAPL

## What is YAPL?
YAPL stands for Yet Another Programming Language. It is a programming language I created. :-)

## Quick Start Guide
1. Add .exe extensions to compiler and source. compiler is the compiler, and source is the program generated from the object file that is in the repository (source.obj).
2. Open the MS-DOS command prompt. Type "compiler file.yapl" where file.yapl is the name of the YAPL source code file. (See source.yapl for an example of valid YAPL syntax.) An object file (file.obj) will be outputted after running compiler.exe.
3. Use Link.exe that comes with the [MASM 6.15](http://www2.hawaii.edu/~pager/312/masm%20615.ZIP) assembler to create the executable file from the object file. Type "link file,,,util.lib" to link file.obj with the util.lib library file.
4. Finally, type "file" to run your YAPL program.
5. Enjoy!

## Links
You can view my blog [here](http://thetravelingcs.blogspot.com).

Developer's website: [BJ Peter DeLaCruz](http://www2.hawaii.edu/~bjpeter)
