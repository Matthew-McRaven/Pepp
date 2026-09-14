Pep7Project: asem7 pep7 os.o mnemon
os.o: asem7
	./asem7 os.pep
	mv os.pep.o os.o
asem7: asem7.cpp
	c++ -o asem7 asem7.cpp
	strip asem7
pep7: pep7.cpp
	c++ -o pep7 pep7.cpp
	strip pep7

