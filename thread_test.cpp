#include <iostream>
#include <thread>

using namespace std;

void t(int x) {
	cout << x << endl;
}

int main(){
	for(int i = 0; i < 10; i++) {
		thread thr(t, i);
		thr.detach();
	}
	return 0;
}