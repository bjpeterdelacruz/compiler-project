# C Compiler Project

## Quick Start Guide
1. Add .exe extensions to compiler and source. compiler.exe is the compiler, and source.exe is the program generated from the object file that is in the repository (source.obj).
2. Open the MS-DOS command prompt. Type "compiler file.src" where file.src is the name of the source code file. (See source.src for an example of valid syntax.) An object file (file.obj) will be outputted after running compiler.exe. An error report (file.rpt) and cross-reference list (file.lst) will also be produced by the compiler.
3. Use Link.exe that comes with the [MASM 6.15](http://www2.hawaii.edu/~pager/312/masm%20615.ZIP) assembler to create the executable file from the object file. Type "link file,,,util.lib" to link file.obj with the util.lib library file.
4. Finally, type "file" to run your compiled program.
5. Enjoy!

## Links
Developer's website: [BJ Peter DeLaCruz](http://www.bjpeterdelacruz.com)
