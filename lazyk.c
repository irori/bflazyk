/*
 *  Lazy K interpreter
 *
 *  Copyright 2008-2016 irori <irorin@gmail.com>
 *  This is free software. You may modify and/or distribute it under the
 *  terms of the GNU General Public License, version 2 or any later version.
 *  It comes with no warranty.
 */
#ifdef __8cc__
#include "libf.h"
#define EOF -1
#else
#include <stdio.h>
#include <stdlib.h>
#define print_int(n) printf("%d", n)
#define print_str(s) fputs(s, stdout)
#endif

#define HEAP_SIZE 8*1024
#define RDSTACK_SIZE	4096

/**********************************************************************
 *  Storage management
 **********************************************************************/

struct tagPair;
typedef struct tagPair *Cell;
#define CELL(x)	((Cell)(x))

/* combinator */
#define mkcomb(n)	CELL(n)
#define COMB_S		mkcomb(0)
#define COMB_K		mkcomb(1)
#define COMB_I		mkcomb(2)
#define COMB_IOTA	mkcomb(3)
#define COMB_KI		mkcomb(4)
#define COMB_READ	mkcomb(5)
#define COMB_WRITE	mkcomb(6)
#define COMB_INC	mkcomb(7)
#define COMB_CONS	mkcomb(8)
#define COMB_MAX        ((unsigned)COMB_CONS)

/* immediate objects */
#define IMM_MIN         (COMB_MAX + 1)
#define mkimm(n)	CELL(IMM_MIN + (n))
#define NIL		mkimm(0)
#define COPIED		mkimm(1)
#define UNUSED_MARKER	mkimm(2)
#define IMM_MAX         ((unsigned)UNUSED_MARKER)

/* integer */
#define INT_MIN         (IMM_MAX + 1)
#define INT_MAX         (INT_MIN + 256)
#define isint(c)	((unsigned)(c) >= INT_MIN && (unsigned)(c) <= INT_MAX)
#define mkint(n)	CELL(INT_MIN + (n))
#define intof(c)	((unsigned)(c) - INT_MIN)

/* character */
#define CHAR_MIN         (INT_MAX + 1)
#define CHAR_MAX         (CHAR_MIN + 255)
#define ischar(c)	((unsigned)(c) >= CHAR_MIN && (unsigned)(c) <= CHAR_MAX)
#define mkchar(n)	CELL(CHAR_MIN + (n))
#define charof(c)	((unsigned)(c) - CHAR_MIN)

/* pair */
typedef struct tagPair {
    Cell car;
    Cell cdr;
} Pair;
#define ispair(c)	((unsigned)(c) > CHAR_MAX)
#define car(c)		((c)->car)
#define cdr(c)		((c)->cdr)
#define SET(c,fst,snd)  ((c)->car = (fst), (c)->cdr = (snd))


Pair *heap_area, *free_ptr;

void gc_run(Cell *save1, Cell *save2);
void rs_copy(void);
Cell copy_cell(Cell c);

#define ERROR(s) (puts(s), exit(1))

void storage_init()
{
    heap_area = calloc(sizeof(Pair) * HEAP_SIZE, 4);
    if (heap_area == NULL)
	ERROR("Cannot allocate heap storage");
    if (!ispair(heap_area))
        ERROR("heap_area must be larger than CHAR_MAX");
    
    free_ptr = heap_area;
    heap_area += HEAP_SIZE;
}

Cell pair(Cell fst, Cell snd)
{
    Cell c;
    if (free_ptr >= heap_area)
	gc_run(&fst, &snd);

    c = free_ptr;
    free_ptr += 1;
    car(c) = fst;
    cdr(c) = snd;
    return c;
}

Cell alloc(int n)
{
    Cell p;
    if (free_ptr + n > heap_area)
	gc_run(NULL, NULL);

    p = free_ptr;
    free_ptr += n;
    return p;
}


void gc_run(Cell *save1, Cell *save2)
{
    static Pair* free_area = NULL;
    int num_alive;
    Pair *scan;

    puts("GC");

    if (free_area == NULL) {
	free_area = calloc(sizeof(Pair) * HEAP_SIZE, 4);
	if (free_area == NULL)
	    ERROR("Cannot allocate heap storage\n");
    }

    free_ptr = scan = free_area;
    free_area = heap_area - HEAP_SIZE;
    heap_area = free_ptr + HEAP_SIZE;

    rs_copy();
    if (save1)
	*save1 = copy_cell(*save1);
    if (save2)
	*save2 = copy_cell(*save2);

    while (scan < free_ptr) {
	car(scan) = copy_cell(car(scan));
	cdr(scan) = copy_cell(cdr(scan));
	scan++;
    }

    num_alive = free_ptr - (heap_area - HEAP_SIZE);
}

Cell copy_cell(Cell c)
{
    Cell r;

    if (!ispair(c))
	return c;
    if (car(c) == COPIED)
	return cdr(c);

    r = free_ptr++;
    car(r) = car(c);
    if (car(c) == COMB_I) {
	Cell tmp = cdr(c);
	while (ispair(tmp) && car(tmp) == COMB_I)
	    tmp = cdr(tmp);
	cdr(r) = tmp;
    }
    else
	cdr(r) = cdr(c);
    car(c) = COPIED;
    cdr(c) = r;
    return r;
}

/**********************************************************************
 *  Reduction Machine
 **********************************************************************/

typedef struct {
    Cell *sp;
    Cell stack[RDSTACK_SIZE];
} RdStack;

RdStack rd_stack;

void rs_init(void)
{
    int i;
    rd_stack.sp = rd_stack.stack + RDSTACK_SIZE;

    for (i = 0; i < RDSTACK_SIZE; i++)
	rd_stack.stack[i] = UNUSED_MARKER;
}

void rs_copy(void)
{
    Cell *c;
    for (c = rd_stack.stack + RDSTACK_SIZE - 1; c >= rd_stack.sp; c--)
	*c = copy_cell(*c);
}

void rs_push(Cell c)
{
    if (rd_stack.sp <= rd_stack.stack)
	ERROR("runtime error: stack overflow\n");
    *--rd_stack.sp = c;
}

#define TOP		(*rd_stack.sp)
#define POP		(*rd_stack.sp++)
#define PUSH(c)		rs_push(c)
#define PUSHED(n)	(*(rd_stack.sp+(n)))
#define DROP(n)		(rd_stack.sp += (n))
#define ARG(n)		cdr(PUSHED(n))
#define APPLICABLE(n)	(bottom - rd_stack.sp > (n))

/**********************************************************************
 *  Loader
 **********************************************************************/

int g_buf = -1;

int getChar(void) {
    int r;
    if (g_buf != -1) {
        r = g_buf;
        g_buf = -1;
    } else {
        r = getchar();
        //if (r == -1)
        //    exit(0);
    }
    return r;
}

void ungetChar(int c) {
    g_buf = c;
}

Cell read_one(int i_is_iota);
Cell read_many(int closing_char);

Cell load_program(void)
{
    return read_many(EOF);
}

int next_char(void)
{
    int c;
    do {
	c = getChar();
	if (c == '#') {
	    while (c = getChar(), c != '\n' && c != EOF)
		;
	}
    } while (c == ' ' || c == '\n' || c == '\t');
    return c;
}

Cell read_many(int closing_char)
{
    int c;
    Cell obj;

    c = next_char();
    if (c == closing_char)
	return COMB_I;
    ungetChar(c);

    PUSH(read_one(0));
    while ((c = next_char()) != closing_char) {
	ungetChar(c);
	obj = read_one(0);
	obj = pair(TOP, obj);
	TOP = obj;
    }
    return POP;
}

Cell read_one(int i_is_iota)
{
    int c;
    Cell obj;

    c = next_char();
    switch (c) {
    case '`': case '*':
	PUSH(read_one(c == '*'));
	obj = read_one(c == '*');
	obj = pair(TOP, obj);
	POP;
	return obj;
    case '(':
	obj = read_many(')');
	return obj;
    case 's': case 'S': return COMB_S;
    case 'k': case 'K': return COMB_K;
    case 'i': return i_is_iota ? COMB_IOTA : COMB_I;
    case 'I': return COMB_I;
    case '0': case '1': {
	obj = COMB_I;
	do {
	    if (c == '0')
		obj = pair(pair(obj, COMB_S), COMB_K);
	    else
		obj = pair(COMB_S, pair(COMB_K, obj));
	    c = next_char();
	} while (c == '0' || c == '1');
	ungetChar(c);
	return obj;
    }
    case EOF:
	ERROR("parse error: unexpected EOF\n");
    default:
	ERROR("parse error\n");
    }
}

/**********************************************************************
 *  Printer
 **********************************************************************/

void print(Cell e)
{
    if (ispair(e)) {
        putchar('(');
        print(car(e));
        print_str(" . ");
        print(cdr(e));
        putchar(')');
    } else if (isint(e)) {
        print_int(intof(e));
    } else if (ischar(e)) {
        putchar('\'');
        putchar(charof(e));
        putchar('\'');
    } else {
        switch ((unsigned)e) {
        case 0: putchar('S'); break;
        case 1: putchar('K'); break;
        case 2: putchar('I'); break;
        case 3: print_str("iota"); break;
        case 4: print_str("KI"); break;
        case 5: putchar('@'); break;
        case 6: putchar('!'); break;
        case 7: putchar('+'); break;
        case 8: putchar(':'); break;
        case 9: putchar('v'); break;
        case 10: putchar('?'); break;
        case 11: putchar('#'); break;
        default: ERROR("unknown cell value");
        }
    }
}

/**********************************************************************
 *  Reducer
 **********************************************************************/

void eval(Cell root)
{
    Cell *bottom = rd_stack.sp;
    PUSH(root);

    for (;;) {
	while (ispair(TOP))
	    PUSH(car(TOP));

	if (TOP == COMB_I && APPLICABLE(1))
	{ /* I x -> x */
	    POP;
	    TOP = cdr(TOP);
	}
	else if (TOP == COMB_S && APPLICABLE(3))
	{ /* S f g x -> f x (g x) */
	    Cell a = alloc(2);
	    SET(a+0, ARG(1), ARG(3));	/* f x */
	    SET(a+1, ARG(2), ARG(3));	/* g x */
	    DROP(3);
	    SET(TOP, a+0, a+1);	/* f x (g x) */
	}
	else if (TOP == COMB_K && APPLICABLE(2))
	{ /* K x y -> I x */
	    Cell x = ARG(1);
	    DROP(2);
	    SET(TOP, COMB_I, x);
	    TOP = cdr(TOP);	/* shortcut reduction of I */
	}
	else if (TOP == COMB_IOTA && APPLICABLE(1))
	{ /* IOTA x -> x S K */
	    Cell xs = pair(ARG(1), COMB_S);
	    POP;
	    SET(TOP, xs, COMB_K);
	}
	else if (TOP == COMB_KI && APPLICABLE(2))
	{ /* KI x y -> I y */
	    DROP(2);
	    car(TOP) = COMB_I;
	}
	else if (TOP == COMB_CONS && APPLICABLE(3))
	{ /* CONS x y f -> f x y */
	    Cell fx, y;
	    fx = pair(ARG(3), ARG(1));
	    y = ARG(2);
	    DROP(3);
	    SET(TOP, fx, y);
	}
	else if (TOP == COMB_READ && APPLICABLE(2))
	{ /* READ NIL f -> CONS CHAR(c) (READ NIL) f */
	    int c = getchar();
	    Cell a = alloc(2);
	    SET(a+0, COMB_CONS, mkchar(c == EOF ? 256 : c));
	    SET(a+1, COMB_READ, NIL);
	    POP;
	    SET(TOP, a+0, a+1);
	}
	else if (TOP == COMB_WRITE && APPLICABLE(1))
	{ /* WRITE x -> putc(eval((car x) INC NUM(0))); WRITE (cdr x) */
	    Cell a = alloc(3);
	    SET(a+0, ARG(1), COMB_K);	/* (car x) */
	    SET(a+1, a+0, COMB_INC);	/* (car x) INC */
	    SET(a+2, a+1, mkint(0));	/* (car x) INC NUM(0) */
	    POP;
	    eval(a+2);

	    if (!isint(TOP))
		ERROR("invalid output format (result was not a number)\n");
	    if (intof(TOP) >= 256)
		return;

	    putchar(intof(TOP));
	    POP;
	    a = pair(cdr(TOP), COMB_KI);
	    cdr(TOP) = a;
	}
	else if (TOP == COMB_INC && APPLICABLE(1))
	{ /* INC x -> eval(x)+1 */
            int n;
	    Cell c = ARG(1);
	    POP;
	    eval(c);

	    c = POP;
	    if (!isint(c))
		ERROR("invalid output format (attempted to apply inc to a non-number)\n");
            n = intof(c) + 1;
            if (n > 256)
                n = 256;
	    SET(TOP, COMB_I, mkint(n));
	}
	else if (ischar(TOP) && APPLICABLE(2)) {
	    int c = charof(TOP);
	    if (c <= 0) {  /* CHAR(0) f z -> z */
		Cell z = ARG(2);
		DROP(2);
		SET(TOP, COMB_I, z);
	    }
	    else {       /* CHAR(n+1) f z -> f (CHAR(n) f z) */
		Cell a = alloc(2);
		Cell f = ARG(1);
		SET(a+0, mkchar(c-1), f);	/* CHAR(n) f */
		SET(a+1, a+0, ARG(2));		/* CHAR(n) f z */
		DROP(2);
		SET(TOP, f, a+1);		/* f (CHAR(n) f z) */
	    }
	}
	else if (isint(TOP) && APPLICABLE(1))
	    ERROR("invalid output format (attempted to apply a number)\n");
	else
	    return;
    }
}

void eval_print(Cell root)
{
    eval(pair(COMB_WRITE,
	      pair(root,
		   pair(COMB_READ, NIL))));
}

/**********************************************************************
 *  Main
 **********************************************************************/

int main()
{
    Cell root;
    
    storage_init();
    rs_init();

    root = load_program();

    //print(root);
    eval_print(root);

    return 0;
}
