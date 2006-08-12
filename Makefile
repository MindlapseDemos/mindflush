obj := src/mlapse.o\
	src/events.o\
	src/sdlvf.o\
	src/p_box.o\
	src/p_plat.o\
	src/p_intro.o\
	src/p_tunnel.o\
	src/p_dcurve.o\
	src/p_cyl.o
bin := mlapse
eng_dir=src/3dengfx

CFLAGS := -O3 -std=c89 -pedantic -Wall `sdl-config --cflags`
CXXFLAGS := -g -ansi -pedantic -Wall -Isrc -I$(eng_dir)/src `$(eng_dir)/3dengfx-config --cflags`
LIBS := `$(eng_dir)/3dengfx-config --libs-no-3dengfx` -lvorbisfile

$(bin): $(obj) $(eng_dir)/lib3dengfx.a
	$(CXX) -o $@ $(obj) $(eng_dir)/lib3dengfx.a $(LIBS)

$(eng_dir)/lib3dengfx.a:
	@echo -------------------------------------------------------------------------------
	@echo I will try to compile 3dengfx. In case of failure you may want to try manually,
	@echo by changing into the ``$(eng_dir)'' directory and performing the steps:
	@echo \$$ ./configure --disable-debug --with-gfxlib=sdl
	@echo \$$ make static
	@echo -------------------------------------------------------------------------------
	cd $(eng_dir); ./configure --disable-debug --with-gfxlib=sdl; $(MAKE) static

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
