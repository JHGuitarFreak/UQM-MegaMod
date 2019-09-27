uqm_CFLAGS=-W -Wall -g -O0 -fPIC
uqm_INCLUDE=-I .. -I ../sc2code -I ../sc2code/libs
uqm_LDFLAGS=-shared

abxadec.so: abxaud.c abxaud.h
	gcc $(uqm_CFLAGS) $(uqm_INCLUDE) $(uqm_LDFLAGS) abxaud.c -o abxadec.so

clean:
	rm abxadec.so
