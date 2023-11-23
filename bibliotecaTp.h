
#define _CRT_RAND_S
#include<Windows.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <ctime>



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
	if (resultado >= 55)
		return 55;
	else
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

	int numSorteado = rand_num % 99;
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


// Retorna a descrição do alarme de acordo com seu ID

void printMensagemAlarme(int id) {
	switch (id) {
		case 1:
			cout<< "FALHA HARDWARE CLP No. 1" << endl;
			break;
		case 2:
			cout << "FALHA HARDWARE CLP No. 2" << endl;
			break;
		default:
			cout << " " << endl;

	}
    
}