.PHONY: all
all: ../fs/lnc/setdio

../fs/lnc/setdio: setdio.S
	mkdir -p ../fs/lnc
	nasm -f bin $< -o $@ && chmod +x $@

.PHONY: clean
clean:
	rm -rf *.o