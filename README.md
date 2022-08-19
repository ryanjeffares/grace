# Grace

[![Total alerts](https://img.shields.io/lgtm/alerts/g/ryanjeffares/grace.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/ryanjeffares/grace/alerts/) [![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/ryanjeffares/grace.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/ryanjeffares/grace/context:cpp) [![Linux](https://github.com/ryanjeffares/grace/actions/workflows/linux-build.yml/badge.svg)](https://github.com/ryanjeffares/grace/actions/workflows/linux-build.yml) [![MacOS](https://github.com/ryanjeffares/grace/actions/workflows/macos-build.yml/badge.svg)](https://github.com/ryanjeffares/grace/actions/workflows/macos-build.yml) [![Windows](https://github.com/ryanjeffares/grace/actions/workflows/windows-build.yml/badge.svg)](https://github.com/ryanjeffares/grace/actions/workflows/windows-build.yml)

Grace is a dynamically typed, bytecode interpreted programming language, written with C++20. Grace supports object oriented and procedural programming, reference counting as opposed to garbage collection, extension methods, and features a concise syntax inspired by Python and Ruby.

## Aspirations

The goal of Grace is to combine the ease of use and portability of an interpreted language with the scalability and robustness of a compiled language. Grace is easy to write and run, similar to Python, but to better achieve the goals of scalability it does not support a REPL shell or top level statements. A Grace program starts in the `main` function, and code is limited to within classes and functions.

An important aspiration of Grace is to be unambiguous and predictable. By keeping syntax and operators to a minimum, Grace can avoid unexpected behaviour and having many ways to do the exact same thing, which can be hostile things for newcomers and programming beginners.

Performance will stay predictable due to the use of reference counting over garbage collection (notice how I said "predictable" and not "good").

## Spec and Guidelines

The specification of the language and its standard library and grammar, as well as styling guidelines, can be found [here](https://github.com/ryanjeffares/gracelang).

## Contributing

As Grace is still very much in its infancy, I am not open to pull requests at the moment. But, if you do end up trying Grace out and come across a bug, or notice something wrong in the source code, please do not hesitate to open an issue!

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
python3 build.py <Release/Debug/All>
```

This will build the `grace` executable, which you can add to your path or move somewhere that is on your path. Full installation process as well as documentation is WIP.

**NOTE**: On Mac and Linux, sometimes the first build attempt will fail on `No rule to make target 'dyncall_build/dyncall/libdyncall_s.a'`. Running the build script a second time will fix this.

## Alpha Release Roadmap
* Why does examples/euler/problem03.gr use so much memory?
* Prevent integer overflow by automatically promoting to a `BigInt` class
* TEST THE CYCLE CLEANER
  * Experiment with what is the optimal frequency to run it
  * Add option to run async
    * Can be enabled via preprocessor, but runs so slowly on M1. Can provide as a command line option, maybe try doing it in a way that doesn't require locking mutexes in the GraceObject instances
  * Expose options in Grace
* Lambdas 
* Global const fields
  * Allow expressions containing literals and other constants
  * Improve error reporting
* Allow importing files that require going higher up the folder tree
* Expand standard library
* Allow assignment after index `[]` operator
* Dynamic library loading
  * This is in, but need to handle pointer types for things that aren't strings
  * And also pointer return types for strings and others...
* Ability to get relative path from a Grace file
* Default function parameter values
* Install process 
* Tests 
* Documentation + comments 
* Improve compiler errors and line numbering
* Reduce code duplication in compiler, general clean up
* Make sure everything is always getting popped from the value stack when it needs to be

## Long Term Goals 
* Optional type annotations for use by a static analyzer
  * Type annotations in, expand syntax to allow for List/Dict inner types eg `Dict[String, Float]`, `List[List[Int]]`
  * Not a huge priority since no static analyzer exists yet
* Package management similar to cargo - new projects, type checking, adding libraries, tests
* Optimisation mode which allows the bytecode compiler to perform optimisations such as loop unrolling, constant folding, dead code elimination - slower initial compile time in return for better run time performance for scripts that will run for a long time

