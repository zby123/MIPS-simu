#ifndef CPU_HPP
#define CPU_HPP

#include <vector>
#include <cstring>
#include "code.hpp"

using namespace std;

const int MXC = 256;
const long long MXI = 2147483648;
const long long MXU = 4294967296;

class CPU{
	int reg[35], data_buf;
	unsigned char data[4 * 1024 * 1024];
	vector<code> text;
	char reg_lock[35];

public:
	bool r_stall;

	void init(){
		data_buf = 0;
		memset(reg, 0, sizeof(reg));
		reg[29] = 4 * 1024 * 1024 - 1;
		memset(data, 0, sizeof(data));
		r_stall = 0;
		memset(reg_lock, 0, sizeof(reg_lock));
	}

	long long tou(int x) {
		return (x >= 0 ? x : MXU + x);
	}

	int data_p(){return data_buf;}
	
	int text_p(){return text.size();}

	code fetch_ins(int pc) {
		return text[pc];
	}

	void add_text(code cd) {
		text.push_back(cd);
	}

	void modi_text(code cd, int pos) {
		text[pos] = cd;
	}

	int get_reg(int pos) {
		if (reg_lock[pos]) r_stall = 1;
		return reg[pos];
	}

	void write_reg(int pos, int num) {
		reg[pos] = num;
		reg_lock[pos]--;
	}

	void lock_reg(int pos) {
		reg_lock[pos] ++;
	}

	void sb(int x, int adr) {
		long long tmp = tou(x);
		for (int i = 0; i >= 0; i--) {
			data[adr + i] = tmp % MXC;
			tmp /= MXC;
		}
	}

	void sh(int x, int adr) {
		long long tmp = tou(x);
		for (int i = 1; i >= 0; i--) {
			data[adr + i] = tmp % MXC;
			tmp /= MXC;
		}
	}

	void sw(int x, int adr) {
		long long tmp = tou(x);
		for (int i = 3; i >= 0; i--) {
			data[adr + i] = tmp % MXC;
			tmp /= MXC;
		}
	}

	int lb(int adr) {
		int tmp = 0;
		for (int i = 0; i < 1; i++) tmp = tmp * MXC + data[adr + i];
		return tmp;
	}

	int lh(int adr) {
		int tmp = 0;
		for (int i = 0; i < 2; i++) tmp = tmp * MXC + data[adr + i];
		return tmp;
	}

	int lw(int adr) {
		int tmp = 0;
		for (int i = 0; i < 4; i++) tmp = tmp * MXC + data[adr + i];
		return tmp;
	}

	void mul(int x, int y) {
		long long tmp = x * y;
		reg[32] = tmp & ((1LL << 32) - 1);
		reg[33] = tmp >> 32;
	}

	void mulu(int x, int y) {
		long long tmp = tou(x) * tou(y);
		reg[32] = tmp & ((1LL << 32) - 1);
		reg[33] = tmp >> 32;
	}

	void div(int x, int y) {
		reg[32] = x / y;
		reg[33] = x % y;
	}

	void divu(int x, int y) {
		reg[32] = tou(x) / tou(y);
		reg[33] = tou(x) % tou(y);
	}

	int sys_malloc(int x) {
		int res = data_buf;
		data_buf += x;
		return data_buf;
	}

};

#endif