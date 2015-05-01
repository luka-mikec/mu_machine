# standard library for mu_machine; arithmetics and logic #

# add: add two numbers #
add 0,      x = x
add y + 1,  x = sc add(y, x)

# mul: multiply two numbers #
mul 0,      x = z x
mul y + 1,  x = add(mul(y, x), x)

# pr: immediate predecessor of a number, with pr 0 := 0 #
pr_ 0,      x = z x
pr_ y + 1,  x = y
pr x = pr_(x x)

# sub: subtract y from x, with sub x y := 0 if y > x #
subinv 0,     x = x
subinv y + 1, x = pr subinv(y, x)
sub x y = subinv(y x)

# functions for constructing numbers, nN x = N, for N = 0, ..., 10
  to make larger numbers use cc, which concats decimal numbers; cc 5 2 = 52; cc cc 1 2 3 = 123, etc. #
n0 x = z x
n1 x = sc n0 x ## n2 x = sc n1 x ## n3 x = sc n2 x
n4 x = sc n3 x ## n5 x = sc n4 x ## n6 x = sc n5 x
n7 x = sc n6 x ## n8 x = sc n7 x ## n9 x = sc n8 x
n10 x = sc n9 x

cc x y = add mul(n10 x, x), y

# logical functions:    sg: signum (casts int to bool)
                        neg: !, and: &&, or: ||
                        cond: ->, iff: <->
  0 is false, everything else is true #
neg_ 0,     x = n1 x
neg_ y + 1, x = n0 x
neg  x = neg_(x x)
sg   x = neg neg x
and  x y = mul(sg x, sg y)
or   x y = neg and(neg x, neg y)
cond x y = or(neg x, y)
iff  x y = and(cond(x y), cond(y x))

# arithmetical relations, [gl]t[e]?, g = greater, l = less, e = equals #
gt  x y = sg sub(x y)
lt  x y = gt y x
gte x y = neg lt x y
lte x y = gte y x
eql x y = and lte(x y) lte(y x)
neql y x = or gt(x y) gt(y x)

# exp: exp x y = x^y, exp2 x = 2^x #
exp_ 0,     x  = n1 x
exp_ y + 1, x  = mul(exp_(y, x), x)
exp base power = exp_ power base
exp2 n = exp(n2 n, n)

# branch: branch cond if_true if_false = cond ? if_true : if_false #
branch con iftrue iffalse = add mul(sg con, iftrue) mul(neg con, iffalse)

# reverse pattern: reverse{f} x y = f y x #
{ reversee | reverse x y = reversee(y, x) }

# adapter pattern: adapter{f, g} x y = f(g(x y)) #
{ adaptee  adaptee_nth  | adapter  x y   = adaptee(adaptee_nth(x y))   }
{ adaptee3 adaptee_nth3 | adapter3 x y w = adaptee3(adaptee_nth3(x y w)) }

# restrict pattern: restrict{condition} x = x, if condition
                                            0, else       #
{ condition  | restrict  cnd a       = mul(sg condition (cnd),       a) }
{ condition2 | restrict2 cnd1 cnd2 a = mul(sg condition2(cnd1 cnd2), a) }

# various sum and product patterns, sum_arg{f} a z arg = f(a, arg) + ... + f(z, arg)
  important: f should first take the index and then arg (and nothing else), 
    otherwise use: sum{rev{f}} a z arg
  if your function takes 3+ arguments, use e.g.: sum{bind5{f, .1, .3}} a z arg #
{ merger act | 
    # f(0, arg) + ... + f(y, arg), where + is merge operation #
    accumulate_first_arg 0,     x = act(n0 x, x)
    accumulate_first_arg y + 1, x = merger(
        accumulate_first_arg(y,    x)
        act                 (sc y, x)
    )
    # f(0) + ... + f(y), where + is merge operation #
    accumulate_first x = accumulate_first_arg{merger, adapter{act .0}} x z(x)

    # f(offset, arg) + ... + f(y + offset, arg), where + is merge operation #
    accumulate_offset_arg 0,     offset, x = act(offset, x)
    accumulate_offset_arg y + 1, offset, x = merger(
        accumulate_offset_arg(y,    offset, x)
        act                  (add(offset, sc y) x)
    )

    # f(offset) + ... + f(y + offset), where + is merge operation #
    accumulate_offset y, offset = accumulate_offset_arg{merger, adapter{act .0}} y offset z(y)

    # f(l, arg) + ... + f(r, arg), where + is merge operation #
    accumulate_arg l r x  = restrict2{lte}(l, r,
        accumulate_offset_arg{merger, act} sub(r l) l x
    )
    
    # f(l) + ... + f(r), where + is merge operation #
    accumulate l r = accumulate_arg{merger, adapter{act .0}} l r z(l)
}

{ merger2 act2 | 
    # f(0, arg2) + ... + f(y, arg2), where + is merge operation #
    accumulate_first_arg2 0,     x0 x1 = act2(n0 x0, x0 x1)
    accumulate_first_arg2 y + 1, x0 x1 = merger2(
        accumulate_first_arg2(y,    x0 x1)
        act2                 (sc y, x0 x1)
    )
    
    # f(offset, arg2) + ... + f(y + offset, arg2), where + is merge operation #
    accumulate_offset_arg2 0,     offset, x0 x1 = act2(offset, x0 x1)
    accumulate_offset_arg2 y + 1, offset, x0 x1 = merger2(
        accumulate_offset_arg2(y, offset, x0 x1)
        act2                  (add(offset, sc y) x0 x1)
    )

    # f(l, arg2) + ... + f(r, arg2), where + is merge operation #
    accumulate_arg2 l r x0 x1  = restrict2{lte}(l, r,
        accumulate_offset_arg2{merger2, act2} sub(r l) l x0 x1
    )
}

{ merger3 act3 | 
    # f(0, arg3) + ... + f(y, arg3), where + is merge operation #
    accumulate_first_arg3 0,     x0 x1 x2 = act3(n0 x0, x0 x1 x2)
    accumulate_first_arg3 y + 1, x0 x1 x2 = merger3(
        accumulate_first_arg3(y,    x0 x1 x2)
        act3                 (sc y, x0 x1 x2)
    )
    
    # f(offset, arg3) + ... + f(y + offset, arg3), where + is merge operation #
    accumulate_offset_arg3 0,     offset, x0 x1 x2 = act3(offset, x0 x1 x2)
    accumulate_offset_arg3 y + 1, offset, x0 x1 x2 = merger3(
        accumulate_offset_arg3(y, offset, x0 x1 x2)
        act3                  (add(offset, sc y) x0 x1 x2)
    )

    # f(l, arg3) + ... + f(r, arg3), where + is merge operation #
    accumulate_arg3 l r x0 x1 x2  = restrict2{lte}(l, r,
        accumulate_offset_arg3{merger3, act3} sub(r l) l x0 x1 x2
    )
}

{ summand | 
    # f(0, arg) + ... + f(y, arg) #
    sum_first_arg y x = accumulate_first_arg{add, summand} y x

    # f(0) + ... + f(y) #
    sum_first y = accumulate_first{add, summand} y

    # f(offset, arg) + ... + f(y + offset, arg) #
    sum_offset_arg y offset x = accumulate_offset_arg{add, summand} y offset x

    # f(offset) + ... + f(y + offset) #
    sum_offset y offset = accumulate_offset{add, summand} y offset

    # f(l, arg) + ... + f(r, arg) #
    sum_arg l r x  = accumulate_arg{add, summand} l r x
    
    # f(l) + ... + f(r) #
    sum l r = accumulate{add, summand} l r
}

{ factor | 
    # f(0, arg) * ... * f(y, arg) #
    product_first_arg y x = accumulate_first_arg{add, factor} y x

    # f(0) * ... * f(y) #
    product_first y = accumulate_first{add, factor} y

    # f(offset, arg) * ... * f(y + offset, arg) #
    product_offset_arg y offset x = accumulate_offset_arg{add, factor} y offset x

    # f(offset) * ... * f(y + offset) #
    product_offset y offset = accumulate_offset{add, factor} y offset

    # f(l, arg) * ... * f(r, arg) #
    product_arg l r x  = accumulate_arg{add, factor} l r x
    
    # f(l) * ... * f(r) #
    product l r = accumulate{add, factor} l r
}

# inverse unbounded mu-operator imu{f} x = (mu y)(f(x y) > 0) (searches for truth) #
{ binary_mu_predicate |  imu x = mu{adapter{neg, binary_mu_predicate}} x }

# bounded mu-operator (without primitive recursion, for performance) 
  bmu{f} x limit = (mu y <= limit)(f(x y)) #
{ binary_bmu_predicate | 
   # if y < limit: returns sg f(x, y), else returns z #
   bmu_helper x limit y = branch lt(y limit), sg binary_bmu_predicate(x y), n0 x

   # bounded mu operator #
   bmu x limit = mu2{bmu_helper{binary_bmu_predicate}} x limit

   # bounded inverse mu operator #
   bimu x limit = bmu{adapter{neg, binary_bmu_predicate}} x limit
}
{ ternary_bmu_predicate | 
   # if y < limit: returns sg f(x0, x1, y), else returns z #
   bmu_helper3 x0 x1 limit y = branch lt(y limit), sg ternary_bmu_predicate(x0 x1 y), n0 x0

   # bounded mu operator #
   bmu3 x0 x1 limit = mu3{bmu_helper3{ternary_bmu_predicate}} x0 x1 limit

   # bounded inverse mu operator #
   bimu3 x0 x1 limit = bmu3{adapter3{neg, ternary_bmu_predicate}} x0 x1 limit
}

# div: floor(first/second) #
div_helper a b c = and gte(a mul(b c)) lt(a mul(b, sc c))
div x y = bimu3{div_helper} x y x

# bounded quantifiers # 
{ exist_property | 
    exists left right = sg sum{exist_property} left right
}

{ exist_property3 | 
    exists3 left right x0 x1 = sg accumulate_arg2{add, exist_property3} left right x0 x1
}

{ binder | bind x0 x1 x2 x3 x4 x5 x6 x7 x8 x9 = 


# uncomment one of these: #

# what's the smallest number equal to 7? #
# run a = imu{eql} n7(a) #

# complex pattern usage (haskell's fold equivalent) #
# run a c  = accumulate{mul, exp2} a c #

a_puta_b_je_c a b c = eql c, mul a b

# what's 3 + 5? #
run a b c d e le de = exists3{a_puta_b_je_c} le de b e

NAPRAVITI BINDOVE! eliminiraju sve ove blablaK verzij