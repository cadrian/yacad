#!/usr/bin/make -f

PROJECT=yacad
COMMON_OBJ=$(shell { find src/common -name '*.c'; ls src/*.c; } | sed -r 's|^src/|target/out/|g;s|\.c|.o|g')
CORE_OBJ=$(shell find src/core -name '*.c' | sed -r 's|^src/|target/out/|g;s|\.c|.o|g')
RUNNER_OBJ=$(shell find src/runner -name '*.c' | sed -r 's|^src/|target/out/|g;s|\.c|.o|g')

ifeq "$(wildcard ../libcad)" ""
LIBCAD=
LIBCADINCLUDE=
LIBCADCLEAN=
GENDOC=/usr/share/libcad/gendoc.sh
else
ifeq "$(wildcard /etc/setup/setup.rc)" ""
LIBCAD=target/libcad.so
else
LIBCAD=target/cygcad.dll
endif
LIBCADINCLUDE=-I ../libcad/include
LIBCADCLEAN=libcadclean
GENDOC=$(shell cd ../libcad; echo $$(pwd)/gendoc.sh)
endif

ifeq "$(wildcard ../yacjp)" ""
LIBYACJP=
LIBYACJPINCLUDE=
LIBYACJPCLEAN=
else
ifeq "$(wildcard /etc/setup/setup.rc)" ""
LIBYACJP=target/libyacjp.so
else
LIBYACJP=target/cygyacjp.dll
endif
LIBYACJPINCLUDE=-I ../yacjp/include
LIBYACJPCLEAN=libyacjpclean
endif

CFLAGS ?= -g -D_GNU_SOURCE
LDFLAGS ?=

ifeq "$(wildcard /usr/bin/doxygen)" ""
all: exe
else
all: exe doc
endif
	@echo

clean: $(LIBCADCLEAN) $(LIBYACJPCLEAN)
	rm -rf target debian
	rm -f src/_exp_entry_registry.c

exe: target/$(PROJECT)_core target/$(PROJECT)_runner

target/$(PROJECT)_core: $(COMMON_OBJ) $(CORE_OBJ) $(LIBCAD) $(LIBYACJP)
	@echo "Compiling executable: $@"
	$(CC) $(CFLAGS) -o $@ $(COMMON_OBJ) $(CORE_OBJ) -Wall -Werror -L target -lcad -lyacjp -lm -lgit2 -lsqlite3 -lzmq -lpthread

target/$(PROJECT)_runner: $(COMMON_OBJ) $(RUNNER_OBJ) $(LIBCAD) $(LIBYACJP)
	@echo "Compiling executable: $@"
	$(CC) $(CFLAGS) -o $@ $(COMMON_OBJ) $(RUNNER_OBJ) -Wall -Werror -L target -lcad -lyacjp -lm -lgit2 -lsqlite3 -lzmq -lpthread

target/out/%.o: src/%.c src/*.h Makefile
	mkdir -p $(shell dirname $@)
	@echo "Compiling object: $<"
	$(CC) $(CFLAGS) -I src $(LIBCADINCLUDE) $(LIBYACJPINCLUDE) -Wall -Werror -c $< -o $@

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# libcad

libcadclean:
	cd ../libcad && make clean

target/libcad.so:
	mkdir -p target
	cd ../libcad && make lib
	cp ../libcad/target/libcad.so* target/

target/cygcad.dll: target/libcad.dll.a
	mkdir -p target
	cp ../libcad/target/cygcad.dll target/

target/libcad.dll.a:
	mkdir -p target
	cd ../libcad && make lib
	cp ../libcad/target/libcad.dll.a target/

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# libyacjp

libyacjpclean:
	cd ../yacjp && make clean

target/libyacjp.so:
	mkdir -p target
	cd ../yacjp && make lib
	cp ../yacjp/target/libyacjp.so* target/

target/cygyacjp.dll: target/libyacjp.dll.a
	mkdir -p target
	cp ../yacjp/target/cygyacjp.dll target/

target/libyacjp.dll.a:
	mkdir -p target
	cd ../yacjp && make lib
	cp ../yacjp/target/libyacjp.dll.a target/

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# release

install: exe doc target/version
	mkdir -p debian/tmp/usr/bin
	mkdir -p debian/tmp/usr/share/$(PROJECT)
	mkdir -p debian/tmp/usr/share/doc/$(PROJECT)-doc
	-cp target/$(PROJECT)_core debian/tmp/usr/bin/
	-cp target/$(PROJECT)_runner debian/tmp/usr/bin/
	-cp -a target/*.pdf target/doc/html debian/tmp/usr/share/doc/$(PROJECT)-doc/

release.main: target/dpkg/release.main release.doc

target/dpkg/release.main: exe target/version
	@echo "Releasing main version $(shell cat target/version)"
	debuild -us -uc -nc -F
	mkdir -p target/dpkg
	mv ../$(PROJECT)*_$(shell cat target/version)_*.deb    target/dpkg/
	mv ../$(PROJECT)_$(shell cat target/version).dsc       target/dpkg/
	mv ../$(PROJECT)_$(shell cat target/version).tar.[gx]z target/dpkg/
	mv ../$(PROJECT)_$(shell cat target/version)_*.build   target/dpkg/
	mv ../$(PROJECT)_$(shell cat target/version)_*.changes target/dpkg/
ifeq "$(wildcard /usr/bin/doxygen)" ""
	(cd target && tar cfz $(PROJECT)_$(shell cat target/version)_$(shell gcc -v 2>&1 | grep '^Target:' | sed 's/^Target: //').tgz $(PROJECT)_core $(PROJECT)_runner dpkg)
else
	(cd target && tar cfz $(PROJECT)_$(shell cat target/version)_$(shell gcc -v 2>&1 | grep '^Target:' | sed 's/^Target: //').tgz $(PROJECT)_core $(PROJECT)_runner $(PROJECT).pdf $(PROJECT)-htmldoc.tgz dpkg)
endif
	touch target/dpkg/release.main

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# doc (copied from libcad with a few changes)

ifeq "$(wildcard /usr/bin/doxygen)" ""
release.doc:
else
release.doc: target/dpkg/release.doc

target/dpkg/release.doc: doc
	touch target/dpkg/release.doc

doc: target/$(PROJECT).pdf target/$(PROJECT)-htmldoc.tgz

target/$(PROJECT).pdf: target/doc/latex/refman.pdf
	@echo "	   Saving PDF"
	cp $< $@

target/doc/latex/refman.pdf: target/doc/latex/Makefile target/doc/latex/version.tex
	@echo "	 Building PDF"
#	remove the \batchmode on the first line:
	mv target/doc/latex/refman.tex target/doc/latex/refman.tex.orig
	tail -n +2 target/doc/latex/refman.tex.orig > target/doc/latex/refman.tex
	find target/doc/latex -name \*.tex -exec sed -r 's/(section|paragraph)\*/\1/g' -i {} \;
	-yes x | $(MAKE) -C target/doc/latex > target/doc/make.log 2>&1
#	cat target/doc/latex/refman.log

target/$(PROJECT)-htmldoc.tgz: target/doc/html/index.html
	@echo "	 Building HTML archive"
	(cd target/doc/html; tar cfz - *) > $@

target/doc/latex/version.tex: target/version
	cp $< $@

target/doc/latex/Makefile: target/doc/.doc
	sleep 1; touch $@

target/doc/html/index.html: target/doc/.doc
	sleep 1; touch $@

target/doc/.doc: Doxyfile target/gendoc.sh $(shell find src -name '*.[ch]') $(shell ls -1 doc/*) Makefile
	@echo "Generating documentation"
	target/gendoc.sh
	-doxygen -u $<
	doxygen $< && touch $@

target/gendoc.sh:
	mkdir -p target/doc target/gendoc
	ln -sf $(GENDOC) $@
endif

target/version: debian/changelog
	head -n 1 debian/changelog | awk -F'[()]' '{print $$2}' > $@

debian/changelog: debian/changelog.raw
	sed "s/#DATE#/$(shell date -R)/;s/#SNAPSHOT#/$(shell date -u +'~%Y%m%d%H%M%S')/" < $< > $@

debian/changelog.raw:
	./build/build.sh

.PHONY: all clean libcadclean doc install release.main release.doc
