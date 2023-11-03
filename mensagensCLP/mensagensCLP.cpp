
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


// Casting para terceiro e sexto par�metros da fun��o _beginthreadex
typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;

// Declara��o das func�es executadas pelas threads
DWORD WINAPI LeituraCLP(LPVOID index);
DWORD WINAPI RetiraMensagem();
DWORD WINAPI MonitoraAlarme();
DWORD WINAPI ExibeDadosProcesso();

// Variaveis Globais 
int NSEQ = 0;
int NSEQAlarme = 0;
int pLivre1 = 0;
int pLivre2 = 0;
int pOcupado1 = 0;
int pOcupado2 = 0;
string lista1[50]; 
string lista2[50];

// Declara��o fun��es auxiliares

int setDIAG();  // Define aleatoriamente o valor de DIAG  em um intervalo de 0 a 55

string setNSEQ(); // Define o valor de NSEQ  formatado com 5 dig�tos 

string setPRESS();  // Define aleatoriamente o valor de uma press�o em um intervalo de 100 a 300 com precis�o de 1 casa decimal

string setTEMP(); // Define aleatoriamente a temperatura em um intervalo de 1000 a 2000 com precis�o de 1 casa decimal

int setID(); //  Define aleatoriamente um ID de 0 a 99

string getTIME(); // Retorna uma string contento a hora , o minuto e o segundo atual

void produzMensagem(string &mensagem, int ID); // Produz a mensagem de leitura do CLP

void getParametrosMensagemCLP(string& mensagem, string& NSEQ, string& ID, string&DIAG, string& pressInt, string& pressInj,
	string& tempo, string& temp); // Obtem os parametros de uma mensagem 

string getDIAG(string & mensagem); // Retorna o valor do campo DIAG de uma mensagem


// Cria��o de tipos para indicar o estado da thread
enum estado {
	BLOQUEADO,
	DESBLOQUEADO
};

// Cria��o de apontador para Mutexes e Semaforos 
HANDLE hLista1Livre;
HANDLE hLista2Livre;
HANDLE hLista1Ocup;
HANDLE hLista2Ocup;
HANDLE hMutexCLP;

// Cria��o de HANDLES para eventos 
HANDLE event_ESC;
HANDLE event_M;
HANDLE event_P;
HANDLE event_R;
HANDLE event_1;
HANDLE event_2;


//  TESTE-PG: Handle para um evento teste
HANDLE event_dummy;


int main()
{
	HANDLE hThreads[5];
	DWORD dwThreadLeituraId, dwThreadRetiraMensagemId, dwThreadMonitoraAlarmeId, dwThreadExibeDados;
	DWORD dwExitCode = 0;
	DWORD dwRet;
	int i;

	// Atribui��o de apontadores para eventos criados em outro processo
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
;

	for (i = 0; i < 2; ++i) {	// cria 2 threads de leitura do CLP
		hThreads[i] = (HANDLE)_beginthreadex(
			NULL,
			0,
			(CAST_FUNCTION)LeituraCLP,	// casting necess�rio
			(LPVOID)i,
			0,
			(CAST_LPDWORD)&dwThreadLeituraId	// casting necess�rio
		);

		if (hThreads[i]) printf("Thread de Leitura do CLP %d criada com Id= %0x \n", i, dwThreadLeituraId);
	}	

	// Cria a thread de retirada de mensagens 
	hThreads[2] = (HANDLE)_beginthreadex( 
		NULL,
		0,
		(CAST_FUNCTION)RetiraMensagem,	// casting necess�rio
		NULL,
		0,
		(CAST_LPDWORD)&dwThreadRetiraMensagemId);	// casting necess�rio
	if (hThreads[2]) printf("Thread de Retirada de Mensagens %d criada com Id= %0x \n", 2, dwThreadRetiraMensagemId);

	// Cria a thread de monitora��o de alarme e leitura do CLP
	hThreads[3] = (HANDLE)_beginthreadex( 
		NULL,
		0,
		(CAST_FUNCTION)MonitoraAlarme,	// casting necess�rio
		NULL,
		0,
		(CAST_LPDWORD)&dwThreadMonitoraAlarmeId);	// casting necess�rio
	if (hThreads[3]) printf("Thread de Monitoracao de Alarme %d criada com Id= %0x \n", 3, dwThreadMonitoraAlarmeId);

	// Cria a thread de monitora��o de alarme e leitura do CLP
	hThreads[4] = (HANDLE)_beginthreadex( 
		NULL,
		0,
		(CAST_FUNCTION)ExibeDadosProcesso,	// casting necess�rio
		NULL,
		0,
		(CAST_LPDWORD)&dwThreadExibeDados);	// casting necess�rio
	if (hThreads[4]) printf("Thread de Exibicao de Dados do processo %d criada com Id= %0x \n", 4, dwThreadExibeDados);

	//---  TESTE-PG:  EVENTO DUMMY QUE VAI SER SUBSTITUIDO PELO ENVIO POR MAILSLOT ---//
	event_dummy = CreateEvent(
		NULL,							// Seguranca (default)
		FALSE,							// Reset automatico
		FALSE,							// Inicia desativado
		L"EventoDummy"					// Nome do evento
	);
	if (!event_dummy)
		printf("Erro na criacao de do evento dummy! Codigo = %d\n", GetLastError());
	//---  TESTE-PG: EVENTO DUMMY QUE VAI SER SUBSTITUIDO PELO ENVIO POR MAILSLOT ---//
	

	// Cria��o de Mutex e Semaforos
	hLista1Livre = CreateSemaphore(NULL, 50, 50, L"Lista1Livre");
	hLista2Livre = CreateSemaphore(NULL, 50, 50, L"Lista2Livre");
	hLista1Ocup = CreateSemaphore(NULL, 0, 50, L"Lista1Ocup");
	hLista2Ocup = CreateSemaphore(NULL, 0, 50, L"Lista2Ocup");



	// Aguarda t�rmino das threads
	dwRet = WaitForMultipleObjects(5, hThreads, TRUE, INFINITE);
	CheckForError(dwRet == WAIT_OBJECT_0);

	// Fecha todos os handles de objetos do kernel
	for (int i = 0; i < 5 ; ++i)
		CloseHandle(hThreads[i]);

	// Fecha os handles dos objetos de sincroniza��o
	CloseHandle(hMutexCLP);
	CloseHandle(hLista1Livre);
	CloseHandle(hLista2Livre);
	CloseHandle(hLista1Ocup);
	CloseHandle(hLista2Ocup);

	return EXIT_SUCCESS;
}	// main

DWORD WINAPI LeituraCLP(LPVOID index)
{
	
	int i = (int)index;
	string mensagem = "";
	DWORD dwStatus, ret;
	DWORD nTipoEvento;
	estado estadoLeitura = DESBLOQUEADO; // Thread come�a  desbloqueada

	hMutexCLP = CreateMutex(NULL, FALSE, NULL);

	// Cria vetores de eventos
	HANDLE event_CLP[2] = { event_1, event_2 };
	HANDLE hEventoMutexCLP[3] = { event_CLP[i], event_ESC, hMutexCLP};
	HANDLE hEventoCLPBloqueado[2] = { event_CLP[i], event_ESC};
	HANDLE hEventoLista1Livre[3] = { event_CLP[i], event_ESC, hLista1Livre};
	HANDLE hEventoLista2Livre[3] = { event_CLP[i], event_ESC, hLista2Livre};

	do {
		if (estadoLeitura == DESBLOQUEADO) {
			produzMensagem(mensagem, i);
			// Espera a sua vez de usar pLivre e NSEQ
			ret = WaitForMultipleObjects(3,	hEventoMutexCLP, FALSE, INFINITE);
			CheckForError(ret);
			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) { // Ocorreu um comando para bloquear a thread
				printf("Evento de bloqueio \n");
				estadoLeitura = BLOQUEADO; continue;
			}
			else if (nTipoEvento == 1) { // Ocorreu um comando para encerrar o programa
				printf("Evento de encerramento \n"); break;
			}
			
			// Verifica se existem posi��es livres na lista1, esperando pelo tempo maximo de 1ms
			ret = WaitForMultipleObjects(3,	hEventoLista1Livre,FALSE,1);
			CheckForError(ret);
			if (ret == WAIT_TIMEOUT) { // Caso o tempo for excedido a lista 
				printf("Tempo excedido \n");
				SetEvent(event_dummy); // Informa ao processo de leitura do teclado que a lista1 est� cheia
				ret = WaitForMultipleObjects(3, hEventoLista1Livre, FALSE, INFINITE); // A thread fica bloqueada 
			}
			else {
				nTipoEvento = ret - WAIT_OBJECT_0;
				if (nTipoEvento == 0) { // Ocorreu um comando para bloquear a thread
					printf("Evento de bloqueio \n");
					estadoLeitura = BLOQUEADO; continue;
				}
				else if (nTipoEvento == 1) { // Ocorreu um comando para encerrar o programa
					printf("Evento de encerramento \n"); break;
				}
				// A mensagem � colocada na lista na posi��o livre
				lista1[pLivre1] = mensagem;
				pLivre1 = (pLivre1 + 1) % 50;
				ReleaseSemaphore(hLista1Ocup, 1, NULL); // Indica que existem mensagem a ser lida
				ReleaseMutex(hMutexCLP); // Libera o Mutex para que a outra thread possa utilizar 
			}
		}
		else { // Se a thread estiver bloqueada ela deve esperar pelo comando de desbloqueio ou pelo encerramento 
			ret = WaitForMultipleObjects(2,	hEventoCLPBloqueado,FALSE,INFINITE);
			CheckForError(ret);
			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {
				printf("Evento de desbloqueio \n");
				estadoLeitura = DESBLOQUEADO; continue;
			}
			else if (nTipoEvento == 1) {
				printf("Evento de encerramento \n"); break;
			}
		} 
		Sleep(500);
	} while (1);
	return 0;
}

DWORD WINAPI RetiraMensagem() {

	DWORD ret, nTipoEvento;
	estado estadoRetiraMensagem = DESBLOQUEADO;
	HANDLE hEventoLista1Ocup[3] = { event_R, event_ESC, hLista1Ocup };
	HANDLE hEventoRetiraMensagemBloqueada[2] = { event_R, event_ESC};
	do {
		if (estadoRetiraMensagem == DESBLOQUEADO) {
			ret = WaitForMultipleObjects(3, hEventoLista1Ocup,FALSE,INFINITE);
			CheckForError(ret);
			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {
				printf("Evento de bloqueio \n");
				estadoRetiraMensagem = BLOQUEADO; continue;
			}
			else if (nTipoEvento == 1) {
				printf("Evento de encerramento \n"); break;
			}
			string DIAG = getDIAG(lista1[pOcupado1]);

			if (DIAG != "55") { // Adiciona em outra lista circular 
				WaitForSingleObject(hLista2Livre, INFINITE);
				lista2[pLivre2] = lista1[pOcupado1];
				pLivre2 = (pLivre2 + 1) % 50;
				ReleaseSemaphore(hLista2Ocup, 1, NULL);
			}
			else { // pipes ou mailslots 
			}
			pOcupado1 = (pOcupado1 + 1) % 50;
			ReleaseSemaphore(hLista1Livre, 1, NULL);
		}
		else {
			ret = WaitForMultipleObjects(2,	hEventoRetiraMensagemBloqueada,	FALSE, INFINITE	);
			CheckForError(ret);
			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {
				printf("Evento de desbloqueio \n");
				estadoRetiraMensagem = DESBLOQUEADO; continue;
			}
			else if (nTipoEvento == 1) {
				printf("Evento de encerramento \n"); break;
			}
		}

	} while (1);
	return 0;
}

DWORD WINAPI MonitoraAlarme() {

	estado estadoMonitoraAlarme = DESBLOQUEADO;
	DWORD nTipoEvento, ret;
	HANDLE hEventoMonitoraAlarme[2] = { event_M, event_ESC};
	do {
		if (estadoMonitoraAlarme == DESBLOQUEADO) {
			// Define aleatoriamente um intervalo entre 1 a 5 s para disparar o alarme 
			int tempoDormindo = rand() % 4001 + 1000;
			// Espera pelo o encerramento do programa ou pelo bloqueio enquanto o alarme n�o � disparado
			ret = WaitForMultipleObjects(2,	hEventoMonitoraAlarme,FALSE,tempoDormindo);
			CheckForError(ret);
			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {
				printf("Evento de bloqueio \n");
				estadoMonitoraAlarme = BLOQUEADO; continue;
			}
			else if (nTipoEvento == 1) {
				printf("Evento de encerramento \n"); break;
			}
			stringstream ss;
			string tempo = getTIME();
			int ID = setID();
			// Produz a mensagem de alarme 
			ss << setfill('0') << setw(5) << NSEQ << ";" << setfill('0') << setw(2) << ID << ";" << tempo;
			string mensagemAlarme = ss.str();

			// ---  TESTE-PG: PLACEHOLDER PARA O ENVIO DA MENSAGEM POR MAILSLOTS ---// 
			//PulseEvent(event_dummy);
			// ---  TESTE-PG:  PLACEHOLDER PARA O ENVIO DA MENSAGEM POR MAILSLOTS ---// 
		}
		else {
			ret = WaitForMultipleObjects(2,hEventoMonitoraAlarme,FALSE,	INFINITE);
			CheckForError(ret);
			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {
				printf("Evento de desbloqueio \n");
				estadoMonitoraAlarme = DESBLOQUEADO; continue;
			}
			else if (nTipoEvento == 1) {
				printf("Evento de encerramento \n"); break;
			}
		}
	} while (1);
	return 0;
}

DWORD WINAPI ExibeDadosProcesso() {
	estado estadoExibeDados = DESBLOQUEADO;
	DWORD nTipoEvento, ret;
	HANDLE hEventoLista2Ocup[3] = { event_P, event_ESC, hLista2Ocup };
	HANDLE hEventoExibeDadosBloqueado[2] = { event_P, event_ESC};

	do {
		if (estadoExibeDados == DESBLOQUEADO) {
			string NSEQ, ID, DIAG, TEMP, PRESS_INJ, PRESS_INT, TEMPO;
			ret = WaitForMultipleObjects(3,	hEventoLista2Ocup,	FALSE, INFINITE	);
			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {
				printf("Evento de bloqueio \n");
				estadoExibeDados = BLOQUEADO; continue;
			}
			else if (nTipoEvento == 1) {
				printf("Evento de encerramento \n"); break;
			}
			getParametrosMensagemCLP(lista2[pOcupado2], NSEQ, ID, DIAG, PRESS_INT, PRESS_INJ, TEMPO, TEMP);
			cout << TEMPO << " NSEQ: " << NSEQ << " PR INT: " << PRESS_INT << " PR N2: " << PRESS_INJ << " TEMP: " << TEMP << endl;
			pOcupado2 = (pOcupado2 + 1) % 50;
			ReleaseSemaphore(hLista2Livre, 1, NULL);
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

	} while (1);
	return 0;
}

// Implementa��o das fun��es auxiliares
void produzMensagem(string& mensagem, int ID) {
	int DIAG = setDIAG();
	WaitForSingleObject(hMutexCLP, INFINITE);
	string SEQ = setNSEQ();
	string tempo = getTIME();
	stringstream ss;
	if (DIAG == 55) {
		ss << SEQ << ";" << ID + 1 << ";" << DIAG << ";" << "00000.0" << ";" << "00000.0" << ";" << "00000.0" <<":"<< tempo;
	}
	else {
		string PRESS_INT = setPRESS();
		string PRESS_INJ = setPRESS();
		string TEMP = setTEMP();
		ss << SEQ << ";" << ID + 1 << ";" << DIAG << ";" << PRESS_INT << ";" << PRESS_INJ << ";" << TEMP << ";" << tempo;

	}
	NSEQ++;
	ReleaseMutex(hMutexCLP);
	mensagem = ss.str();
}

int setDIAG() {
	int resultado = rand() % 70;
	if (resultado >= 55) return 55;
	else return resultado;
}

string setNSEQ() {
	ostringstream ss;
	ss << std::setfill('0') << std::setw(5) << NSEQ;
	string numeroFormatado = ss.str(); // Salvar o valor formatado em uma vari�vel
	return numeroFormatado;
}

string setPRESS() {
	int numSorteado = rand() % 2001 + 1000;
	if (numSorteado % 10 == 0) numSorteado++;
	double num = numSorteado / 10.0;
	ostringstream ss;
	ss << "0" << num;
	string numeroFormatado = ss.str();
	return numeroFormatado;

}

string setTEMP() {
	int numSorteado = rand() % 10001 + 10000;
	if (numSorteado % 10 == 0) numSorteado++;
	double num = numSorteado / 10;
	ostringstream ss;
	ss << num;
	string numeroFormatado = ss.str();
	return numeroFormatado;
}

int setID() {
	int numSorteado = rand() % 99;
	return numSorteado;
}

string getTIME() {
	SYSTEMTIME tempo;
	GetSystemTime(&tempo);
	ostringstream ss;
	ss << tempo.wHour << ":" << tempo.wMinute << ":" << tempo.wSecond;
	string temp = ss.str();
	return temp;
}

void getParametrosMensagemCLP(string& mensagem, string& NSEQ, string& ID, string& DIAG, string& pressInt, string& pressInj, string& tempo, string& temp) {
	// Retorna o valor na variavel passada no segundo parametro do getline
	stringstream ss(mensagem);
	getline(ss, NSEQ, ';');
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