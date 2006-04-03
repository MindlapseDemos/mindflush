obj := src/mlapse.o src/events.o\
	src/p_box.o
bin := mlapse

CXXFLAGS := -g -ansi -pedantic -Wall -Isrc -Isrc/3dengfx/src `src/3dengfx/3dengfx-config --cflags`
LIBS := `src/3dengfx/3dengfx-config --libs-no-3dengfx`

$(bin): $(obj) src/3dengfx/lib3dengfx.a
	$(CXX) -o $@ $(obj) src/3dengfx/lib3dengfx.a $(LIBS)

3dengfx/lib3dengfx.a:
	cd 3dengfx; $(MAKE) static

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
