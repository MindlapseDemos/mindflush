obj := src/mlapse.o src/events.o\
	src/p_box.o
bin := mlapse
eng_dir=src/3dengfx

CXXFLAGS := -g -ansi -pedantic -Wall -Isrc -I$(eng_dir)/src `$(eng_dir)/3dengfx-config --cflags`
LIBS := `$(eng_dir)/3dengfx-config --libs-no-3dengfx`

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
