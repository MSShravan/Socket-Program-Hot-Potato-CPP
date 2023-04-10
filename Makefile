TARGETS=ringmaster player
FLAGS=-g -pedantic -Werror -Wall

all: $(TARGETS)
clean:
	rm -f $(TARGETS)

ringmaster: ringmaster.cpp potato.h utility.h
	g++ $(FLAGS) -o $@ $<

player: player.cpp potato.h utility.h
	g++ $(FLAGS) -o $@ $<

