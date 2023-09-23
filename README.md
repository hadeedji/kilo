# kilo

My implementation of the [kilo text editor][1], which I wrote by following
[this tutorial][2].

## How to build

As simple as it can be, there are no external dependencies.

You can either use the provided Makefile:
``` sh
make kilo
```

Or if you don't have make, just compile and link all the source files together,
specifying the include directory:
``` sh
gcc -I include src/*.c -o kilo
```

## My additions
- Split it up into multiple files and tried to follow good design and
organization practices.

## References
- [antirez/kilo][1]
- [Build Your Own Text Editor][2]

[1]: https://github.com/antirez/kilo
[2]: https://viewsourcecode.org/snaptoken/kilo/index.html
