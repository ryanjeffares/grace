# Grace

[![Total alerts](https://img.shields.io/lgtm/alerts/g/ryanjeffares/grace.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/ryanjeffares/grace/alerts/) [![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/ryanjeffares/grace.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/ryanjeffares/grace/context:cpp) [![Linux](https://github.com/ryanjeffares/grace/actions/workflows/linux-build.yml/badge.svg)](https://github.com/ryanjeffares/grace/actions/workflows/linux-build.yml) [![MacOS](https://github.com/ryanjeffares/grace/actions/workflows/macos-build.yml/badge.svg)](https://github.com/ryanjeffares/grace/actions/workflows/macos-build.yml) [![Windows](https://github.com/ryanjeffares/grace/actions/workflows/windows-build.yml/badge.svg)](https://github.com/ryanjeffares/grace/actions/workflows/windows-build.yml)

Grace is a fast, dynamically typed, bytecode interpreted programming language, written with C++20. Grace supports object oriented and procedural programming, reference counting with a minimal garbage collector that only handles cyclic references, extension methods, and features a concise syntax inspired by Python and Ruby.

## Aspirations

The goal of Grace is to combine the ease of use and portability of an interpreted language with the scalability and robustness of a compiled language. Grace is easy to write and run, similar to Python, but to better achieve the goals of scalability it does not support a REPL shell or top level statements. A Grace program starts in the `main` function, and code is limited to within classes and functions.

An important aspiration of Grace is to be unambiguous and predictable. By keeping syntax and operators to a minimum, Grace can avoid unexpected behaviour and having many ways to do the exact same thing, which can be hostile things for newcomers and programming beginners.

## Spec and Guidelines

The specification of the language and its standard library and grammar, as well as styling guidelines, can be found [here](https://github.com/ryanjeffares/gracelang). Note that the spec and documentation is very much WIP, like the language.

## Contributing

I am very much open to contributions. If you'd like to contribute or have a problem with Grace, please open an issue first.

## License

Grace is licensed under the MIT License.

## Getting Started 

Grace's only dependencies are [fmtlib](https://github.com/fmtlib/fmt) which is used in header only mode and included as a submodule in this repository, and [dyncall](https://github.com/LWJGL-CI/dyncall) which is also included as a submodule and built/statically linked to by CMake. It is recommended to run the provided Python script with Python >= 3.2 to invoke CMake with the desired settings.

### Prerequisites:
* Python >= 3.2
* CMake >= 3.8
* C++20 compiler
* C99 compiler

```bash
git clone --recursive https://github.com/ryanjeffares/grace.git 
cd grace 
python3 build.py <exe/dll> <Release/Debug/All>
```

This will build the `grace` executable in the build folder. To install to your system, run

```bash
python3 install.py
```

**NB: To use Grace globally, you will need to add the `GRACE_STD_PATH` environment variable and may need to add Grace to your path.**

##### Windows
On Windows, by default Grace installs to `C:/Program Files (x86)/grace/bin/grace.exe` and the standard library will be copied to `C:/Program Files (x86)/grace/std/`.

##### macOs/Linux
On Mac and Linux, by default Grace installs to `/usr/local/bin/grace` and the standard library is coped to `/usr/local/share/grace/std/`


## TODO

#### STD
* Expand standard library
* Precompile to bytecode that can be loaded by the interpreter

#### Core Language
* Short circuiting
* Prevent integer overflow by automatically promoting to a `BigInt` class
* Lambdas 
* Global const fields
  * Allow expressions
  * Improve error reporting
  * STD imports are broken?
* Compound assignment to subscripts and object members
* Allow importing files that require going higher up the folder tree
* Default function parameter values
* Dynamic library loading
  * This is in, but need to handle pointer types for things that aren't strings
  * And also pointer return types for strings and others...
* Think about how `char`s should behave in binary operations

#### Interpreter
* TEST THE CYCLE CLEANER
  * There is a problem - see backup day 7 aoc solution
* Improve compiler errors and line numbering
* Reduce code duplication in compiler, general clean up
  * The parsing is a bit all over the place. Review grammar and stick strictly to recursive descent
* Ensure use of backticks and quotes is consistent in error messages

#### Business™
* Install process 
* Tests 
* Documentation + comments 
* Dynamic library build
* Can we avoid copying over `dyncall` static libs and headers during install? Maybe hack the CMake to just compile as source...


## Long Term Goals 
* Optional type annotations for use by a static analyzer
  * Type annotations in, expand syntax to allow for List/Dict inner types eg `Dict[String, Float]`, `List[List[Int]]`
  * Not a huge priority since no static analyzer exists yet
* Package management similar to cargo - new projects, type checking, adding libraries, tests
* Optimisation mode which allows the bytecode compiler to perform optimisations such as loop unrolling, constant folding, dead code elimination - slower initial compile time in return for better run time performance for scripts that will run for a long time

