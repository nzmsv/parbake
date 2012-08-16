INC=-I../include
CFLAGS=-Wall $(INC)
OBJS=emalloc.o poller.o linereader.o run.o

parbake.a: $(OBJS)
	$(AR) rs $@ $?

.PHONY : clean
clean :
	@rm parbake.a $(OBJS) 2>/dev/null || true
