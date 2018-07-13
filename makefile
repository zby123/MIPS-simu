make:
	g++ mips_sim.cpp -lpthread -o code -std=c++11
clean:
	rm -f mips