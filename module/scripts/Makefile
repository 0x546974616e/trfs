# :set noexpandtab

BINS = tryread trywrite

CSRC = $(wildcard *.c)
COBJ = $(CSRC:.c=.o)
CDEP = $(CSRC:.c=.d)

CC = gcc

CFLAGS = \
	-Wall \
	-Wextra \
	-Wconversion \
	-Werror \
	-pedantic
# -ansi

CDEPS = -MMD -MP -MT $@ -MF $(@:.o=.d)

LDFLAGS =

.PHONY : all clean cleanall mrproper

all : $(BINS)
	@echo "-->" $(addprefix ./,$(notdir $^))

# It is not perfect because all objects are dependencies, but OK so far.
# https://www.gnu.org/software/make/manual/make.html#Static-Pattern
$(BINS) : % : %.o $(filter-out $(BINS:%=%.o), $(COBJ))
	@echo Generating $@...
	@$(CC) $^ -o $@ $(LDFLAGS)

%.o : %.c
	@echo $(notdir $<)
	@$(CC) -c $< -o $@ $(CFLAGS) $(CDEPS)

-include $(CDEP)

clean :
	@rm -f $(COBJ)

cleanall : clean
	@rm -f $(CDEP)

mrproper : cleanall
	@rm -f $(BINS)
