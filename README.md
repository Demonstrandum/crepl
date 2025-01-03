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

### Symbolic Manipulation

Quotes are expressions enclosed in `[`, `]`.
Quotes recursively evaluate its containing expression until it reaches
a leaf (a literal value) or it reaches an unresolvable symbol (an unknown).

So, if `f (x, a) = x^3 + 2x + a` is defined, but no variable `x` is defined,
writing `[f(x, 1)]` evaluates to `[x^3 + 2x + 1]`.

Quoting is idempotent `[[f(x)]]` is the same as `[f(x)]`.
And, `[x + [y] + 3]` is the same as `[x + y + 3]`.
Quoting literals leaves them unchanged `[5]` is the same as `5`.

To evaluate quotes, simply trigger evaluating the quote again in an environment
where its unknowns are defined. You can make a function:
```
expr = [x^2]
f x = [expr]
f 4 #=> 16
```

To keep quotes from evaluating known variables, specify the unknowns
explicitly: `[f(x); x]` reads "quote `f(x)` where `x` is the unknown.
When unknowns are listed explicitly, they must be exhaustive,
i.e. the rest can be generic stand-ins by must be eventually determined,
they are not implicitly understood as unknowns if they are not defined.
That is, `[x + y; x]` is an error if `y` is not defined.

Functions can take quotes, match against them, and construct new ones.
```
D [x^n; x] = n x^(n - 1)
D [sin(x); x] = cos(x)
D [cos(x); x] = -sin(x)
D [f (g x); x] = D[f(y)] D[g(x)] where y = g(x)
D [(f x) * (g x); x] = D[f(x)] g(x) + f(x) D[g(x)]
D [_] = error "Unknown derivative"
D _ = 0
f x0 = x0^3
f' x0 = D[f(x)] where x = x0
f' 1 #=> 2
```

In the above, `x` is automatically quoted as `[x]` in the RHS since its an unknown in the LHS.
Operations on a quoted expression results in another quote,
e.g. `[x + 2] + 3` evaluates to `[x + 2 + 3]` which continues to be evaluated to
`[x + 5]`.

Thus, the expression defining `f` earlier can be retrieved by passing
quoted variables `f ([x], [a])` becomes `[x^3 + 2x + a].

## TODO

 - [x] Temporary variables with `let ... in ...` and `... where ...` expressions.
 - [ ] Computed physical units with through postfix operators.
   - [ ] User defined units.
 - [ ] A `ref(.)` function, for referencing/aliasing other variables.
 - [ ] Throw errors on overflows until we implement bignums.
 - [ ] Imaginary numbers (using `complex.h`).
 - [x] User defined functions.
   - [x] Single argument.
   - [x] Tuple argument.
   - [x] Anonymous functions.
   - [x] Currying.
 - [ ] Tuple slicing with `a:b` range syntax.
 - [x] Tuple splat operator `(a, ...tup)`.
 - [ ] Overloading operators. Operations on tuples.
 - [ ] Pool constant and literal values in a preallocated structure, to save on reallocating constants by preventing them from being garbage collected.
 - [x] Garbage collection.
   - [x] Reference count function scopes.
   - [x] Reference count data values.
 - [ ] Numerical equation solver (polynomial, simultaneous, &c.).
 - [ ] Extend numbers to include “Big Numbers” (“Big Integers” and “Big Decimals”/Rationals), numbers a currently limited to ~80bit floats and pointer-sized (likely 64bit) integeres.
