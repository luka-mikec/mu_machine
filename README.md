# mu_machine
μ-recursive functions execution.

Requires:
 - C++ compiler (GCC)
 - Boost (Tokenizer library)
 - QT (but can be used separately)
 
 
Notes:
 - all commas are optional, and can be added anywhere (f,(x y) = ,Sc(x,,,) is valid code)

----

Planned:
 - saving and manipulating numbers in factorized form
 - optimizations:
   - if induction step case doesn't depend on f(y ...), run just the y-th iteration 
 
---- 
 
Builtin functions:
 - sc : N -> N, Sc(x) = x + 1
 - (.0, .1, .2, ...) .k : N -> N, .k(x_0 ... x_n) = x_k (requires k < n)
 - z : N -> N, z(x) = 0
 
 
----

- *Writing functions, basic case*

  function_name var_1 ... var_n = expression
 
  Expressions are defined as:
    expression ::= bounded variable | function_name(expression_1 ... expression_k)
    
  For example, f x y z = .1(z .0(y))
  
  
- *Writing functions, primitive recursion case*
  - Base case:

    f 0 var_1 ... var_n = expression 
  
  - Induction step case:

    f y + 1 var_1 ... var_n = expression
  
  Base case *must* appear before induction step case. 
  Induction step case can only refer to f by f(y, x). 
  The name and the position of 'y' is fixed, but you can always create a wrapper function:

  g something counter something_else = f counter something something_else

- *Macros*
   Syntax: { macro_parameters | statement1 ... statementn } (see example)


