#define _CRT_RAND_S
#include<Windows.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <ctime>
#include <iomanip>

#define NUM_ALARMES 10

using namespace std;
// Criação de tipos para indicar o estado da thread
enum estado {
	BLOQUEADO,
	DESBLOQUEADO
};

//// Estrutura dos alarmes
struct almType {
	int nSeq;
	int id;
	SYSTEMTIME timestamp;
};

//// Estrutura das mensagens
struct msgType {
	int nSeq;
	int diag;
	int id;
	float presInt;
	float presInj;
	float temp;
	SYSTEMTIME timestamp;
};

// --- FUNCOES AUXILIARES --- //

int setDIAG() {
	unsigned int rand_num;
	
	if (rand_s(&rand_num) != 0)
		printf("rand_s falhou em setDIAG()\n");

	int resultado = rand_num % 70;
	// if (resultado >= 55)
	// 	return 55;
	// else
		return resultado;
}


float setPRESS() {
	unsigned int rand_num;
	if (rand_s(&rand_num) != 0)
		printf("rand_s falhou em setPRESS()\n");

	int numSorteado = rand_num % 2001 + 1000;
	if (numSorteado % 10 == 0) numSorteado++;
	float num = numSorteado / 10.0;

	return num;
}

float setTEMP() {
	unsigned int rand_num;
	if (rand_s(&rand_num) != 0)
		printf("rand_s falhou em setTEMP()\n");

	int numSorteado = rand_num % 10001 + 10000;
	if (numSorteado % 10 == 0) numSorteado++;
	float num = numSorteado / 10;

	return num;
}

// Para o ID do alarme
int setID() {
	unsigned int rand_num;
	if (rand_s(&rand_num) != 0)
		printf("rand_s falhou em setID()\n");

  // Gera numero aleatorio entre 3 e NUM_ALARMES + 3
	int numSorteado = rand_num % NUM_ALARMES + 3;
	return numSorteado;
}

// Parse de tempo para string
string getTIME(SYSTEMTIME tempo) {
	ostringstream ss;
	ss << tempo.wHour << ":" << tempo.wMinute << ":" << tempo.wSecond;
	string temp = ss.str();
	return temp;
}

// Cria mensagens como struct 
void produzMensagem(msgType& mensagem, int ID, int NSEQ_aux) {

	GetSystemTime(&mensagem.timestamp);
	mensagem.nSeq = NSEQ_aux;
	mensagem.id = ID;
	mensagem.diag = setDIAG();

	if (mensagem.diag == 55) {	// Mensagem de falha no CLP
		mensagem.presInj = 0;
		mensagem.presInt = 0;
		mensagem.temp = 0;
	}
	else {		// Mensagem normal de dados do processo 
		mensagem.presInj = setPRESS();
		mensagem.presInt = setPRESS();
		mensagem.temp = setTEMP();
	}
}

// Cria alarmes como struct 
void produzAlarme(almType& alarme, int NSEQ_aux, int ID=0) {
	GetSystemTime(&alarme.timestamp);
	alarme.nSeq = NSEQ_aux;
	if (ID == 0) alarme.id = setID();
	else alarme.id = ID;
	
}

// Retorna um tempo aleatorio entre 1s e 5s em pacotes de 100ns
int randTime1a5s(){
    unsigned int rand_num;
	  if (rand_s(&rand_num) != 0)
		  printf("rand_s falhou em randTime1a5s()\n");
    int tempo = (rand_num % 4001 + 1000) * 1e+4;
    return tempo;
}

// Retorna a descrição do alarme de acordo com seu ID
// 0 nao e usado
// 1 e 2 sao reservados para falha de hardware
// 3 a 12 sao alarmes de processo
void printMensagemAlarme(int id) {
	switch (id) {
		case 1:
			cout<< "FALHA HARDWARE CLP No. 1" << endl;
			break;
		case 2:
			cout << "FALHA HARDWARE CLP No. 2" << endl;
			break;
		case 3:
			cout << "ID: " << setw(2) << setfill('0') << id << " TEXTO ALARME 3" << endl;
			break;
		case 4:
			cout << "ID: " << setw(2) << setfill('0') << id << " TEXTO ALARME 4" << endl;
			break;
		case 5:
			cout << "ID: " << setw(2) << setfill('0') << id << " TEXTO ALARME 5" << endl;
			break;
		case 6:
			cout << "ID: " << setw(2) << setfill('0') << id << " TEXTO ALARME 6" << endl;
			break;
		case 7:
			cout << "ID: " << setw(2) << setfill('0') << id << " TEXTO ALARME 7" << endl;
			break;
		case 8:
			cout << "ID: " << setw(2) << setfill('0') << id << " TEXTO ALARME 8" << endl;
			break;
		case 9:
			cout << "ID: " << setw(2) << setfill('0') << id << " TEXTO ALARME 9" << endl;
			break;
		case 10:
			cout << "ID: " << setw(2) << setfill('0') << id << " TEXTO ALARME 10" << endl;
			break;
		case 11:
			cout << "ID: " << setw(2) << setfill('0') << id << " TEXTO ALARME 11" << endl;
			break;
		case 12: 
			cout << "ID: " << setw(2) << setfill('0') << id << " TEXTO ALARME 12" << endl;
			break;
		default:
			cout << "ID: " << setw(2) << setfill('0') << id << " TEXTO  " << endl;
	}    
}