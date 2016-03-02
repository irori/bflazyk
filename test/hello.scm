#!/usr/bin/gosh

(load "./lazier/lazier.scm")
(load "./lazier/prelude.scm")
(load "./lazier/churchnum.scm")

(lazy-def '(hello input)
 '(cons 72 (cons 101 (cons 108 (cons 108 (cons 111 (cons 44 (cons 32 (cons 119 (cons 111 (cons 114 (cons 108 (cons 100 (cons 33 end-of-output))))))))))))))

(print-as-unlambda (laze 'hello))
