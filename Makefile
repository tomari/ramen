.PHONY: clean
CC = m68k-palmos-gcc
OBJRES = m68k-palmos-obj-res
BUILDPRC = build-prc
PILRC = pilrc
CFLAGS = -Wall -Os -finline-functions -I.

resourcesmain = APPL0001.bin \
	MBAR03e8.bin \
	Talt03e9.bin \
	Talt03ea.bin \
	Talt03eb.bin \
	Talt03ec.bin \
	tAIB03e8.bin \
	tAIB03e9.bin \
	tAIN0064.bin \
	tFRM03e8.bin \
	tFRM044c.bin \
	tver0001.bin

resourcesprefs = tFRM07d0.bin \
	Talt07d0.bin

coderesources =	code0000.ramen.grc \
	code0001.ramen.grc \
	data0000.ramen.grc \
	pref0000.ramen.grc \
	rloc0000.ramen.grc

ramen.prc: $(coderesources) $(resourcesmain) $(resourcesprefs)
	$(BUILDPRC) $@ "Ramen" Ramn $^

$(resourcesmain): ramen.rcp ramenrsc.h
	$(PILRC) -noEllipsis $<

$(resourcesprefs): ramenprefs.rcp
	$(PILRC) -noEllipsis $<

ramen.rcp: ramen.rcp.in VERSION
	sed 's/RAMENVERSION/'`cat VERSION`'/g' $< > $@

$(coderesources): ramen
	$(OBJRES) $<

ramen: ramen.o ramenprefs.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

ramen.o: ramen.c ramenrsc.h ramen.h
	$(CC) -c $(CFLAGS) -o $@ $<

ramenprefs.o: ramenprefs.c ramenprefsrsc.h ramen.h
	$(CC) -c $(CFLAGS) -o $@ $<

package: ramen.prc README README.jp VERSION
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
