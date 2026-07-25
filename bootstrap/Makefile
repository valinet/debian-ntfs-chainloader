.PHONY: all
all: ../fs/lnc/bootstrap

../fs/lnc/bootstrap: custom.ld bootstrap.o syscall.o
	mkdir -p ../fs/lnc
	$(LD) $(filter-out $<,$^) -o $@ -T $< --strip-all -z noexecstack

bootstrap.o: bootstrap.c
	$(CC) -Wall -c -Os -fno-builtin -fno-ident -fno-asynchronous-unwind-tables -fno-stack-protector -fomit-frame-pointer -fno-pie $< -o $@

syscall.o: syscall.S
	as $< -o $@

.PHONY: clean
clean:
	rm -rf *.o