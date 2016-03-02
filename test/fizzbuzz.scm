#!/usr/bin/gosh

(load "./lazier/lazier.scm")
(load "./lazier/prelude.scm")
(load "./lazier/churchnum.scm")

(lazy-def '(generate-fizz rest)
   '(cons 70 (cons 105 (cons 122 (cons 122 rest)))))

(lazy-def '(generate-buzz rest)
   '(cons 66 (cons 117 (cons 122 (cons 122 rest)))))

(lazy-def '(generate-number d1 d2 rest)
   '((lambda (digits)
       ((ifnonzero d1 (cons (if<= d1 9 (nth d1 digits) 256)) I)
        (cons (nth d2 digits) rest)))
     (list-from 48)))

(lazy-def '(generate fizz buzz d1 d2)
   '(((car fizz) I generate-fizz)
     (((car buzz) I generate-buzz)
      (((and (car fizz) (car buzz)) (generate-number d1 d2) I)
       (cons 10
             ((lambda (next)
                (if<= d2 8 (next d1 (1+ d2)) (next (1+ d1) 0)))
              (generate (cdr fizz) (cdr buzz))))))))

(lazy-def '(fizz-list n)
  '(if<= n 1 (cons #t (fizz-list (1+ n)))
             (cons #f (fizz-list 0))))

(lazy-def '(buzz-list n)
  '(if<= n 3 (cons #t (buzz-list (1+ n)))
             (cons #f (buzz-list 0))))

(lazy-def '(main input)
  '(generate (fizz-list 0) (buzz-list 0) 0 1))

(print-as-unlambda (laze 'main))

