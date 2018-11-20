OBJDIR = obj
DEPS = deps
EXEDIR = unix

OBJS =	aeddum.o aedit.o aedplm.o block.o calc.o cctrap.o cmnds.o comand.o \
	   	conf.o consol.o find.o io.o macrof.o main.o pub.o screen.o setcmd.o \
		start.o tags.o tmpman.o unix.o util.o verv.o view.o words.o

CFLAGS = -O2 -I. -Wunused-variable
DEFS = -DUNIX

.PHONY: all
all: $(OBJDIR) $(DEPS) $(EXEDIR) $(EXEDIR)/aedit

$(OBJDIR) $(DEPS) $(EXEDIR): ; mkdir -p $@

$(EXEDIR)/aedit: $(addprefix $(OBJDIR)/,$(OBJS))
	$(CC) $(CFLAGS) -o $@ $^


$(OBJDIR)/%.o : %.c
	$(COMPILE.c) $(DEFS) -MP -MMD -MF $(DEPS)/$*.d -o $@ $<

clean:
	rm -f $(OBJDIR)/* $(DEPS)/*


-include $(DEPS)/$(OBJS:.o=.d)
