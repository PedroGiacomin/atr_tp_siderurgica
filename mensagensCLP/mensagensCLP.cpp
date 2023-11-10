#define WIN32_LEAN_AND_MEAN 
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <conio.h>		// _getch
#include "CheckForError.h"
#include <string.h>
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;

// Casting para terceiro e sexto parâmetros da função _beginthreadex
typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;

// Criação de tipos para indicar o estado da thread
enum estado {
	BLOQUEADO,
	DESBLOQUEADO
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

//// Estrutura dos alarmes
//struct almType {
//	int nSeq;
//	int id;
//	SYSTEMTIME timestamp;
//};

// Variaveis Globais 
int NSEQ = 0;
int NSEQAlarme = 0;
int pLivre1 = 0;
int pLivre2 = 0;
int pOcupado1 = 0;
int pOcupado2 = 0;
msgType lista1[100];
msgType lista2[50];

// Declaração das funcões executadas pelas threads
DWORD WINAPI LeituraCLP(LPVOID index);
DWORD WINAPI RetiraMensagem();
DWORD WINAPI MonitoraAlarme();
DWORD WINAPI ExibeDadosProcesso();

// Declaração funções auxiliares da mensagem

int setDIAG();		// Define aleatoriamente o valor de DIAG  em um intervalo de 0 a 55

//string setNSEQ(int NSEQ_aux);		// Define o valor de NSEQ  formatado com 5 digítos 

float setPRESS();	// Define aleatoriamente o valor de uma pressão em um intervalo de 100 a 300 com precisão de 1 casa decimal

float setTEMP();	// Define aleatoriamente a temperatura em um intervalo de 1000 a 2000 com precisão de 1 casa decimal

int setID();		// Define aleatoriamente um ID de 0 a 99

string getTIME(SYSTEMTIME tempo);	// Retorna uma string contento a hora , o minuto e o segundo atual

void produzMensagem(msgType &mensagem, int ID, int NSEQ_aux); // Produz a mensagem de leitura do CLP

void produzAlarme(string &alarme);				// Produz alarme critico do CLP

//void getParametrosMensagemCLP(string& mensagem, string& NSEQ, string& ID, string& DIAG, string& pressInt, string& pressInj,
//	string& tempo, string& temp); // Obtem os parametros de uma mensagem 

//string getDIAG(string &mensagem); // Retorna o valor do campo DIAG de uma mensagem

// Criação de apontador para Mutexes e Semaforos 
HANDLE hLista1Livre;
HANDLE hLista2Livre;
HANDLE hLista1Ocup;
HANDLE hLista2Ocup;
HANDLE hMutexNSEQ;
HANDLE hMutexpLivre1;

// Criação de HANDLES para eventos  
HANDLE event_ESC;		// evento de termino
HANDLE event_M;			// eventos de bloqueio
HANDLE event_P;
HANDLE event_R;
HANDLE event_1;
HANDLE event_2;
HANDLE event_lista1;	// eventos de lista cheia
HANDLE event_lista2;

int main()
{
	HANDLE hThreads[5];
	DWORD dwThreadLeituraId, dwThreadRetiraMensagemId, dwThreadMonitoraAlarmeId, dwThreadExibeDados;
	DWORD dwExitCode = 0;
	DWORD dwRet;
	int i;

	// Criação de Mutex e Semaforos
	hLista1Livre = CreateSemaphore(NULL, 100, 100, L"Lista1Livre");
	hLista2Livre = CreateSemaphore(NULL, 50, 50, L"Lista2Livre");
	hLista1Ocup = CreateSemaphore(NULL, 0, 100, L"Lista1Ocup");
	hLista2Ocup = CreateSemaphore(NULL, 0, 50, L"Lista2Ocup");
	hMutexNSEQ = CreateMutex(NULL, FALSE, NULL);
	hMutexpLivre1 = CreateMutex(NULL, FALSE, NULL);


	// Atribuição de apontadores para eventos criados em outro processo
	event_ESC = OpenEvent(EVENT_ALL_ACCESS, FALSE, (LPWSTR)"EventoESC");
	if (!event_ESC)
		printf("Erro na abertura do handle para event_1 Codigo = %d\n", GetLastError());

	event_M = OpenEvent(EVENT_ALL_ACCESS, FALSE, (LPWSTR)"Evento_2");
	if (!event_M)
		printf("Erro na abertura do handle para event_M Codigo = %d\n", GetLastError());
	
	event_R = OpenEvent(EVENT_ALL_ACCESS, FALSE, (LPWSTR)"Evento_3");
	if (!event_R)
		printf("Erro na abertura do handle para event_R! Codigo = %d\n", GetLastError());
	
	event_P = OpenEvent(EVENT_ALL_ACCESS, FALSE, (LPWSTR)"Evento_4");
	if (!event_P)
		printf("Erro na abertura do handle para event_P! Codigo = %d\n", GetLastError());

	event_1 = OpenEvent(EVENT_ALL_ACCESS, FALSE, (LPWSTR)"Evento_0");
	if (!event_1)
		printf("Erro na abertura do handle para event_1 Codigo = %d\n", GetLastError());

	event_2 = OpenEvent(EVENT_ALL_ACCESS, FALSE, (LPWSTR)"Evento_1");
	if (!event_2)
		printf("Erro na abertura do handle para event_2! Codigo = %d\n", GetLastError());

	event_lista1 = OpenEvent(EVENT_ALL_ACCESS, FALSE, (LPWSTR)"Evento_L1");
	if (!event_lista1)
		printf("Erro na abertura do handle para event_lista1! Codigo = %d\n", GetLastError());

	event_lista2 = OpenEvent(EVENT_ALL_ACCESS, FALSE, (LPWSTR)"Evento_L2");
	if (!event_lista2)
		printf("Erro na abertura do handle para event_lista2! Codigo = %d\n", GetLastError());
;

	for (i = 0; i < 2; ++i) {	// cria 2 threads de leitura do CLP
		hThreads[i] = (HANDLE)_beginthreadex(
			NULL,
			0,
			(CAST_FUNCTION)LeituraCLP,	// casting necessário
			(LPVOID)i,
			0,
			(CAST_LPDWORD)&dwThreadLeituraId	// casting necessário
		);

		if (hThreads[i]) printf("Thread de Leitura do CLP %d criada com Id= %0x \n", i, dwThreadLeituraId);
	}	

	// Cria a thread de retirada de mensagens 
	hThreads[2] = (HANDLE)_beginthreadex( 
		NULL,
		0,
		(CAST_FUNCTION)RetiraMensagem,	// casting necessário
		NULL,
		0,
		(CAST_LPDWORD)&dwThreadRetiraMensagemId);	// casting necessário
	if (hThreads[2]) printf("Thread de Retirada de Mensagens %d criada com Id= %0x \n", 2, dwThreadRetiraMensagemId);

	// Cria a thread de monitoração de alarme e leitura do CLP
	hThreads[3] = (HANDLE)_beginthreadex( 
		NULL,
		0,
		(CAST_FUNCTION)MonitoraAlarme,	// casting necessário
		NULL,
		0,
		(CAST_LPDWORD)&dwThreadMonitoraAlarmeId);	// casting necessário
	if (hThreads[3]) printf("Thread de Monitoracao de Alarme %d criada com Id= %0x \n", 3, dwThreadMonitoraAlarmeId);

	// Cria a thread de monitoração de alarme e leitura do CLP
	hThreads[4] = (HANDLE)_beginthreadex( 
		NULL,
		0,
		(CAST_FUNCTION)ExibeDadosProcesso,	// casting necessário
		NULL,
		0,
		(CAST_LPDWORD)&dwThreadExibeDados);	// casting necessário
	if (hThreads[4]) printf("Thread de Exibicao de Dados do processo %d criada com Id= %0x \n", 4, dwThreadExibeDados);	



	// Aguarda término das threads
	dwRet = WaitForMultipleObjects(5, hThreads, TRUE, INFINITE);
	CheckForError(dwRet == WAIT_OBJECT_0);

	// Fecha todos os handles de objetos do kernel
	for (int i = 0; i < 5 ; ++i)
		CloseHandle(hThreads[i]);

	// Fecha os handles dos objetos de sincronização
	CloseHandle(hMutexNSEQ);
	CloseHandle(hMutexpLivre1);
	CloseHandle(hLista1Livre);
	CloseHandle(hLista2Livre);
	CloseHandle(hLista1Ocup);
	CloseHandle(hLista2Ocup);

	return EXIT_SUCCESS;
}	// main

// --- LOGICA DAS THREADS --- //
DWORD WINAPI LeituraCLP(LPVOID index)
{
	int i = (int)index;
	int NSEQ_aux;
	msgType mensagem;
	DWORD dwStatus, ret;
	DWORD nTipoEvento;
	estado estadoLeitura = DESBLOQUEADO; // Thread começa  desbloqueada

	// Cria vetores de eventos
	HANDLE event_CLP[2] = { event_1, event_2 };
	HANDLE hEventoMutexNSEQ[3] = { event_CLP[i], event_ESC, hMutexNSEQ};
	HANDLE hEventoMutexpLivre1[3] = { event_CLP[i], event_ESC, hMutexpLivre1 };
	HANDLE hEventoCLPBloqueado[2] = { event_CLP[i], event_ESC};
	HANDLE hEventoLista1Livre[3] = { event_CLP[i], event_ESC, hLista1Livre};
	HANDLE hEventoLista2Livre[3] = { event_CLP[i], event_ESC, hLista2Livre};

	do {
		if (estadoLeitura == DESBLOQUEADO) {
			//// Espera a sua vez para usar NSEQ na criação da mensagem 	
			ret = WaitForMultipleObjects(3,	hEventoMutexNSEQ, FALSE, INFINITE);
			CheckForError(ret);
			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {			// Ocorreu um comando para bloquear a thread
				printf("Evento de bloqueio \n");
				estadoLeitura = BLOQUEADO; 
			}
			else if (nTipoEvento == 1) {	// Ocorreu um comando para encerrar o programa
				printf("Evento de encerramento \n"); 
			}
			else if (nTipoEvento == 2) {   // Mutex para acessar a varivael NSEQ está disponível
				NSEQ_aux = NSEQ;
				if (NSEQ != 99999) NSEQ++;
				else NSEQ = 0; 
			}
			ReleaseMutex(hMutexNSEQ); 
			
			// Uma concluida a secao critica de acesso ao NSEQ: libera mutex e produz mensagem
			produzMensagem(mensagem, i + 1, NSEQ_aux);
			
			// Verifica se existem posições livres na lista1, esperando pelo tempo maximo de 1ms
			ret = WaitForMultipleObjects(3,	hEventoLista1Livre,FALSE,1);
			CheckForError(ret);
			if (ret == WAIT_TIMEOUT) { // Caso o tempo for excedida então a lista esta cheia 
				printf("Tempo excedido \n");
				SetEvent(event_lista1); // Informa ao processo de leitura do teclado que a lista1 está cheia
				ret = WaitForMultipleObjects(3, hEventoLista1Livre, FALSE, INFINITE); // A thread fica bloqueada 
			}
			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {			// Ocorreu um comando para bloquear a thread
				printf("Evento de bloqueio \n");
				estadoLeitura = BLOQUEADO; 
			}
			else if (nTipoEvento == 1) {	// Ocorreu um comando para encerrar o programa
				printf("Evento de encerramento \n"); 
			}
			else if (nTipoEvento == 2) {	// Existe uma posiçao livre na lista 1
				ret = WaitForMultipleObjects(3, hEventoMutexpLivre1, FALSE, INFINITE);  
				CheckForError(ret);
				nTipoEvento = ret - WAIT_OBJECT_0;
				if (nTipoEvento == 0) {			// Ocorreu um comando para bloquear a thread
					printf("Evento de bloqueio \n");
					estadoLeitura = BLOQUEADO;
				}
				else if (nTipoEvento == 1) {	// Ocorreu um comando para encerrar o programa
					printf("Evento de encerramento \n");
				}
				else if (nTipoEvento == 2) {    // A variavel pLivre1 pode ser acessada 
					lista1[pLivre1] = mensagem;			// A mensagem é colocada na lista na posição livre
					pLivre1 = (pLivre1 + 1) % 100;
					ReleaseMutex(hMutexpLivre1);				// Libera o mutex da variavel pLivre1
					ReleaseSemaphore(hLista1Ocup, 1, NULL);		// Indica que existe mensagem a ser lida
				}
			}
			
		}
		else { // Se a thread estiver bloqueada ela deve esperar pelo comando de desbloqueio ou pelo encerramento 
			ret = WaitForMultipleObjects(2,	hEventoCLPBloqueado,FALSE,INFINITE);
			CheckForError(ret);
			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {
				printf("Evento de desbloqueio \n");
				estadoLeitura = DESBLOQUEADO; 
			}
			else if (nTipoEvento == 1) {
				printf("Evento de encerramento \n"); 
			}
		}
		Sleep(500);
	} while (nTipoEvento != 1);
	return 0;
}

DWORD WINAPI RetiraMensagem() {

	DWORD ret, nTipoEvento;
	estado estadoRetiraMensagem = DESBLOQUEADO;
	HANDLE hEventoLista1Ocup[3] = { event_R, event_ESC, hLista1Ocup };
	HANDLE hEventoRetiraMensagemBloqueada[2] = { event_R, event_ESC};
	msgType mensagem;
	
	do {
		if (estadoRetiraMensagem == DESBLOQUEADO) {
			ret = WaitForMultipleObjects(3, hEventoLista1Ocup,FALSE,INFINITE);
			CheckForError(ret);
			
			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {
				printf("Evento de bloqueio \n");
				estadoRetiraMensagem = BLOQUEADO;
			}
			else if (nTipoEvento == 1) {
				printf("Evento de encerramento \n");
			}
			else if (nTipoEvento == 2) {		// Existem mensagens a serem lidas da lista1 
				mensagem = lista1[pOcupado1];	// Le mensagem da lista 1

				if (mensagem.diag != 55) {		
					WaitForSingleObject(hLista2Livre, INFINITE);	// Verifica se tem espaco na lista 2 para escrever
					lista2[pLivre2] = mensagem;						// Escreve mensagem na lista 2
					pLivre2 = (pLivre2 + 1) % 50;					
					ReleaseSemaphore(hLista2Ocup, 1, NULL);			// Sinaliza que tem mensagens a serem lidas na lista 2
				}
				else {
					// pipes ou mailslots 
					
				}
				pOcupado1 = (pOcupado1 + 1) % 100;
				ReleaseSemaphore(hLista1Livre, 1, NULL);
			}
		}
		else {
			ret = WaitForMultipleObjects(2,	hEventoRetiraMensagemBloqueada,	FALSE, INFINITE	);
			CheckForError(ret);
			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {
				printf("Evento de desbloqueio \n");
				estadoRetiraMensagem = DESBLOQUEADO; 
			}
			else if (nTipoEvento == 1) {
				printf("Evento de encerramento \n");
			}
		}

	} while (nTipoEvento != 1);

	return 0;
}

DWORD WINAPI MonitoraAlarme() {
	estado estadoMonitoraAlarme = DESBLOQUEADO;
	DWORD nTipoEvento, ret;
	HANDLE hEventoMonitoraAlarme[2] = { event_M, event_ESC };
	string alarme;

	do {
		if (estadoMonitoraAlarme == DESBLOQUEADO) {
			// Define aleatoriamente um intervalo entre 1 a 5 s para disparar o alarme 
			int tempoDormindo = rand() % 4001 + 1000;
			// Espera pelo o encerramento do programa ou pelo bloqueio enquanto o alarme não é disparado
			ret = WaitForMultipleObjects(2,	hEventoMonitoraAlarme, FALSE, tempoDormindo);
			CheckForError(ret);

			if (ret == WAIT_TIMEOUT) {	// Quando exceder o tempo lanca o alarme
				//produzAlarme(alarme);
				//envia por mailslot para o processo exibir alarme
			}
			else {						// Mas se antes disso receber um comando do teclado
				nTipoEvento = ret - WAIT_OBJECT_0;
				if (nTipoEvento == 0) {
					printf("Evento de bloqueio \n");
					estadoMonitoraAlarme = BLOQUEADO;
				}
				else if (nTipoEvento == 1) {
					printf("Evento de encerramento \n");
				}
			}
		}

		else {
			ret = WaitForMultipleObjects(2,hEventoMonitoraAlarme,FALSE,	INFINITE);
			CheckForError(ret);
			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {
				printf("Evento de desbloqueio \n");
				estadoMonitoraAlarme = DESBLOQUEADO;
			}
			else if (nTipoEvento == 1) {
				printf("Evento de encerramento \n");
			}
		}
	} while(nTipoEvento != 1);
	return 0;
}

DWORD WINAPI ExibeDadosProcesso() {
	estado estadoExibeDados = DESBLOQUEADO;
	DWORD nTipoEvento, ret;
	HANDLE hEventoLista2Ocup[3] = { event_P, event_ESC, hLista2Ocup };
	HANDLE hEventoExibeDadosBloqueado[2] = { event_P, event_ESC };
	msgType mensagem;

	do {
		if (estadoExibeDados == DESBLOQUEADO) {
			ret = WaitForMultipleObjects(3,	hEventoLista2Ocup,	FALSE, INFINITE	);
			CheckForError(ret);

			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {			// Recebeu comando de bloqueio
				printf("Evento de bloqueio \n");
				estadoExibeDados = BLOQUEADO;
			}	
			else if (nTipoEvento == 1) {	// Recebeu comando de encerramento
				printf("Evento de encerramento \n");
			}
			else if (nTipoEvento == 2) {	// Existem mensagens a serem lidas na lista 2
				mensagem = lista2[pOcupado2]; // Le a mensagem
	
				cout << getTIME(mensagem.timestamp)	<< 
					" NSEQ: "	<< std::setw(5) << std::setfill('0') << mensagem.nSeq  <<
					" ID: "		<< mensagem.id		<< 
					" PR INT: " << mensagem.presInt << 
					" PR N2: "	<< mensagem.presInj << 
					" TEMP: "	<< mensagem.temp	<< endl;
				pOcupado2 = (pOcupado2 + 1) % 50;
				ReleaseSemaphore(hLista2Livre, 1, NULL);	// Sinaliza que liberou um espaco na lista 2
			}
		}
		else {
			ret = WaitForMultipleObjects(2,	hEventoExibeDadosBloqueado,	FALSE, INFINITE);
			CheckForError(ret);

			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {
				printf("Evento de desbloqueio \n");
				estadoExibeDados = DESBLOQUEADO; continue;
			}
			else if (nTipoEvento == 1) {
				printf("Evento de encerramento \n"); break;
			}
		}

	} while (nTipoEvento != 1);
	return 0;
}

// --- FUNCOES AUXILIARES --- //
// Cria mensagens como struct 
void produzMensagem(msgType &mensagem, int ID, int NSEQ_aux) {
	
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
//void produzAlarme(string& alarme) {
//
//	// Acessa com exclusividade o NSEQ e o System time
//	stringstream ss;
//	WaitForSingleObject(hMutexNSEQ, INFINITE);
//	//string SEQ = setNSEQ();
//	NSEQ++;
//	//string tempo = getTIME();
//	ReleaseMutex(hMutexNSEQ);
//	int ID = setID();
//
//	//ss << SEQ << ";" << ID << ";" << tempo;
//	alarme = ss.str();
//	
//}


int setDIAG() {
	int resultado = rand() % 70;
	if (resultado >= 55) return 55;
	else return resultado;
}

//string setNSEQ(int NSEQ_aux) {
//	ostringstream ss;
//	ss << std::setfill('0') << std::setw(5) << NSEQ_aux;
//	string numeroFormatado = ss.str(); // Salvar o valor formatado em uma variável
//	return numeroFormatado;
//}

float setPRESS() {
	int numSorteado = rand() % 2001 + 1000;
	if (numSorteado % 10 == 0) numSorteado++;
	float num = numSorteado / 10.0;

	return num;
}

float setTEMP() {

	int numSorteado = rand() % 10001 + 10000;
	if (numSorteado % 10 == 0) numSorteado++;
	float num = numSorteado / 10;
	
	return num;
}

// Para o ID do alarme
int setID() {
	int numSorteado = rand() % 99;
	return numSorteado;
}

// Parse de tempo para string
string getTIME(SYSTEMTIME tempo) {
	ostringstream ss;
	ss << tempo.wHour << ":" << tempo.wMinute << ":" << tempo.wSecond;
	string temp = ss.str();
	return temp;
}

void getParametrosMensagemCLP(string& mensagem, string& nSEQ, string& ID, string& DIAG, string& pressInt, string& pressInj, string& tempo, string& temp) {
	// Retorna o valor na variavel passada no segundo parametro do getline
	stringstream ss(mensagem);
	getline(ss, nSEQ, ';');
	getline(ss, ID, ';');
	getline(ss, DIAG, ';');
	getline(ss, pressInt, ';');
	getline(ss, pressInj, ';');
	getline(ss, temp, ';');
	getline(ss, tempo, ';');
}

string getDIAG(string& mensagem) {
	string auxiliar, diag;
	stringstream ss(mensagem);
	getline(ss, auxiliar, ';');
	getline(ss, auxiliar, ';');
	getline(ss, diag, ';');
	return diag;
}
