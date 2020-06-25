# CREPL
> **C**alculator **REPL**

A command line calculator read-eval-print-loop,
with intuitive mathematical notation as syntax.

## Install

You'll need `git` (if cloning), a C compiler (e.g. `gcc`)
and `make` (`build-essential` / `base-devel`).

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
 - [ ] User defined functions.
 - [ ] Garbage collection.
 - [ ] Add more functionality, notably for calculus.
 - [ ] Symbol manipulation (?).
 - [ ] Extend numbers to include “Big Numbers” (“Big Integers” and “Big Decimals”/Rationals), numbers a currently limited to ~80bit floats and pointer-sized (likely 64bit) integeres.
