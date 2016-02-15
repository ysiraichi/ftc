# The Pure-Functional Tiger Language Compiler (ftc)

The 'ftc' program is a Pure-functional Tiger Language Compiler. 

The Pure-Functional Tiger is a derived language from the imperative Tiger, 
specified in the book ['Modern Compiler Implementation in C', by Andrew W. Appel](https://www.cs.princeton.edu/~appel/modern/c/). 
As the name suggests, it is a pure-functional language, as specified in the book, chapter 15.

### Setup

In order to setup this program, first clone this repo and then build it, as follows:

```
$ git clone https://github.com/YuKill/ftc
$ cd ftc && mkdir build && cd build
$ make && make install
```

### Usage
```
$ ftc <inputFile> [-o <outputFile>] [-tree] [-emit-llvm ] [-emit-as]
```

##### Arguments:

* InputFile  :: the file which contains a pure-functional tiger program;
* -o         :: the output file base name;
* -tree      :: prints the program's ASTree in a 'dot' file graph;
* -emit-llvm :: emits a file corresponding to the program in LLVM IR language;
* -emit-as   :: emits a file corresponding to the program in Assembly language;
