#define WIN32_LEAN_AND_MEAN 
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#define _CRT_RAND_S

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

// Cria��o de tipos para indicar o estado da thread
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
struct almType {
	int nSeq;
	int id;
	SYSTEMTIME timestamp;
};

// Variaveis Globais 
int NSEQ = 0;
int pLivre1 = 0;
int pLivre2 = 0;
int pOcupado1 = 0;
int pOcupado2 = 0;
msgType lista1[100];
msgType lista2[50];
BOOL eventoListaCheia = FALSE;

// Declara��o das func�es executadas pelas threads
DWORD WINAPI LeituraCLP(LPVOID index);
DWORD WINAPI RetiraMensagem();
DWORD WINAPI MonitoraAlarme();
DWORD WINAPI ExibeDadosProcesso();

// Declara��o fun��es auxiliares da mensagem

int setDIAG();		// Define aleatoriamente o valor de DIAG  em um intervalo de 0 a 55

float setPRESS();	// Define aleatoriamente o valor de uma press�o em um intervalo de 100 a 300 com precis�o de 1 casa decimal

float setTEMP();	// Define aleatoriamente a temperatura em um intervalo de 1000 a 2000 com precis�o de 1 casa decimal

int setID();		// Define aleatoriamente um ID de 0 a 99

string getTIME(SYSTEMTIME tempo);	// Retorna uma string contento a hora , o minuto e o segundo atual

void produzMensagem(msgType &mensagem, int ID, int NSEQ_aux); // Produz a mensagem de leitura do CLP

void produzAlarme(almType &alarme, int NSEQ_aux);				// Produz alarme critico do CLP

//void getParametrosMensagemCLP(string& mensagem, string& NSEQ, string& ID, string& DIAG, string& pressInt, string& pressInj,
//	string& tempo, string& temp); // Obtem os parametros de uma mensagem 

//string getDIAG(string &mensagem); // Retorna o valor do campo DIAG de uma mensagem

// Cria��o de apontador para Mutexes e Semaforos 
HANDLE hLista1Livre;
HANDLE hLista2Livre;
HANDLE hLista1Ocup;
HANDLE hLista2Ocup;
HANDLE hMutexNSEQ;
HANDLE hMutexpLivre1;

// Cria��o de HANDLES para eventos  
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

	// Cria��o de Mutex e Semaforos
	hLista1Livre = CreateSemaphore(NULL, 100, 100, L"Lista1Livre");
	hLista2Livre = CreateSemaphore(NULL, 50, 50, L"Lista2Livre");
	hLista1Ocup = CreateSemaphore(NULL, 0, 100, L"Lista1Ocup");
	hLista2Ocup = CreateSemaphore(NULL, 0, 50, L"Lista2Ocup");
	hMutexNSEQ = CreateMutex(NULL, FALSE, NULL);
	hMutexpLivre1 = CreateMutex(NULL, FALSE, NULL);


	// Atribui��o de apontadores para eventos criados em outro processo
	event_ESC = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, (LPWSTR)"EventoESC");
	if (!event_ESC)
		printf("Erro na abertura do handle para event_1 Codigo = %d\n", GetLastError());

	event_M = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, (LPWSTR)"Evento_2");
	if (!event_M)
		printf("Erro na abertura do handle para event_M Codigo = %d\n", GetLastError());
	
	event_R = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, (LPWSTR)"Evento_3");
	if (!event_R)
		printf("Erro na abertura do handle para event_R! Codigo = %d\n", GetLastError());
	
	event_P = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, (LPWSTR)"Evento_4");
	if (!event_P)
		printf("Erro na abertura do handle para event_P! Codigo = %d\n", GetLastError());

	event_1 = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, (LPWSTR)"Evento_0");
	if (!event_1)
		printf("Erro na abertura do handle para event_1 Codigo = %d\n", GetLastError());

	event_2 = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, (LPWSTR)"Evento_1");
	if (!event_2)
		printf("Erro na abertura do handle para event_2! Codigo = %d\n", GetLastError());

	event_lista1 = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, (LPWSTR)"Evento_L1");
	if (!event_lista1)
		printf("Erro na abertura do handle para event_lista1! Codigo = %d\n", GetLastError());

	event_lista2 = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, (LPWSTR)"Evento_L2");
	if (!event_lista2)
		printf("Erro na abertura do handle para event_lista2! Codigo = %d\n", GetLastError());
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

	// Aguarda t�rmino das threads
	dwRet = WaitForMultipleObjects(5, hThreads, TRUE, INFINITE);
	CheckForError(dwRet == WAIT_OBJECT_0);

	// Fecha todos os handles de objetos do kernel
	for (int i = 0; i < 5 ; ++i)
		CloseHandle(hThreads[i]);

	// Fecha os handles dos objetos de sincroniza��o
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
	estado estadoLeitura = DESBLOQUEADO; // Thread come�a  desbloqueada

	// Cria vetores de eventos
	HANDLE event_CLP[2] = { event_1, event_2 };
	HANDLE hEventoMutexNSEQ[3] = { event_CLP[i], event_ESC, hMutexNSEQ};
	HANDLE hEventoMutexpLivre1[3] = { event_CLP[i], event_ESC, hMutexpLivre1 };
	HANDLE hEventoCLPBloqueado[2] = { event_CLP[i], event_ESC};
	HANDLE hEventoLista1Livre[3] = { event_CLP[i], event_ESC, hLista1Livre};
	HANDLE hEventoLista2Livre[3] = { event_CLP[i], event_ESC, hLista2Livre};

	do {
		if (estadoLeitura == DESBLOQUEADO) {
			//// Espera a sua vez para usar NSEQ na cria��o da mensagem 	
			ret = WaitForMultipleObjects(3,	hEventoMutexNSEQ, FALSE, INFINITE);
			CheckForError(ret);
			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {			// Ocorreu um comando para bloquear a thread
				printf("Tarefa Leitura do CLP%d foi bloqueada \n", i+1);
				estadoLeitura = BLOQUEADO; 
			}
			else if (nTipoEvento == 1) {	// Ocorreu um comando para encerrar o programa
				printf("Tecla ESC digitada, encerrando o programa... \n"); 
			}
			else if (nTipoEvento == 2) {   // Mutex para acessar a varivael NSEQ est� dispon�vel
				NSEQ_aux = NSEQ;
				if (NSEQ != 99999) NSEQ++;
				else NSEQ = 0; 
				ReleaseMutex(hMutexNSEQ);
			}
			
			// Uma vez concluida a secao critica de acesso ao NSEQ: libera mutex e produz mensagem
			produzMensagem(mensagem, i + 1, NSEQ_aux);
			
			// Verifica se existem posi��es livres na lista1, esperando pelo tempo maximo de 1ms
			ret = WaitForMultipleObjects(3,	hEventoLista1Livre,FALSE,1);
			CheckForError(ret);
			if (ret == WAIT_TIMEOUT) { // Caso o tempo for excedida ent�o a lista esta cheia 
				if (eventoListaCheia == TRUE) { // Se o evento j� foi sinalizado ent�o reseta a vari�vel auxiliar 
					eventoListaCheia = FALSE;
				}
				else {
					SetEvent(event_lista1);// Informa ao processo de leitura do teclado que a lista1 est� cheia
					eventoListaCheia = TRUE; // Indica para outra thread que o evento j� foi sinalizado
				} 
				ret = WaitForMultipleObjects(3, hEventoLista1Livre, FALSE, INFINITE); // A thread fica bloqueada 
			}
			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {			// Ocorreu um comando para bloquear a thread
				printf("Tarefa Leitura do CLP%d foi bloqueada \n", i + 1);
				estadoLeitura = BLOQUEADO; 
			}
			else if (nTipoEvento == 1) {	// Ocorreu um comando para encerrar o programa
				printf("Tecla ESC digitada, encerrando o programa... \n"); 
			}
			else if (nTipoEvento == 2) {	// Existe uma posi�ao livre na lista 1
				ret = WaitForMultipleObjects(3, hEventoMutexpLivre1, FALSE, INFINITE);  
				CheckForError(ret);
				nTipoEvento = ret - WAIT_OBJECT_0;
				if (nTipoEvento == 0) {			// Ocorreu um comando para bloquear a thread
					printf("Tarefa Leitura do CLP%d foi bloqueada \n", i + 1);
					estadoLeitura = BLOQUEADO;
				}
				else if (nTipoEvento == 1) {	// Ocorreu um comando para encerrar o programa
					printf("Tecla ESC digitada, encerrando o programa... \n");
				}
				else if (nTipoEvento == 2) {    // A variavel pLivre1 pode ser acessada 
					lista1[pLivre1] = mensagem;			// A mensagem � colocada na lista na posi��o livre
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
				printf("Tarefa Leitura do CLP%d foi desbloqueada \n", i + 1);
				estadoLeitura = DESBLOQUEADO; 
			}
			else if (nTipoEvento == 1) {
				printf("Tecla ESC digitada, encerrando o programa... \n");
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
			ret = WaitForMultipleObjects(3, hEventoLista1Ocup, FALSE, INFINITE);
			CheckForError(ret);
			
			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {
				printf("Tarefa de retirada de mensagens foi bloqueada \n");
				estadoRetiraMensagem = BLOQUEADO;
			}
			else if (nTipoEvento == 1) {
				printf("Tecla ESC digitada, encerrando o programa... \n");
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
					// pipes ou mailslots (printando como teste)
					/*cout << getTIME(mensagem.timestamp) <<
						" NSEQ: " << std::setw(5) << std::setfill('0') << mensagem.nSeq <<
						" ID: " << mensagem.id <<
						" PR INT: " << mensagem.presInt <<
						" PR N2: " << mensagem.presInj <<
						" TEMP: " << mensagem.temp <<
						" !!! FALHA !!! " << endl;*/
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
				printf("Tarefa de retirada de mensagens foi desbloqueada \n");
				estadoRetiraMensagem = DESBLOQUEADO; 
			}
			else if (nTipoEvento == 1) {
				printf("Tecla ESC digitada, encerrando o programa... \n");
			}
		}

	} while (nTipoEvento != 1);

	return 0;
}

DWORD WINAPI MonitoraAlarme() {
	estado estadoMonitoraAlarme = DESBLOQUEADO;
	DWORD nTipoEvento, ret;
	HANDLE hEventoMonitoraAlarme[3] = { event_M, event_ESC, hMutexNSEQ};
	int NSEQ_aux;
	almType alarme;

	// Define aleatoriamente um intervalo entre 1 a 5 s para disparar o alarme 
	int tempoDormindo = rand() % 4001 + 1000;

	do {
		if (estadoMonitoraAlarme == DESBLOQUEADO) {
			// Espera pelo o encerramento do programa, pelo bloqueio, ou pelo mutex do NSEQ
			ret = WaitForMultipleObjects(3,	hEventoMonitoraAlarme, FALSE, INFINITE);
			CheckForError(ret);

			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {			// Bloqueio
				printf("Tarefa de leitura do CLP de monitoracao dos alarmes criticos foi bloqueada \n");
				estadoMonitoraAlarme = BLOQUEADO;
			}
			else if (nTipoEvento == 1) {	// Encerramento
				printf("Tecla ESC digitada, encerrando o programa... \n");;
			}
			else if (nTipoEvento == 2) {	// Mutex
				NSEQ_aux = NSEQ;
				if (NSEQ != 99999) NSEQ++;	
				else NSEQ = 0;
				ReleaseMutex(hMutexNSEQ);
				produzAlarme(alarme, NSEQ_aux);

				// envia por mailslot (printando como teste)
				/*cout << getTIME(alarme.timestamp) <<
					" NSEQ: "	<< std::setw(5) << std::setfill('0') << alarme.nSeq <<
					" ID: "		<< alarme.id	<< 
					" !!!!!!! ALARME !!!!!!! "	<< endl;*/
			}
		}
		else {
			ret = WaitForMultipleObjects(2,hEventoMonitoraAlarme,FALSE,	INFINITE);
			CheckForError(ret);
			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {
				printf("Tarefa de leitura do CLP de monitoracao dos alarmes criticos foi desbloqueada \n");
				estadoMonitoraAlarme = DESBLOQUEADO;
			}
			else if (nTipoEvento == 1) {
				printf("Tecla ESC digitada, encerrando o programa... \n");
			}
		}
		Sleep(tempoDormindo);
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
				printf("Tarefa de exibicao dos dados do processo bloqueada \n");
				estadoExibeDados = BLOQUEADO;
			}	
			else if (nTipoEvento == 1) {	// Recebeu comando de encerramento
				printf("Tecla ESC digitada, encerrando o programa... \n");
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
				printf("Tarefa de exibicao dos dados do processo desbloqueada \n");
				estadoExibeDados = DESBLOQUEADO; continue;
			}
			else if (nTipoEvento == 1) {
				printf("Tecla ESC digitada, encerrando o programa... \n"); break;
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
void produzAlarme (almType& alarme, int NSEQ_aux) {
	GetSystemTime(&alarme.timestamp);
	alarme.nSeq = NSEQ_aux;
	alarme.id = setID();
}


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

