# Grace

[![Total alerts](https://img.shields.io/lgtm/alerts/g/ryanjeffares/grace.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/ryanjeffares/grace/alerts/) [![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/ryanjeffares/grace.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/ryanjeffares/grace/context:cpp)

Grace is a dynamically typed, bytecode interpreted programming language, written with C++17, that manages memory without the need for a garbage collector through reference counting. Still a work in progress, Grace will support object oriented and precedural programming, and features a concise syntax inspired by Python and Ruby.

The goal of Grace is to provide the portability and ease of use of an interpreted language with the scalability and robustness of a compiled language. For this prupose, Grace does not support a REPL or top-level statements - a Grace program starts at the `main` function, and performance is predictable due to automatic reference counting as opposed to a garbage collector. 

# Getting Started 

Grace's only dependency is [fmtlib](https://github.com/fmtlib/fmt) which is used in header only mode and included in this repository, so building is simple. In a terminal, simply run

```bash
git clone https://github.com/ryanjeffares/grace.git 
cd grace 
mkdir build 
cd build 
cmake ..
cmake --build .
```

This will build the `grace` executable, which you can add to your path or move somewhere that is on your path. Full install process as well as documentation is WIP.

## TODO
* Performance
* Classes
* Functions as first class objects/lambdas 
* Imports
* Extension methods
* Native methods
* Consider removing colons, wrapping expressions for ifs/loops in parentheses
