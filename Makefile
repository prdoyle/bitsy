
bitsy: bitsy.cc
	${CXX} -g -O0 -Wall -Werror $< -o $@

# -m32 doesn't work for some reason
small: bitsy.cc
	${CXX} -DNDEBUG -Os -Wall -Werror $< -o $@
	#strip $@

fast: bitsy.cc
	${CXX} -DNDEBUG -O2 -Wall -Werror $< -o $@

clean:
	rm -f bitsy small fast
