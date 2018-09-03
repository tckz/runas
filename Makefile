.PHONY: all clean
.SUFFIXES: .cxx .o

TARGETS = runas

OBJS_RUNAS = runas.o

all: $(TARGETS)
	@echo $@ done.

clean:
	/bin/rm -f $(TARGETS) $(OBJS_RUNAS)
	@echo $@ done.

.cxx.o:
	g++ -c -Wall -std=c++11 $^

runas: $(OBJS_RUNAS)
	/bin/rm -f $@
	g++ -o $@ $^
	sudo sh -c "chown root:`id -g root` $@ && chmod u+s $@"
