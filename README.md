# BfLazyK

[Lazy K](https://tromp.github.io/cl/lazy-k.html) interpreter in [Brainfuck](https://en.wikipedia.org/wiki/Brainfuck).

lazyk.bf is generated from lazyk.c, using @shinh's [modified 8cc](https://github.com/shinh/8cc/tree/bfs) and [assembly -> Brainfuck translator](https://github.com/shinh/bflisp).

lazy2c is a "compiler" from Lazy K to C, which just bundles given Lazy K code with the interpreter. Combining [hs2lazy](https://github.com/irori/hs2lazy) and these together, you can compile Haskell programs into Lazy K, then into C, and then Brainfuck!
