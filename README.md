# CREPL
> **C**alculator **REPL**

A command line calculator read-eval-print-loop,
with intuitive mathematical notation as syntax.

## Install

You'll need `git` (if cloning), a C compiler (e.g. `gcc`),
the GNU readline library, and `make` (`build-essential` / `base-devel`).

### Clone
Clone / Download this repositroy however you like, e.g.
```sh
git clone "https://github.com/Demonstrandum/crepl.git"
```

Then change directory into it:
```sh
cd crepl
```

### Build & Install
```sh
make  # Builds and compiles the project.
sudo make install  # Installs the program system wide.
```

## TODO
 - [ ] Temporary variables with `let ... in ...` and `... where ...` expressions.
 - [ ] Computed physical units with through postfix operators.
   - [ ] User defined units.
 - [ ] A `ref(.)` function, for referencing/aliasing other variables.
 - [ ] Throw errors on overflows until we implement bignums.
 - [ ] Imaginary numbers (using `complex.h`).
 - [ ] User defined functions.
   - [x] Single argument.
   - [ ] Tuple argument.
 - [ ] Garbage collection.
 - [ ] Numerical equation solver (polynomial, simultaneous, &c.).
 - [ ] Add more functionality, notably for calculus.
 - [ ] Symbol manipulation (?).
 - [ ] Extend numbers to include “Big Numbers” (“Big Integers” and “Big Decimals”/Rationals), numbers a currently limited to ~80bit floats and pointer-sized (likely 64bit) integeres.
