#ifndef MIPS_HPP
#define MIPS_HPP

#include <iostream>
#include <cstdio>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include "tokenscanner.hpp"
#include "code.hpp"
#define Src(i) (cur.offset?reg[cur.arg[i]]:cur.arg[i])
using namespace std;

const int MXC = 256;
const long long MXI = 2147483648;
const long long MXU = 4294967296;

class mips {
	vector<code> text;
	map<string, int> data_label;
	map<string, int> text_label;
	vector<string> n_label;
	int reg[35], data_buf;
	unsigned char data[4 * 1024 * 1024];

public:

	string pro_trans(string x) {
		string res = x;
		int i = 0, len = res.length();
		while (i < len) {
			if (res[i] == '\\') {
				switch (res[i + 1]) {
					case 'n':
						res.erase(i + 1, 1);
						res[i] = '\n';
						len--;
						break;
					case '\\':
						res.erase(i + 1, 1);
						res[i] = '\\';
						len--;
						break;
					case '\"':
						res.erase(i + 1, 1);
						res[i] = '\"';
						len--;
						break;
					case '\'':
						res.erase(i + 1, 1);
						res[i] = '\'';
						len--;
						break;
					case '0':
						res.erase(i + 1, 1);
						res[i] = '\0';
						len--;
						break;
				}
			}
			i++;
		}
		return res;
	}

	void init(const char* filename) {
		ifstream inf(filename);
		text.clear();
		string tmp, tmp2;
		char line[1000];
		tmp = "";
		data_buf = 0;
		memset(reg, 0, sizeof(reg));
		memset(data, 0, sizeof(data));
		codeinit();
		bool is_data = false;
		Tokenscanner buf;
		int i, j;
		while (!inf.eof()) {
			inf.getline(line, 999);
			buf.process(line);
			if (buf.dat.size() == 0) continue;
			if (buf.dat[0] == ".data") is_data = true;
			else if (buf.dat[0] == ".text") is_data = false;
			else if (buf.dat[0][buf.dat[0].length() - 1] == ':') {
				tmp = buf.dat[0]; tmp.pop_back();
				n_label.push_back(tmp);
			}
			else if (is_data) {
				if (buf.dat[0][buf.dat[0].length() - 1] == ':') {
					tmp = buf.dat[0]; tmp.pop_back();
					n_label.push_back(tmp);
				}
				else {
					int typ = dat_typ[buf.dat[0]];
					int tt, tn, i, j;
					switch (typ) {
						case 1:
							tt = stoi(buf.dat[1]);
							data_buf = ((data_buf - 2) / (1 << tt) + 1) * (1 << tt) + 1;
							break;
						case 2:
							while (n_label.size()) {
								tmp = n_label[n_label.size() - 1];
								data_label[tmp] = data_buf;
								n_label.pop_back();
							}
							tmp2 = "";
							i = 0;
							while (line[i] != '\"') i++;
							i++;
							while (line[i] != '\"') {
								tmp2 += line[i];
								i++;
							}
							tmp2 = pro_trans(tmp2);
							tt = tmp2.length();
							for(i = 0; i < tt; i++) data[data_buf + i] = tmp2[i];
							data_buf += tt;
							break;
						case 3:
							while (n_label.size()) {
								tmp = n_label[n_label.size() - 1];
								data_label[tmp] = data_buf;
								n_label.pop_back();
							}
							tmp2 = "";
							i = 0;
							while (line[i] != '\"') i++;
							i++;
							while (line[i] != '\"') {
								tmp2 += line[i];
								i++;
							}
							tmp2 = pro_trans(tmp2);
							tt = tmp2.length();
							for(i = 0; i < tt; i++) data[data_buf + i] = tmp2[i];
							data_buf += tt;
							data[data_buf] = 0;
							data_buf++;
							break;
						case 4:
							while (n_label.size()) {
								tmp = n_label[n_label.size() - 1];
								data_label[tmp] = data_buf;
								n_label.pop_back();
							}
							tt = buf.dat.size() - 1;
							for (i = 1; i <= tt; i++) {
								data[data_buf + i - 1] = stoi(buf.dat[i]);
							}
							data_buf += tt;
							break;
						case 5:
							while (n_label.size()) {
								tmp = n_label[n_label.size() - 1];
								data_label[tmp] = data_buf;
								n_label.pop_back();
							}
							tt = buf.dat.size() - 1;
							for (i = 1; i <= tt; i++) {
								tn = stoi(buf.dat[i]);
								for (j = 1; j >= 0; j--) {
									data[data_buf + (i - 1) * 2 + j] = tn % MXC;
									tn /= MXC;
								}
							}
							data_buf += tt * 2;
							break;
						case 6:
							while (n_label.size()) {
								tmp = n_label[n_label.size() - 1];
								data_label[tmp] = data_buf;
								n_label.pop_back();
							}
							tt = buf.dat.size() - 1;
							for (i = 1; i <= tt; i++) {
								tn = stoi(buf.dat[i]);
								for (j = 3; j >= 0; j--) {
									data[data_buf + (i - 1) * 4 + j] = tn % MXC;
									tn /= MXC;
								}
							}
							data_buf += tt * 4;
							break;
						case 7:
							data_buf += stoi(buf.dat[1]);
							break;
					}

				}
			}
			else {
				while (n_label.size()) {
					tmp = n_label[n_label.size() - 1];
					text_label[tmp] = text.size();
					n_label.pop_back();
				}
				code tcode(buf);
				text.push_back(tcode);
				//cout << text.size() - 1 << "\t" << line << endl;
			}
		}
		for (int i = 0; i < text.size(); i++) 
			if ((text[i].opr_type >= 26 && text[i].opr_type <= 39)|| text[i].opr_type == 41) {
				text[i].arg[2] = text_label[text[i].label];
			}
			else if (text[i].opr_type >= 43 && text[i].opr_type <= 49){
				if (text[i].label != "") {
					text[i].arg[1] = -data_label[text[i].label] - 1;
				}
			}
		reg[29] = 4 * 1024 * 1024 - 1;
	}

	long long tou(int x) {
		long long res = (long long)x > 0 ? x : (long long)MXU + x;
		return res;
	}

	int run() {
		//cout << "run\n";
		reg[34] = text_label["main"];
		long long tmp;
		unsigned long long tmp2;
		int i, len;
		int tot = 0;
		char stmp[1000];
		while (reg[34] < text.size()) {
			code cur = text[reg[34]];
			//if (reg[34] == 496) {
			//	printf("line : %d\n", reg[34]);
			//}
			reg[34]++;
			switch (cur.opr_type) {
				case 0:
					reg[cur.arg[0]] = reg[cur.arg[1]] + Src(2);
					break;
				case 1:
					reg[cur.arg[0]] = reg[cur.arg[1]] + Src(2);
					break;
				case 2:
					reg[cur.arg[0]] = reg[cur.arg[1]] + cur.arg[2];
					break;
				case 3:
					reg[cur.arg[0]] = reg[cur.arg[1]] - Src(2);
					break;
				case 4:
					reg[cur.arg[0]] = reg[cur.arg[1]] - Src(2);
					break;
				case 5:
					tmp = (long long)reg[cur.arg[1]] * Src(2);
					reg[cur.arg[0]] = tmp % MXI;
					break;
				case 6:
					tmp = (long long)reg[cur.arg[0]] * Src(1);
					reg[32] = tmp % MXI;
					reg[33] = tmp / MXI;
					break;
				case 7: 
					tmp2 = (unsigned long long)tou(reg[cur.arg[1]]) * tou(Src(2));
					reg[cur.arg[0]] = (unsigned long)(tmp % MXU);
					break;
				case 8:
					tmp2 = (unsigned long long)tou(reg[cur.arg[0]]) * tou(Src(1));
					reg[32] = (unsigned long)(tmp % MXU);
					reg[33] = (unsigned long)(tmp / MXU);
					break;
				case 9:
					reg[cur.arg[0]] = reg[cur.arg[1]] / Src(2);
					break;
				case 10:
					reg[32] = reg[cur.arg[0]] / Src(1);
					reg[33] = reg[cur.arg[0]] % Src(1);
					break;
				case 11:
					reg[cur.arg[0]] = tou(reg[cur.arg[1]]) / tou(Src(2));
					break;
				case 12:
					reg[32] = tou(reg[cur.arg[0]]) / tou(Src(1));
					reg[33] = tou(reg[cur.arg[0]]) % tou(Src(1));
					break;
				case 13: case 14:
					reg[cur.arg[0]] = reg[cur.arg[1]] ^ Src(2);
					break;
				case 15:
					reg[cur.arg[0]] = -reg[cur.arg[1]];
					break;
				case 16:
					reg[cur.arg[0]] = ~reg[cur.arg[1]];
					break;
				case 17:
					reg[cur.arg[0]] = reg[cur.arg[1]] % Src(2);
					break;
				case 18:
					reg[cur.arg[0]] = tou(reg[cur.arg[1]]) % tou(Src(2));
					break;
				case 19:
					reg[cur.arg[0]] = cur.arg[1];
					break;
				case 20:
					reg[cur.arg[0]] = (reg[cur.arg[1]] == Src(2));
					break;
				case 21:
					reg[cur.arg[0]] = (reg[cur.arg[1]] >= Src(2));
					break;
				case 22:
					reg[cur.arg[0]] = (reg[cur.arg[1]] > Src(2));
					break;
				case 23:
					reg[cur.arg[0]] = (reg[cur.arg[1]] <= Src(2));
					break;
				case 24:
					reg[cur.arg[0]] = (reg[cur.arg[1]] < Src(2));
					break;
				case 25:
					reg[cur.arg[0]] = (reg[cur.arg[1]] != Src(2));
					break;
				case 26:
					reg[34] = cur.arg[2];
					break;
				case 27:
					if (reg[cur.arg[0]] == Src(1)) reg[34] = cur.arg[2];
					break;
				case 28:
					if (reg[cur.arg[0]] != Src(1)) reg[34] = cur.arg[2];
					break;
				case 29:
					if (reg[cur.arg[0]] >= Src(1)) reg[34] = cur.arg[2];
					break;
				case 30:
					if (reg[cur.arg[0]] <= Src(1)) reg[34] = cur.arg[2];
					break;
				case 31:
					if (reg[cur.arg[0]] > Src(1)) reg[34] = cur.arg[2];
					break;
				case 32:
					if (reg[cur.arg[0]] < Src(1)) reg[34] = cur.arg[2];
					break;
				case 33:
					if (reg[cur.arg[0]] == 0) reg[34] = cur.arg[2];
					break;
				case 34:
					if (reg[cur.arg[0]] != 0) reg[34] = cur.arg[2];
					break;
				case 35:
					if (reg[cur.arg[0]] <= 0) reg[34] = cur.arg[2];
					break;
				case 36:
					if (reg[cur.arg[0]] >= 0) reg[34] = cur.arg[2];
					break;
				case 37:
					if (reg[cur.arg[0]] > 0) reg[34] = cur.arg[2];
					break;
				case 38:
					if (reg[cur.arg[0]] < 0) reg[34] = cur.arg[2];
					break;
				case 39:
					reg[34] = cur.arg[2];
					break;
				case 40:
					reg[34] = reg[cur.arg[0]];
					break;
				case 41:
					reg[31] = reg[34]; reg[34] = cur.arg[2];
					break;
				case 42:
					reg[31] = reg[34]; reg[34] = reg[cur.arg[0]];
					break;
				case 43:
				 	if (cur.arg[1] >= 0) reg[cur.arg[0]] = reg[cur.arg[1]];
				 	else reg[cur.arg[0]] = -cur.arg[1] - 1;
				 	break;
				case 44:
					if (cur.arg[1] >= 0) reg[cur.arg[0]] = data[reg[cur.arg[1]] + cur.offset];
					else reg[cur.arg[0]] = data[-cur.arg[1] - 1];
					break;
				case 45:
					if (cur.arg[1] >= 0) reg[cur.arg[0]] = data[reg[cur.arg[1]] + cur.offset] * MXC + data[reg[cur.arg[1]] + cur.offset + 1];
					else reg[cur.arg[0]] = data[-cur.arg[1] - 1] * MXC + data[-cur.arg[1] - 1 + 1];
					break;
				case 46:
				 	tmp = 0;
				 	if (cur.arg[1] >= 0) {
				 		for (i = 0; i < 4; i++) tmp = tmp * MXC + data[reg[cur.arg[1]] + cur.offset + i];
				 	}
				 	else {
				 		for (i = 0; i < 4; i++) tmp = tmp * MXC + data[-cur.arg[1] - 1 + i];
				 	}
					reg[cur.arg[0]] = tmp;
					break;
				case 47:
					tmp = tou(reg[cur.arg[0]]);
					if (cur.arg[1] >= 0) data[reg[cur.arg[1]] + cur.offset] = tmp;
					else data[-cur.arg[1] - 1] = reg[cur.arg[0]];
					break;
				case 48:
					tmp = tou(reg[cur.arg[0]]);
					if (cur.arg[1] >= 0) for (i = 1; i>=0; i--) data[reg[cur.arg[1]] + cur.offset + i] = tmp % MXC, tmp /= MXC;
					else for (i = 1; i>=0; i--) data[-cur.arg[1] - 1 + i] = tmp % MXC, tmp /= MXC;
					break;
				case 49:
					tmp = tou(reg[cur.arg[0]]);
					if (cur.arg[1] >= 0) for (i = 3; i>=0; i--) data[reg[cur.arg[1]] + cur.offset + i] = tmp % MXC, tmp /= MXC;
					else for (i = 3; i>=0; i--) data[-cur.arg[1] - 1 + i] = tmp % MXC, tmp /= MXC;
					break;
				case 50:
					reg[cur.arg[0]] = reg[cur.arg[1]];
					break;
				case 51:
					reg[cur.arg[0]] = reg[33];
					break;
				case 52:
					reg[cur.arg[0]] = reg[32];
					break;
				case 53:
					break;
				case 54:
					switch (reg[2]) {
						case 1:
							cout << reg[4];
							break;
						case 4:
							tmp = reg[4];
							while (data[tmp] != '\0') {
								cout << data[tmp]; tmp++;
							}
							break;
						case 5:
							cin >> reg[2];
							break;
						case 8:
							cin.getline(stmp, reg[5] - 1);
							len = strlen(stmp);
							if (len == 0) {
								cin.getline(stmp, reg[5] - 1);
								len = strlen(stmp);
							}
							for (i = 0; i < len; i++) data[reg[4] + i] = stmp[i];
							break;
						case 9:
							reg[2] = data_buf;
							data_buf += reg[4];
							break;
						case 10:
							return 0;
						case 17:
							return reg[4];
					}
					break;
			}
		}
		return 0;
	}

private:


};

#endif 