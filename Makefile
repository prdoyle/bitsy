
bitsy: bitsy.go
	gccgo $< -g -Os -o $@

clean:
	rm -f bitsy
