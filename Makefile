
CXXFLAGS := -std=gnu++11 -Wall -Werror
SMALL_FLAGS := -DNDEBUG -DSMALL -Os

bitsy: bitsy.cc
	${CXX} ${CXXFLAGS} -g -O0 $< -o $@

bitsy.s: bitsy.cc
	${CXX} ${CXXFLAGS} -S -masm=intel -Os $< -o $@

# -m32 doesn't work for some reason
small: bitsy.cc
	${CXX} ${CXXFLAGS} $(SMALL_FLAGS) $< -o $@ #-Xlinker link-script #-m32
	#strip --strip-unneeded -R .comment -R .gnu.version -R .eh_frame_hdr -R .eh_frame $@

fast: bitsy.cc
	${CXX} ${CXXFLAGS} -g -DNDEBUG -O2 $< -o $@

clean:
	rm -f bitsy small fast bitsy.s
