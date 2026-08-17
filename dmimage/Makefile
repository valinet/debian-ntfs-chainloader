.PHONY: all
all: ../fs/lnc/dmimage

../fs/lnc/dmimage: custom.ld dmimage.o syscall.o
	mkdir -p ../fs/lnc
	$(LD) $(filter-out $<,$^) -o $@ -T $< --strip-all -z noexecstack

dmimage.o: dmimage.c
	$(CC) -Wall -c -Os -fno-builtin -fno-ident -fno-asynchronous-unwind-tables -fno-stack-protector -fomit-frame-pointer -fno-pie -mno-sse $< -o $@

syscall.o: syscall.S
	as $< -o $@

.PHONY: clean
clean:
	rm -rf *.o