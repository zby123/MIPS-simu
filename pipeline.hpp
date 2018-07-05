#ifndef PIPELINE_HPP
#define PIPELINE_HPP
#include <iostream>
#include <cstdio>
#include <fstream>
#include <string>
#include "mips.hpp"
#include "code.hpp"
#include "cpu.hpp"
using namespace std;

class mips;

class pipeline {

	struct _IF_ID{
		code inst;
		int npc;
	}IF_ID;

	struct _ID_EX{
		int ctrl, imm, A, B, npc, rd, rt;
	}ID_EX;

	struct _EX_MEM{
		int ctrl, dest;
		long long res;
	}EX_MEM;

	struct _MEM_WB{
		int ctrl, mdata, dest;
		long long res;
		char tstr[1000];
	}MEM_WB;

	CPU *cpu;
	int pc;
	bool c_stall;
	int fin;

public:

	void init(CPU &t_cpu, int tpc) {
		cpu = &t_cpu;
		pc = tpc;
		c_stall = 0;
		ID_EX.ctrl = EX_MEM.ctrl = MEM_WB.ctrl = -1;
		IF_ID.inst.opr_type = -1;
		fin = 0;
	}

	void IF() {
		if (fin) return;
		if (cpu->r_stall) {
			cpu->r_stall = 0;
			return;
		}
		if (c_stall) {
			IF_ID.inst.opr_type = -1;
			return;
		}
		IF_ID.inst = cpu->fetch_ins(pc);
		if (IF_ID.inst.opr_type == -1) {
			fin = 1;
			return;
		}
		//printf("IF : %d %d \n", pc, IF_ID.inst.opr_type);
		IF_ID.npc = pc + 1;
		pc += 1;
	}

	void ID() {
		if (fin) {
			if (fin == 1) fin++;
			else return;
		}
		if (c_stall) {
			ID_EX.ctrl = -1;
			return;
		}
		code tmp = IF_ID.inst;
		ID_EX.ctrl = IF_ID.inst.opr_type;
		ID_EX.npc = IF_ID.npc;
		if (tmp.opr_type == -1) return;
		int v0;
		if (tmp.opr_type == 54) {
			v0 = cpu->get_reg(2);
			ID_EX.ctrl += v0;
		}
		//printf("ID : %d\n", tmp.opr_type);
		// A:
		switch (tmp.opr_type) {
			case 0: case 1: case 2: case 3: case 4: case 5:
			case 7: case 9: case 11: case 13: case 14: 
			case 17: case 18:
			case 20: case 21: case 22: case 23: case 24: case 25:
			case 50:
				ID_EX.A = cpu->get_reg(tmp.arg[1]);
				break;
			case 6: case 8: case 10: case 12: case 15: case 16:
			case 19: 
			case 27: case 28: case 29: case 30: case 31: case 32:
			case 33: case 34: case 35: case 36: case 37: case 38:
			case 40: case 42: case 47: case 48: case 49:
				ID_EX.A = cpu->get_reg(tmp.arg[0]);
				break;
			case 51:
				ID_EX.A = cpu->get_reg(33);
				break;
			case 52:
				ID_EX.A = cpu->get_reg(32);
				break;
			case 54:
				switch (v0) {
					case 1: case 4: case 8: case 9:
						ID_EX.A = cpu->get_reg(4);
						break;
					default:
						ID_EX.A = 0;
				}
				break;
			default:
				ID_EX.A = 0;
		}
		// B:
		switch (tmp.opr_type) {
			case 0: case 1: case 2: case 3: case 4: case 5:
			case 7: case 9: case 11: case 13: case 14: case 17: case 18:
			case 20: case 21: case 22: case 23: case 24: case 25:
				if (tmp.offset) ID_EX.B = cpu->get_reg(tmp.arg[2]);
				else ID_EX.B = tmp.arg[2];
				break;
			case 6: case 8: case 10: case 12:
			case 27: case 28: case 29: case 30: case 31: case 32:
				if (tmp.offset) ID_EX.B = cpu->get_reg(tmp.arg[1]);
				else ID_EX.B = tmp.arg[1];
				break;
			case 43: case 44: case 45: case 46: case 47: case 48: case 49:
				if (tmp.arg[1] < 0) ID_EX.B = 0;
				else ID_EX.B = cpu->get_reg(tmp.arg[1]);
				break;
			case 54:
				if (v0 == 8) ID_EX.B = cpu->get_reg(5);
				else ID_EX.B = 0;
				break;
			default:
				ID_EX.B = 0;
		}

		if (cpu->r_stall) {
			ID_EX.ctrl = -1;
			return;
		}

		// rd:
		switch (tmp.opr_type) {
			case 0: case 1: case 2: case 3: case 4: case 5:
			case 7: case 9: case 11: case 13: case 14: case 15: case 16: case 17:
			case 18: case 19: case 20: case 21: case 22: case 23:
			case 24: case 25: case 43: case 44: case 45: case 46:
			case 50: case 51: case 52:
				ID_EX.rd = tmp.arg[0];
				break;
			case 54:
				if (v0 == 5 || v0 == 9) ID_EX.rd = 2;
				else ID_EX.rd = -1;
				break;
			default:
				ID_EX.rd = -1;
		}

		if (ID_EX.rd >= 0) cpu->lock_reg(ID_EX.rd);
		if (tmp.opr_type == 6 || tmp.opr_type == 8 || tmp.opr_type == 10 || tmp.opr_type == 12) {
			cpu->lock_reg(32); cpu->lock_reg(33);
		}
		if (tmp.opr_type == 41 || tmp.opr_type == 42) {
			cpu->lock_reg(31);
		}

		// imm:
		if ((tmp.opr_type >= 26 && tmp.opr_type <= 39) || (tmp.opr_type == 41)) {
			ID_EX.imm = tmp.arg[2];
		}
		if (tmp.opr_type >= 43 && tmp.opr_type <= 49) {
			if (tmp.arg[1] < 0) ID_EX.imm = - tmp.arg[1] - 1;
			else ID_EX.imm = tmp.offset;
		}
		if (tmp.opr_type == 19) ID_EX.imm = tmp.arg[1];

		if (tmp.opr_type >= 26 && tmp.opr_type <= 42) c_stall = 1;
	}

	void EX() {
		if (fin) {
			if (fin == 2) fin++;
			else return;
		}
		int op = ID_EX.ctrl;
		if (op == -1) {
			EX_MEM.ctrl = -1;
			return;
		}
		int A, B, imm, rd, npc, len;
		A = ID_EX.A; B = ID_EX.B;
		imm = ID_EX.imm; rd = ID_EX.rd;
		npc = ID_EX.npc;
		EX_MEM.ctrl = op;

		//printf("EX : %d\n", op);
		// res:
		switch (op) {
			case 0:	case 1: case 2:
				EX_MEM.res = A + B;
				break;
			case 3: case 4:
				EX_MEM.res = A - B;
				break;
			case 5: case 6:
				EX_MEM.res = (long long)A * B;
				break;
			case 7: case 8:
				EX_MEM.res = cpu->tou(A) * cpu->tou(B);
				break;
			case 9: case 10:
				EX_MEM.res = A % B;
				EX_MEM.res <<= 32;
				EX_MEM.res |= A / B;
				break;
			case 11: case 12:
				EX_MEM.res = cpu->tou(A) % cpu->tou(B);
				EX_MEM.res <<= 32;
				EX_MEM.res |= cpu->tou(A) / cpu->tou(B);
				break;
			case 13: case 14:
				EX_MEM.res = A ^ B;
				break;
			case 15:
				EX_MEM.res = -A;
				break;
			case 16:
				EX_MEM.res = ~A;
				break;
			case 17:
				EX_MEM.res = A % B;
				break;
			case 18:
				EX_MEM.res = cpu->tou(A) % cpu->tou(B);
				break;
			case 19:
				EX_MEM.res = imm;
				break;
			case 20:
				EX_MEM.res = (A == B);
				break;
			case 21:
				EX_MEM.res = (A >= B);
				break;
			case 22:
				EX_MEM.res = (A > B);
				break;
			case 23:
				EX_MEM.res = (A <= B);
				break;
			case 24:
				EX_MEM.res = (A < B);
				break;
			case 25:
				EX_MEM.res = (A != B);
				break;
			case 26:
				pc = imm;
				break;
			case 27:
				if (A == B) pc = imm;
				break;
			case 28:
				if (A != B) pc = imm;
				break;
			case 29:
				if (A >= B) pc = imm;
				break;
			case 30:
				if (A <= B) pc = imm;
				break;
			case 31:
				if (A > B) pc = imm;
				break;
			case 32:
				if (A < B) pc = imm;
				break;
			case 33:
				if (A == 0) pc = imm;
				break;
			case 34:
				if (A != 0) pc = imm;
				break;
			case 35:
				if (A <= 0) pc = imm;
				break;
			case 36:
				if (A >= 0) pc = imm;
				break;
			case 37:
				if (A > 0) pc = imm;
				break;
			case 38:
				if (A < 0) pc = imm;
				break;
			case 39:
				pc = imm;
				break;
			case 40:
				pc = A;
				break;
			case 41:
				pc = imm;
				EX_MEM.res = npc;
				break;
			case 42:
				pc = A;
				EX_MEM.res = npc;
				break;
			case 43: case 44: case 45: case 46:
			case 47: case 48: case 49:
				EX_MEM.res = B + imm;
				break;
			case 50: case 51: case 52: case 55:
			case 58: case 62: case 63:
				EX_MEM.res = A;
				break;
			case 64: case 71:
				fin = 3;
				break;
		}

		// dest:
		if (op >= 0 && op <= 25) EX_MEM.dest = rd;
		if (op == 41 || op == 42) EX_MEM.dest = 31;
		if (op >= 43 && op <= 46) EX_MEM.dest = rd;
		if (op >= 47 && op <= 49) EX_MEM.dest = A;
		if (op >= 50 && op <= 52) EX_MEM.dest = rd;
		if (op == 59 || op == 63) EX_MEM.dest = 2;
		if (op == 62) EX_MEM.dest = B;
		c_stall = 0;
	}

	void MEM() {
		if (fin) {
			if (fin == 3) fin++;
			else return;
		}
		int ctrl, dest;
		long long res;
		ctrl = EX_MEM.ctrl;
		dest = EX_MEM.dest; res = EX_MEM.res;
		if (ctrl == -1) {
			MEM_WB.ctrl = -1;
			return;
		}
		//printf("MEM : %d\n", ctrl);
		int mdata = 0;
		int i, j, len;
		char tt;
		char tstr[1000];
		string tmp = "";
		switch (ctrl) {
			case 44:
				mdata = cpu->lb(res);
				break;
			case 45:
				mdata = cpu->lh(res);
				break;
			case 46:
				mdata = cpu->lw(res);
				break;
			case 47:
				cpu->sb(dest, res);
				break;
			case 48:
				cpu->sh(dest, res);
				break;
			case 49:
				cpu->sw(dest, res);
				break;
			case 55:
				cout << res;
				break;
			case 58:
				i = res;
				for(tt = cpu->lb(i);tt != '\0'; i++, tt = cpu->lb(i)) tmp += tt;
				cout << tmp;
				break;
			case 59:
				cin >> res;
				break;
			case 62:
				cin.getline(tstr, dest - 1);
				len = strlen(tstr);
				if (len == 0) {
					cin.getline(tstr, dest - 1);
					len = strlen(tstr);
				}
				for(i = 0; i < len; i++) cpu->sb(tstr[i], res + i);
				break;
			case 63:
				i = cpu->data_p();
				cpu->sys_malloc(res);
				res = i;
				break;
		}
		MEM_WB.mdata = mdata;
		MEM_WB.ctrl = ctrl;
		MEM_WB.dest = dest;
		MEM_WB.res = res;
	}

	void WB() {
		if (fin) {
			if (fin == 4) fin++;
			else return;
		}
		int ctrl, dest, mdata;
		long long res;
		ctrl = MEM_WB.ctrl;
		dest = MEM_WB.dest;
		mdata = MEM_WB.mdata;
		res = MEM_WB.res;
		if (ctrl == -1) return;
		//printf("WB : %d\n", ctrl);
		if (ctrl == 6 || ctrl == 8 || ctrl == 10 || ctrl == 12) {
			cpu->write_reg(32, res & ((1LL << 32) - 1));
			cpu->write_reg(33, res >> 32);
		}
		else if ((ctrl >= 0 && ctrl <= 25) || (ctrl >= 41 && ctrl <= 43) || (ctrl >= 50 && ctrl <= 52) || ctrl == 59 || ctrl == 63) {
			cpu->write_reg(dest, res);
		}
		else if (ctrl >= 44 && ctrl <= 46) {
			cpu->write_reg(dest, mdata);
		}
	}

	void run() {
		while (fin != 5) {
			WB();
			MEM();
			EX();
			ID();
			IF();
		}
	}

};

#endif