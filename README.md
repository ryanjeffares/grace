# Grace

[![Total alerts](https://img.shields.io/lgtm/alerts/g/ryanjeffares/grace.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/ryanjeffares/grace/alerts/) [![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/ryanjeffares/grace.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/ryanjeffares/grace/context:cpp)

Grace is a dynamically typed, bytecode interpreted programming language, written with C++20. Still a work in progress, Grace will support object-oriented and procedural programming, use reference counting as opposed to a garbage collector, and features a concise syntax inspired by Python and Ruby.

## Aspirations

The goal of Grace is to combine the ease of use and portability of an interpreted language with the scalability and robustness of a compiled language. Grace is easy to write and run, similar to Python, but to better achieve the goals of scalability it does not support a REPL shell or top level statements. A Grace program starts in the `main` function, and code is limited to within classes and functions.

An important aspiration of Grace is to be unambiguous and predictable. By keeping syntax and operators to a minimum, Grace can avoid unexpected behaviour and having many ways to do the exact same thing, which can be hostile things for newcomers and programming beginners.

Performance will stay predictable due to the use of reference counting over garbage collection.

## Spec and Guidelines

The specification of the language and its standard library and grammar, as well as styling guidelines, can be found [here](https://github.com/ryanjeffares/gracelang).

## Contributing

As Grace is still very much in its infancy, I am not open to pull requests at the moment. But, if you do end up trying Grace out and come across a bug, or notice something wrong in the source code, please do not hesitate to open an issue!

## License

Grace is licensed under the MIT License.

## Getting Started 

Grace's only dependency is [fmtlib](https://github.com/fmtlib/fmt) which is used in header only mode and included as a submodule in this repository, so building is simple

### Prerequisites:
* Python >= 3.2
* CMake >= 3.8
* C++ compiler
* C++20

```bash
git clone --recursive https://github.com/ryanjeffares/grace.git 
cd grace 
python3 build.py <Release/Debug/All>
```

This will build the `grace` executable, which you can add to your path or move somewhere that is on your path. Full installation process as well as documentation is WIP.

## Alpha Release Roadmap
* Why does examples/euler/problem03.gr use so much memory?
* Prevent integer overflow by automatically promoting to a `BigInt` class
* Experiment with string interning
* Investigate wide chars over regular chars
* Classes
  * Reference counting
  * Cyclic references handled through a "cyclic reference tracker" - if a cyclic reference is detected, start tracking the two objects, and when those objects' only remaining references are each other they can be safely destroyed 
  * Maybe we could use structs that only hold variables, and just use extension methods...
* Lambdas 
* Global const fields 
* Imports 
  * Obviously importing the same file multiple times is allowed, we will only compile it the first time - but how do we warn the user about two files with the same but different contents?
  * Reorganise the way functions are held in memory to avoid duplicate function definitions
  * Fix namespace qualifiers inside a function call that's also namespace qualified
* Extension methods
* Standard library
* Install process 
* Tests 
* Documentation + comments 
* Improve compiler errors and line numbering

## Long Term Goals 
* Optional type annotations for use by a static analyzer
  * Type annotations in, expand syntax to allow for List/Dict inner types eg `Dict[String, Float]`, `List[List[Int]]`
  * Not a huge priority since no static analyzer exists yet
* Package management similar to cargo - new projects, type checking, adding libraries, tests
* Optimisation mode which allows the bytecode compiler to perform optimisations such as loop unrolling, constant folding, dead code elimination - slower initial compile time in return for better run time performance for scripts that will run for a long time

