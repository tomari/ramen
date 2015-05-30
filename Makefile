ramen.prc: code0000.ramen.grc tFRM03e8.bin Talt03e9.bin MBAR03e8.bin\
		Talt03ea.bin Talt03eb.bin APPL0001.bin tAIN0064.bin \
		tver0001.bin tAIB03e8.bin tAIB03e9.bin tFRM044c.bin \
		tFRM07d0.bin Talt07d0.bin
	build-prc  ramen.prc "Ramen" Ramn *.grc *.bin

tFRM03e8.bin Talt03e9.bin MBAR03e8.bin Talt03ea.bin Talt03eb.bin APPL0001.bin tAIN0064.bin tver0001.bin tAIB03e8.bin tAIB03e9.bin tFRM044c.bin: ramen.rcp ramenrsc.h
				pilrc -noEllipsis ramen.rcp

tFRM07d0.bin Talt07d0.bin: ramenprefs.rcp ramenprefs.h
				pilrc -noEllipsis ramenprefs.rcp

ramen.rcp: ramen.rcp.in
		sed 's/RAMENVERSION/'`cat VERSION`'/g' ramen.rcp.in > ramen.rcp

code0000.ramen.grc: ramen.o ramenprefs.o
	m68k-palmos-gcc ramen.o ramenprefs.o -o ramen
	m68k-palmos-obj-res ramen

ramen.o: ramen.c ramenrsc.h ramen.h ramenprefs.h
	m68k-palmos-gcc -Wall -I. -Os -c ramen.c -o ramen.o

ramenprefs.o: ramenprefs.c ramenprefs.h ramenprefsrsc.h
	m68k-palmos-gcc -Wall -I. -Os -c ramenprefs.c -o ramenprefs.o

package: ramen.prc README README.jp
	rm -fr ramen-release-*
	mkdir ramen-release-`cat VERSION`
	cp ramen.prc README README.jp ramen-release-`cat VERSION`
	echo Simple timer for PalmOS, version `cat VERSION` | zip -9 -z ramen-`cat VERSION`.zip ramen-release-`cat VERSION`/*

clean:
	rm -f ramen
	rm -f *.grc
	rm -f *.bin
	rm -f *.o
	rm -f ramen.rcp
	rm -fr ramen-release-*
	rm -f ramen.prc
