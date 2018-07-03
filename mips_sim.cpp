#include <iostream>
#include <cstring>
#include <string>
#include "mips.hpp"

char st[100];

int main(){
	mips mip;
	mip.init("a.s");
	mip.run();
	return 0;
}