.DEFAULT_GOAL=latc_x86_64

latc_x86_64: src lib
	+$(MAKE) -C src
	+$(MAKE) -C lib
	mv src/$@ .

clean:
	+$(MAKE) -C src clean
	+$(MAKE) -C lib clean
	rm -f latc_x86_64 >/dev/null 2>/dev/null
