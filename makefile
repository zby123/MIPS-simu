make:
	g++ mips_sim.cpp -lpthread -o code -std=c++11 -O2
clean:
	rm -f mips