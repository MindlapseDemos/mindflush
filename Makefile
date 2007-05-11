obj = src/mlapse.o\
	src/events.o\
	src/p_box.o\
	src/p_plat.o\
	src/p_intro.o\
	src/p_tunnel.o\
	src/p_dcurve.o\
	src/p_cyl.o\
	src/p_greets.o\
	src/p_credits.o\
	src/sdlvf.o
bin = mlfc
eng_dir = src/3dengfx

CFLAGS = -m32 -O3 -std=c89 -pedantic -Wall `sdl-config --cflags`
CXXFLAGS = -m32 -O3 -ansi -pedantic -Wall -Isrc -I$(eng_dir)/src `$(eng_dir)/3dengfx-config --cflags`
LIBS = -L/usr/lib32 `$(eng_dir)/3dengfx-config --libs-no-3dengfx` -lvorbisfile

eng_cfg_cmd = ./configure --disable-debug --with-gfxlib=sdl --disable-ft

.PHONY: all
all: $(bin) demoscript

$(bin): $(obj) $(eng_dir)/lib3dengfx.a
	$(CXX) $(CFLAGS) -o $@ $(obj) $(eng_dir)/lib3dengfx.a $(LIBS)

demoscript: demoscript.h
	tools/genscript

gui/guicfg: gui/cfg.c
	cd gui; $(MAKE)

$(eng_dir)/lib3dengfx.a:
	@echo -------------------------------------------------------------------------------
	@echo I will try to compile 3dengfx. In case of failure you may want to try manually,
	@echo by changing into the ``$(eng_dir)'' directory and performing the steps:
	@echo \$$ $(eng_cfg_cmd)
	@echo \$$ make static
	@echo -------------------------------------------------------------------------------
	cd $(eng_dir); $(eng_cfg_cmd); $(MAKE) static

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
