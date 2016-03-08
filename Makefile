BFLISP ?= bflisp

lazyk.bf: lazyk.bfs
	(cd $(BFLISP) && ./bfcore.rb ../lazyk.bfs) > $@.tmp && mv $@.tmp $@

lazyk.bfs: lazyk.c $(BFLISP)/8cc/8cc $(BFLISP)/libf.h
	$(BFLISP)/8cc/8cc -S -I $(BFLISP) $< -o $@.tmp && mv $@.tmp $@

$(BFLISP)/README.md:
	rm -fr $(BFLISP).tmp
	git clone https://github.com/shinh/bflisp.git $(BFLISP).tmp
	mv $(BFLISP).tmp $(BFLISP)

$(BFLISP)/8cc/8cc: $(BFLISP)/README.md
	$(MAKE) -C $(BFLISP) 8cc/8cc

