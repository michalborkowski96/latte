# Latte compiler

This is a compiler of a simple OO language to x86_64 ASM, a solution to [this task](https://www.mimuw.edu.pl/~ben/Zajecia/Mrj2018/Latte/). In particular, please take a look at code examples that can be compiled.

The compiler doesn't use any libraries other than C++'s STL, which means the implementation also includes a hand-written parser. Of course, since that was my first time doing that, a number of choices was far from optimal.

What's important to notice is that the output assembly doesn't require standard C library. This means I use bare Linux syscalls instead for IO and memory allocation (easy, as there is no deallocation). Please refer to lib/runtime.S for details.

Once you call 'make' in both 'lib' and 'src', you should be ready to use the compiler, as it prints sensible error messages in case something is wrong.

The project is divided into parts, using one of standard approaches to compiler design:

* standard library
* lexer
* parser
* type checker
* x86_64 ASM backend
