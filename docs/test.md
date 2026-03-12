# Примеры грамматик для тестов

```txt
S -> P = E | w E d S
P -> I | I ( E A )
A -> e | , E A
E -> E + T | T
T -> T * F | F
F -> P | ( E )
I -> a | b | c
```

```txt
S -> t I = T B #
T -> i | r I : T B n
B -> e | ; I : T B
I -> a | b | c
```

```txt
Z -> S #
S -> P = E | i E t S | i E t S l S
P -> I | I ( E )
E -> E + T | T
T -> Τ * F | F
F -> P | (E)
I -> a | b
```

```txt
F -> f I ( I ) S #
S -> ; I = E S | e
E -> E * I | E + I | I
I -> a | b
```
