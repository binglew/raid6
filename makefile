#---------------------------------------------------------------------------------
#	makefile of raid6 test
#	Bingle
#---------------------------------------------------------------------------------
CFLAGS = -DLINUX -O3

clean:
	rm -fr ./linux/*
	@echo ====clean done====	

compile:
	mkdir ./linux/obj
	g++ $(CFLAGS) -c -o ./linux/obj/raid6.o			./raid6_lib/raid6.cpp
	g++ $(CFLAGS) -c -o ./linux/obj/raid6_test.o	./raid6_test/raid6_test.cpp
	@echo ====compile done====

link:
	mkdir ./linux/bin
	g++ $(CFLAGS) -o ./linux/bin/raid6_test ./linux/obj/raid6.o ./linux/obj/raid6_test.o
	@echo ====link done====

all: clean compile link
