
CXXFLAGS := -std=gnu++11 -Wall -Werror

bitsy: bitsy.cc
	${CXX} ${CXXFLAGS} -g -O0 $< -o $@

# -m32 doesn't work for some reason
small: bitsy.cc
	${CXX} ${CXXFLAGS} -m32 -DNDEBUG -DSMALL -Os $< -o $@ #-Xlinker link-script
	#strip --strip-unneeded -R .comment -R .gnu.version -R .eh_frame_hdr -R .eh_frame $@

fast: bitsy.cc
	${CXX} ${CXXFLAGS} -g -DNDEBUG -O2 $< -o $@

clean:
	rm -f bitsy small fast
