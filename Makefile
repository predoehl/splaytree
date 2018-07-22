# $Id: Makefile 162 2018-07-17 00:13:27Z predoehl $

.PHONY = all clean pings svgs

CFLAGS += -std=c89
CFLAGS += -g3 -Wall -Wextra

TARGETS = driver1 driver2 driver3 cli

all: $(TARGETS)

driver1 driver2 driver3: %: %.o splay.o
	$(CC) -o $@ $^

cli: %: %.o splay.o
	$(CXX) -o $@ $^

splay.o driver1.o: splay.h

clean:
	$(RM) *.o *.gcno *.gcda *.gcov *.dot *.png *.svg $(TARGETS) 

pings:
	bash -c 'for x in *.dot ; do dot -Tpng -o "$${x%.dot}.png" "$$x"; done'

svgs:
	bash -c 'for x in *.dot ; do dot -Tsvg -o "$${x%dot}svg" "$$x"; done'

