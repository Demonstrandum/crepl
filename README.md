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

You may also compile with debug-printing by `DEFINES=-DDEBUG make` instead.

## Example

An example of a session:

```console
::> k = 9
#=> 9
::> f x = 2x - k
::> f 10
#=> 11
::> addTo x = y -> x + y
::> add3 = addTo 3
::> add3 10
#=> 13
::> sum = x -> y -> x + y
::> (sum 2) 3
#=> 5
::> sin 2pi - 1
#=> -1
::> (k-2)!
#=> 5040
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
   - [x] Anonymous functions.
 - [ ] Pool constant and literal values in a preallocated structure, to save on reallocating constants by preventing them from being garbage collected.
 - [x] Garbage collection.
   - [x] Reference count function scopes.
   - [x] Reference count data values.
 - [ ] Numerical equation solver (polynomial, simultaneous, &c.).
 - [ ] Add more functionality, notably for calculus.
 - [ ] Symbol manipulation (?).
 - [ ] Extend numbers to include “Big Numbers” (“Big Integers” and “Big Decimals”/Rationals), numbers a currently limited to ~80bit floats and pointer-sized (likely 64bit) integeres.
