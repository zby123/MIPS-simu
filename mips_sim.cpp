#include <iostream>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "mips.hpp"
#include "code.hpp"
#include "cpu.hpp"
using namespace std;

char st[100];

struct TLA_pre {
	int prediction[16];
	TLA_pre(){
		memset(prediction, 0, sizeof(prediction));
	}
};

int pc, ppc, prd;
bool r_stall;
map<int, int> b_history;
map<int, TLA_pre> predictor;
int fin, suc, fail;
mutex f_mtx;

struct _IF_ID{
	code inst;
	int npc;
	mutex mtx;
	condition_variable fetched;
	condition_variable wrote;
	bool fr;
}IF_ID;

struct _ID_EX{
	int ctrl, imm, A, B, npc, rd, rt;
	mutex mtx;
	condition_variable fetched;
	bool fr;
}ID_EX;

struct _EX_MEM{
	int ctrl, dest;
	long long res;
	mutex mtx;
	condition_variable fetched;
	bool fr;
}EX_MEM;

struct _MEM_WB{
	int ctrl, mdata, dest;
	long long res;
	mutex mtx;
	condition_variable fetched;
	bool fr;
}MEM_WB;

CPU *tcpu;

void init(CPU &tcp, int tpc) {
	tcpu = &tcp;
	pc = tpc;
	ID_EX.ctrl = EX_MEM.ctrl = MEM_WB.ctrl = -1;
	IF_ID.inst.opr_type = -1;
	fin = 0;
	ppc = 0;
	fail = suc = 0;
	predictor.clear();
	IF_ID.fr = ID_EX.fr = EX_MEM.fr = MEM_WB.fr = 1;
}

void *IF(void *ptr) {
	if (pc == -1) {
		////puts("wait IF 1");
		unique_lock<mutex> lock(IF_ID.mtx);
		if (IF_ID.fr) IF_ID.fetched.wait(lock);
		IF_ID.fr = 1;
		IF_ID.inst.opr_type = -2;
		IF_ID.wrote.notify_all();
		return NULL;
	}
	if (r_stall) {
		unique_lock<mutex> lock(IF_ID.mtx);
		if (IF_ID.fr) {
			////puts("wait IF 2");
			IF_ID.fetched.wait(lock);
			////puts("wait end");
		}
		IF_ID.fr = 1;
		IF_ID.wrote.notify_all();
		return NULL;
	}
	////puts("wait IF 3");
	unique_lock<mutex> lock(IF_ID.mtx);
	if (IF_ID.fr) IF_ID.fetched.wait(lock);
	IF_ID.inst = tcpu->fetch_ins(pc);
	//printf("IF : %d %d \n", pc, IF_ID.inst.opr_type);
	IF_ID.npc = pc + 1;
	pc += 1;
	IF_ID.fr = 1;
	if (IF_ID.inst.opr_type == -1) fin = 1;
	IF_ID.wrote.notify_all();
	return NULL;
}

void *ID(void *ptr) {
	if (tcpu->r_stall) tcpu->r_stall = 0;
	int A, B, npc, ctrl, rd, imm;
	IF_ID.mtx.lock();
	code tmp = IF_ID.inst;
	ctrl = tmp.opr_type;
	npc = IF_ID.npc;
	IF_ID.fr = 0;
	//printf("fr : 0\n");
	IF_ID.mtx.unlock();
	IF_ID.fetched.notify_all();
	if (tmp.opr_type == -1) {
		unique_lock<mutex> lock(ID_EX.mtx);
		//puts("wait ID 2");
		if (ID_EX.fr) ID_EX.fetched.wait(lock);
		ID_EX.ctrl = -1;
		ID_EX.fr = 1;
		return 0;
	}
	if (tmp.opr_type == -2) {
		unique_lock<mutex> lock(ID_EX.mtx);
		//puts("wait ID 3");
		if (ID_EX.fr) ID_EX.fetched.wait(lock);
		ID_EX.ctrl = -2;
		ID_EX.fr = 1;
		return 0;
	}
	int v0;
	if (tmp.opr_type == 54) {
		v0 = tcpu->get_reg(2);
		ctrl += v0;
	}
	//printf("%d %d %d\nID : %d\n", tmp.arg[0], tmp.arg[1], tmp.arg[2], tmp.opr_type);
	// A:
	switch (tmp.opr_type) {
		case 0: case 1: case 2: case 3: case 4: case 5:
		case 7: case 9: case 11: case 13: case 14: 
		case 17: case 18:
		case 20: case 21: case 22: case 23: case 24: case 25:
		case 50:
			A = tcpu->get_reg(tmp.arg[1]);
			break;
		case 6: case 8: case 10: case 12: case 15: case 16:
		case 27: case 28: case 29: case 30: case 31: case 32:
		case 33: case 34: case 35: case 36: case 37: case 38:
		case 47: case 48: case 49:
			A = tcpu->get_reg(tmp.arg[0]);
			break;
		case 51:
			A = tcpu->get_reg(33);
			break;
		case 52:
			A = tcpu->get_reg(32);
			break;
		case 54:
			switch (v0) {
				case 1: case 4: case 8: case 9:
					A = tcpu->get_reg(4);
					break;
				default:
					A = 0;
			}
			break;
		default:
			A = 0;
	}
	// B:
	switch (tmp.opr_type) {
		case 0: case 1: case 2: case 3: case 4: case 5:
		case 7: case 9: case 11: case 13: case 14: case 17: case 18:
		case 20: case 21: case 22: case 23: case 24: case 25:
			if (tmp.offset) B = tcpu->get_reg(tmp.arg[2]);
			else B = tmp.arg[2];
			break;
		case 6: case 8: case 10: case 12:
		case 27: case 28: case 29: case 30: case 31: case 32:
			if (tmp.offset) B = tcpu->get_reg(tmp.arg[1]);
			else B = tmp.arg[1];
			break;
		case 43: case 44: case 45: case 46: case 47: case 48: case 49:
			if (tmp.arg[1] < 0) B = 0;
			else B = tcpu->get_reg(tmp.arg[1]);
			break;
		case 54:
			if (v0 == 8) B = tcpu->get_reg(5);
			else B = 0;
			break;
		default:
			B = 0;
	}

	// imm:
	if ((tmp.opr_type >= 26 && tmp.opr_type <= 39) || (tmp.opr_type == 41)) {
		imm = tmp.arg[2];
	}
	if (tmp.opr_type >= 43 && tmp.opr_type <= 49) {
		if (tmp.arg[1] < 0) imm = - tmp.arg[1] - 1;
		else imm = tmp.offset;
	}
	if (tmp.opr_type == 40 || tmp.opr_type == 42) {
		imm = tcpu->get_reg(tmp.arg[0]);
	}
	if (tmp.opr_type == 19) imm = tmp.arg[1];

	if (tcpu->r_stall) {
		unique_lock<mutex> lock2(IF_ID.mtx);
		if (!IF_ID.fr) {
			//puts("wait ID 4");
			IF_ID.wrote.wait(lock2);
			//puts("wait end");
		}
		IF_ID.inst = tmp; IF_ID.npc = npc;
		IF_ID.fr = 1;
		unique_lock<mutex> lock(ID_EX.mtx);
		//puts("wait ID 5");
		if (ID_EX.fr) ID_EX.fetched.wait(lock);
		ID_EX.ctrl = -1;
		ID_EX.fr = 1;
		r_stall = 1;
		return 0;
	}

	// rd:
	switch (tmp.opr_type) {
		case 0: case 1: case 2: case 3: case 4: case 5:
		case 7: case 9: case 11: case 13: case 14: case 15: case 16: case 17:
		case 18: case 19: case 20: case 21: case 22: case 23:
		case 24: case 25: case 43: case 44: case 45: case 46:
		case 50: case 51: case 52:
			rd = tmp.arg[0];
			break;
		case 54:
			if (v0 == 5 || v0 == 9) rd = 2;
			else rd = -1;
			break;
		default:
			rd = -1;
	}

	if (rd >= 0) tcpu->lock_reg(rd);
	if (tmp.opr_type == 6 || tmp.opr_type == 8 || tmp.opr_type == 10 || tmp.opr_type == 12) {
		tcpu->lock_reg(32); tcpu->lock_reg(33);
	}
	if (tmp.opr_type == 41 || tmp.opr_type == 42) {
		tcpu->lock_reg(31);
	}


	if ((tmp.opr_type >= 26 && tmp.opr_type <= 42)|| ctrl == 64 || ctrl == 71) {
		unique_lock<mutex> c_lock(IF_ID.mtx);
		//puts("wait ID 6");
		if (!IF_ID.fr) IF_ID.wrote.wait(c_lock);
		IF_ID.inst.opr_type = -1;
		IF_ID.fr = 1;
		if (tmp.opr_type == 26 || (tmp.opr_type >= 39 && tmp.opr_type <= 42)) {
			pc = imm;
		}
		if (tmp.opr_type >= 27 && tmp.opr_type <= 38) {
			if (predictor.count(IF_ID.npc - 1) == 0) {
				TLA_pre tmp;
				predictor[IF_ID.npc - 1] = tmp;
				b_history[IF_ID.npc - 1] = 0;
			}
			if (predictor[IF_ID.npc - 1].prediction[b_history[IF_ID.npc - 1]] > 1) {
				ppc = pc = imm;
			}
			else ppc = pc = npc;
			prd = rd;
		}
		if (r_stall) {
			r_stall = 0;
		}
		if (ctrl == 64 || ctrl == 71) {
			pc = -1;
		}
	}
	else if (r_stall) {
		unique_lock<mutex> lock_r(IF_ID.mtx);

		if (!IF_ID.fr) {
			//puts("wait ID 7");
			IF_ID.wrote.wait(lock_r);
			//puts("wait end");
		}
		IF_ID.fr = 1;
		IF_ID.inst.opr_type = -1;
		r_stall = 0;
		pc = npc;
	}

	unique_lock<mutex> lock(ID_EX.mtx);

		//puts("wait ID 8");
	if (ID_EX.fr) ID_EX.fetched.wait(lock);
	ID_EX.A = A; ID_EX.B = B; ID_EX.rd = rd;
	ID_EX.imm = imm;
	ID_EX.ctrl = ctrl;
	ID_EX.npc = npc;
	ID_EX.fr = 1;
	return 0;
}

void *EX(void *ptr) {
	int A, B, imm, rd, npc, len;
	int dest, tpc;
	long long res;
	ID_EX.mtx.lock();
	int op = ID_EX.ctrl;
	A = ID_EX.A; B = ID_EX.B;
	imm = ID_EX.imm; rd = ID_EX.rd;
	npc = ID_EX.npc;
	ID_EX.fr = 0;
	ID_EX.mtx.unlock();
	ID_EX.fetched.notify_all();
	if (op == -1) {
		unique_lock<mutex> lock(EX_MEM.mtx);
		////puts("wait EX 1");
		if (EX_MEM.fr) EX_MEM.fetched.wait(lock);
		EX_MEM.ctrl = -1;
		EX_MEM.fr = 1;
		return 0;
	}
	if (op == -2) {
		unique_lock<mutex> lock(EX_MEM.mtx);
		////puts("wait EX 2");
		if (EX_MEM.fr) EX_MEM.fetched.wait(lock);
		EX_MEM.ctrl = -2;
		EX_MEM.fr = 1;
		return 0;
	}
	//printf("%d %d %d %d\n", A, B, imm, rd);
	//printf("EX : %d\n", op);
	// res:
	switch (op) {
		case 0:	case 1: case 2:
			res = A + B;
			break;
		case 3: case 4:
			res = A - B;
			break;
		case 5: case 6:
			res = (long long)A * B;
			break;
		case 7: case 8:
			res = tcpu->tou(A) * tcpu->tou(B);
			break;
		case 9: case 10:
			res = A % B;
			res <<= 32;
			res |= A / B;
			break;
		case 11: case 12:
			res = tcpu->tou(A) % tcpu->tou(B);
			res <<= 32;
			res |= tcpu->tou(A) / tcpu->tou(B);
			break;
		case 13: case 14:
			res = A ^ B;
			break;
		case 15:
			res = -A;
			break;
		case 16:
			res = ~A;
			break;
		case 17:
			res = A % B;
			break;
		case 18:
			res = tcpu->tou(A) % tcpu->tou(B);
			break;
		case 19:
			res = imm;
			break;
		case 20:
			res = (A == B);
			break;
		case 21:
			res = (A >= B);
			break;
		case 22:
			res = (A > B);
			break;
		case 23:
			res = (A <= B);
			break;
		case 24:
			res = (A < B);
			break;
		case 25:
			res = (A != B);
			break;
		case 27:
			if (A == B) {
				tpc = imm;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 3) predictor[npc - 1].prediction[b_history[npc - 1]]++;
				b_history[npc - 1] = (b_history[npc - 1] * 2 + 1) % 16;
			}
			else {
				tpc = npc;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 0) predictor[npc - 1].prediction[b_history[npc - 1]]--;
				b_history[npc - 1] = (b_history[npc - 1] * 2) % 16;
			}
			break;
		case 28:
			if (A != B) {
				tpc = imm;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 3) predictor[npc - 1].prediction[b_history[npc - 1]]++;
				b_history[npc - 1] = (b_history[npc - 1] * 2 + 1) % 16;
			}
			else {
				tpc = npc;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 0) predictor[npc - 1].prediction[b_history[npc - 1]]--;
				b_history[npc - 1] = (b_history[npc - 1] * 2) % 16;
			}
			break;
		case 29:
			if (A >= B) {
				tpc = imm;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 3) predictor[npc - 1].prediction[b_history[npc - 1]]++;
				b_history[npc - 1] = (b_history[npc - 1] * 2 + 1) % 16;
			}
			else {
				tpc = npc;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 0) predictor[npc - 1].prediction[b_history[npc - 1]]--;
				b_history[npc - 1] = (b_history[npc - 1] * 2) % 16;
			}
			break;
		case 30:
			if (A <= B) {
				tpc = imm;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 3) predictor[npc - 1].prediction[b_history[npc - 1]]++;
				b_history[npc - 1] = (b_history[npc - 1] * 2 + 1) % 16;
			}
			else {
				tpc = npc;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 0) predictor[npc - 1].prediction[b_history[npc - 1]]--;
				b_history[npc - 1] = (b_history[npc - 1] * 2) % 16;
			}
			break;
		case 31:
			if (A > B) {
				tpc = imm;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 3) predictor[npc - 1].prediction[b_history[npc - 1]]++;
				b_history[npc - 1] = (b_history[npc - 1] * 2 + 1) % 16;
			}
			else {
				tpc = npc;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 0) predictor[npc - 1].prediction[b_history[npc - 1]]--;
				b_history[npc - 1] = (b_history[npc - 1] * 2) % 16;
			}
			break;
		case 32:
			if (A < B) {
				tpc = imm;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 3) predictor[npc - 1].prediction[b_history[npc - 1]]++;
				b_history[npc - 1] = (b_history[npc - 1] * 2 + 1) % 16;
			}
			else {
				tpc = npc;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 0) predictor[npc - 1].prediction[b_history[npc - 1]]--;
				b_history[npc - 1] = (b_history[npc - 1] * 2) % 16;
			}
			break;
		case 33:
			if (A == 0) {
				tpc = imm;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 3) predictor[npc - 1].prediction[b_history[npc - 1]]++;
				b_history[npc - 1] = (b_history[npc - 1] * 2 + 1) % 16;
			}
			else {
				tpc = npc;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 0) predictor[npc - 1].prediction[b_history[npc - 1]]--;
				b_history[npc - 1] = (b_history[npc - 1] * 2) % 16;
			}
			break;
		case 34:
			if (A != 0) {
				tpc = imm;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 3) predictor[npc - 1].prediction[b_history[npc - 1]]++;
				b_history[npc - 1] = (b_history[npc - 1] * 2 + 1) % 16;
			}
			else {
				tpc = npc;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 0) predictor[npc - 1].prediction[b_history[npc - 1]]--;
				b_history[npc - 1] = (b_history[npc - 1] * 2) % 16;
			}
			break;
		case 35:
			if (A <= 0) {
				tpc = imm;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 3) predictor[npc - 1].prediction[b_history[npc - 1]]++;
				b_history[npc - 1] = (b_history[npc - 1] * 2 + 1) % 16;
			}
			else {
				tpc = npc;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 0) predictor[npc - 1].prediction[b_history[npc - 1]]--;
				b_history[npc - 1] = (b_history[npc - 1] * 2) % 16;
			}
			break;
		case 36:
			if (A >= 0) {
				tpc = imm;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 3) predictor[npc - 1].prediction[b_history[npc - 1]]++;
				b_history[npc - 1] = (b_history[npc - 1] * 2 + 1) % 16;
			}
			else {
				tpc = npc;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 0) predictor[npc - 1].prediction[b_history[npc - 1]]--;
				b_history[npc - 1] = (b_history[npc - 1] * 2) % 16;
			}
			break;
		case 37:
			if (A > 0) {
				tpc = imm;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 3) predictor[npc - 1].prediction[b_history[npc - 1]]++;
				b_history[npc - 1] = (b_history[npc - 1] * 2 + 1) % 16;
			}
			else {
				tpc = npc;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 0) predictor[npc - 1].prediction[b_history[npc - 1]]--;
				b_history[npc - 1] = (b_history[npc - 1] * 2) % 16;
			}
			break;
		case 38:
			if (A < 0) {
				tpc = imm;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 3) predictor[npc - 1].prediction[b_history[npc - 1]]++;
				b_history[npc - 1] = (b_history[npc - 1] * 2 + 1) % 16;
			}
			else {
				tpc = npc;
				if (predictor[npc - 1].prediction[b_history[npc - 1]] != 0) predictor[npc - 1].prediction[b_history[npc - 1]]--;
				b_history[npc - 1] = (b_history[npc - 1] * 2) % 16;
			}
			break;
		case 41:
			res = npc;
			break;
		case 42:
			res = npc;
			break;
		case 43: case 44: case 45: case 46:
		case 47: case 48: case 49:
			res = B + imm;
			break;
		case 50: case 51: case 52: case 55:
		case 58: case 62: case 63:
			res = A;
			break;
		case 64: case 71:
			op = -2;
			break;
	}

	// dest:
	if (op >= 0 && op <= 25) dest = rd;
	if (op == 41 || op == 42) dest = 31;
	if (op >= 43 && op <= 46) dest = rd;
	if (op >= 47 && op <= 49) dest = A;
	if (op >= 50 && op <= 52) dest = rd;
	if (op == 59 || op == 63) dest = 2;
	if (op == 62) dest = B;

	if (op >= 27 && op <= 38) {
		if (tpc != ppc) {
			unique_lock<mutex> lock(IF_ID.mtx);

			////puts("wait EX 3");
			if (!IF_ID.fr) IF_ID.wrote.wait(lock);
			IF_ID.inst.opr_type = -1;
			IF_ID.fr = 1;
			pc = tpc;
			if (rd >= 0) tcpu->unlock_reg(rd);
			if (op == 6 || op == 8 || op == 10 || op == 12) {
				tcpu->unlock_reg(32); tcpu->unlock_reg(33);
			}
			if (op == 41 || op == 42) {
				tcpu->unlock_reg(31);
			}
		}
	}

	unique_lock<mutex> lock(EX_MEM.mtx);
	if (EX_MEM.fr) EX_MEM.fetched.wait(lock);
	EX_MEM.dest = dest; EX_MEM.res = res;
	EX_MEM.fr = 1; EX_MEM.ctrl = op;
	return 0;
}

void *MEM(void *ptr) {
	int ctrl, dest;
	long long res;
	int mdata;
	EX_MEM.mtx.lock();
	ctrl = EX_MEM.ctrl;
	dest = EX_MEM.dest; res = EX_MEM.res;
	//printf("%d %d %lld\n", ctrl, dest, res);
	EX_MEM.fr = 0;
	EX_MEM.mtx.unlock();
	EX_MEM.fetched.notify_all();
	if (ctrl == -1) {
		unique_lock<mutex> lock(MEM_WB.mtx);
		if (MEM_WB.fr) MEM_WB.fetched.wait(lock);
		MEM_WB.ctrl = -1;
		MEM_WB.fr = 1;
		return 0;
	}
	if (ctrl == -2) {
		unique_lock<mutex> lock(MEM_WB.mtx);
		if (MEM_WB.fr) MEM_WB.fetched.wait(lock);
		MEM_WB.ctrl = -2;
		MEM_WB.fr = 1;
		return 0;
	}
	//printf("MEM : %d\n", ctrl);
	int i, j, len;
	char tt;
	char tstr[1000];
	string tmp = "";
	switch (ctrl) {
		case 44:
			mdata = tcpu->lb(res);
			break;
		case 45:
			mdata = tcpu->lh(res);
			break;
		case 46:
			mdata = tcpu->lw(res);
			break;
		case 47:
			tcpu->sb(dest, res);
			break;
		case 48:
			tcpu->sh(dest, res);
			break;
		case 49:
			tcpu->sw(dest, res);
			break;
		case 55:
			cout << res;
			break;
		case 58:
			i = res;
			for(tt = tcpu->lb(i);tt != '\0'; i++, tt = tcpu->lb(i)) tmp += tt;
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
			for(i = 0; i < len; i++) tcpu->sb(tstr[i], res + i);
			break;
		case 63:
			i = tcpu->data_p();
			tcpu->sys_malloc(res);
			res = i;
			break;
	}
	unique_lock<mutex> lock(MEM_WB.mtx);
	if (MEM_WB.fr) MEM_WB.fetched.wait(lock);
	MEM_WB.mdata = mdata;
	MEM_WB.ctrl = ctrl;
	MEM_WB.dest = dest;
	MEM_WB.res = res;
	MEM_WB.fr = 1;
	return 0;
}

void *WB(void *ptr) {
	int ctrl, dest, mdata;
	long long res;
	MEM_WB.mtx.lock();
	ctrl = MEM_WB.ctrl;
	dest = MEM_WB.dest;
	mdata = MEM_WB.mdata;
	res = MEM_WB.res;
	MEM_WB.fr = 0;
	MEM_WB.mtx.unlock();
	MEM_WB.fetched.notify_all();
	if (ctrl == -1) return 0;
	if (ctrl == -2) fin = 1;
	//printf("WB : %d\n", ctrl);
	if (ctrl == 6 || ctrl == 8 || ctrl == 10 || ctrl == 12) {
		tcpu->write_reg(32, res & ((1LL << 32) - 1));
		tcpu->write_reg(33, res >> 32);
	}
	else if ((ctrl >= 0 && ctrl <= 25) || (ctrl >= 41 && ctrl <= 43) || (ctrl >= 50 && ctrl <= 52) || ctrl == 59 || ctrl == 63) {
		tcpu->write_reg(dest, res);
	}
	else if (ctrl >= 44 && ctrl <= 46) {
		tcpu->write_reg(dest, mdata);
	}

	//printf("%d %d %d %lld\n", pc, ctrl, dest, res);
	/*for(int i = 0; i < 34; i++) {
		if (i % 6 == 0) printf("\n");
		printf("$%d:%d\t", i, tcpu->get_reg(i));
	}
	printf("\n");*/
	return 0;
}

mutex l_if, l_id, l_ex, l_mem;
int f_if, f_id, f_ex, f_mem;
condition_variable c_if, c_id, c_ex, c_mem;

void _IF(){
	unique_lock<mutex> lock(l_if);
	while (!fin) {
		lock.unlock();
		while (!f_if) this_thread::yield();
		lock.lock();
		//puts("run if");
		IF(NULL);
		//puts("fin if");
		f_if = 0;
	}
}

void _ID(){
	unique_lock<mutex> lock(l_id);
	while (!fin) {
		lock.unlock();
		while (!f_id) this_thread::yield();
		lock.lock();
		//puts("run id");
		ID(NULL);
		//puts("fin id");
		f_id = 0;
	}
}

void _EX(){
	unique_lock<mutex> lock(l_ex);
	while (!fin) {
		lock.unlock();
		while (!f_ex) this_thread::yield();
		lock.lock();
		//puts("run ex");
		EX(NULL);
		//puts("fin ex");
		f_ex = 0;
	}
}

void _MEM(){
	unique_lock<mutex> lock(l_mem);
	while (!fin) {
		lock.unlock();
		while (!f_mem) this_thread::yield();
		lock.lock();
		//puts("run mem");
		MEM(NULL);
		//puts("fin mem");
		f_mem = 0;
	}
}

void run() {
	fin = 0;
	f_if = f_id = f_ex = f_mem = 0;
	thread _mem(_MEM), _ex(_EX), _id(_ID), _if(_IF);
	while (!fin) {
		if (pc == 390) {
			//printf("%d\n", pc);
		}
		WB(NULL);
		l_if.lock(); l_id.lock();
		l_ex.lock(); l_mem.lock();

		f_if = f_id = f_ex = f_mem = 1;

		l_if.unlock(); l_id.unlock();
		l_ex.unlock(); l_mem.unlock();

		while (f_if || f_id || f_ex || f_mem) {}
	}
	_if.join(); _id.join();
	_ex.join(); _mem.join();
	//fprintf(stderr, "%d %d %.4lf\n", suc, fail, (double)1.0 * suc / (suc + fail));
}

int main(int argc, char *argv[]){
	mips mip;
	mip.init(argv[1]);
	init(mip.cpu, mip.text_label["main"]);
	run();
	return 0;
}