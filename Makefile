
bitsy: bitsy.go
	gccgo $< -g -o $@

clean:
	rm -f bitsy
