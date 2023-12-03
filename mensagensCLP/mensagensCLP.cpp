#define WIN32_LEAN_AND_MEAN 
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#define _CRT_RAND_S
#define _CHECKERROR	1	// Ativa fun��o CheckForError

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>	// _beginthreadex() e _endthreadex() 
#include <conio.h>		// _getch
#include "../CheckForError.h"
#include <string.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "../bibliotecaTp.h"


using namespace std;

// Casting para terceiro e sexto par�metros da fun��o _beginthreadex
typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;

// Variaveis Globais 
int NSEQ = 0;
int NSEQAlarme = 0;
int pLivre1 = 0;
int pLivre2 = 0;
int pOcupado1 = 0;
int pOcupado2 = 0;
msgType lista1[100];
msgType lista2[50];
BOOL ListaCheiaNotificada;

// Declara��o das func�es executadas pelas threads
DWORD WINAPI LeituraCLP(LPVOID index);
DWORD WINAPI RetiraMensagem();
DWORD WINAPI MonitoraAlarme();
DWORD WINAPI ExibeDadosProcesso();

// Cria��o de apontador para Mutexes e Semaforos 
HANDLE hLista1Livre;
HANDLE hLista2Livre;
HANDLE hLista1Ocup;
HANDLE hLista2Ocup;
HANDLE hMutexNSEQ;
HANDLE hMutexpLivre1;
HANDLE hMutexMailslot;
HANDLE hMutexListaCheia;

// Cria��o de HANDLES para eventos  
HANDLE event_ESC;		// evento de termino
HANDLE event_M;			// eventos de bloqueio
HANDLE event_P;
HANDLE event_R;
HANDLE event_1;
HANDLE event_2;
HANDLE event_lista1;	// eventos de lista cheia
HANDLE event_lista2;
HANDLE event_mailslotCriado;

// HANDLE para mailslot
HANDLE hMailslot;

// HANDLES para os temporizadores
HANDLE hTimerCLP[3];

estado estadoRetiraMensagem; 

int main()
{
	HANDLE hThreads[5];
	DWORD dwThreadLeituraId, dwThreadRetiraMensagemId, dwThreadMonitoraAlarmeId, dwThreadExibeDados;
	DWORD dwExitCode = 0;
	DWORD dwRet;
	bool bSucesso;
	int i;

	// ------- SEMAFOROS E MUTEXES -------//
	hLista1Livre = CreateSemaphore(NULL, 100, 100, L"Lista1Livre");
	hLista2Livre = CreateSemaphore(NULL, 50, 50, L"Lista2Livre");
	hLista1Ocup = CreateSemaphore(NULL, 0, 100, L"Lista1Ocup");
	hLista2Ocup = CreateSemaphore(NULL, 0, 50, L"Lista2Ocup");
	hMutexNSEQ = CreateMutex(NULL, FALSE, NULL);
	hMutexpLivre1 = CreateMutex(NULL, FALSE, NULL);
	hMutexMailslot = CreateMutex(NULL, FALSE, NULL);
	hMutexListaCheia = CreateMutex(NULL, FALSE, NULL);

	// ------- EVENTOS ------//
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

	event_mailslotCriado = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, (LPWSTR)"event_mailslotCriado");
	if (!event_mailslotCriado)
		printf("Erro na abertura do handle para  event_mailslotCriado! Codigo = %d\n", GetLastError());
	
	// ------- TEMPORIZADORES -------//
	// Criacao dos temporizadores para cada CLP
	hTimerCLP[0] = CreateWaitableTimer(NULL, FALSE, L"TimerCLP1");
	CheckForError(hTimerCLP[0]);
	hTimerCLP[1] = CreateWaitableTimer(NULL, FALSE, L"TimerCLP2");
	CheckForError(hTimerCLP[1]);
	hTimerCLP[2] = CreateWaitableTimer(NULL, TRUE, L"TimerCLP3");	//Tem que ser manual para poder trocar a temporizacao
	CheckForError(hTimerCLP[2]);

	// Inicializacao dos temporizadores
	LARGE_INTEGER Preset;
	Preset.QuadPart = 0;

	bSucesso = SetWaitableTimer(hTimerCLP[0], &Preset, 500, NULL, NULL, FALSE);
	CheckForError(bSucesso);
	bSucesso = SetWaitableTimer(hTimerCLP[1], &Preset, 500, NULL, NULL, FALSE);
	CheckForError(bSucesso);

	int timeCLP3 = randTime1a5s();	// Calcula tempo aleatorio
	bSucesso = SetWaitableTimer(hTimerCLP[2], &Preset, timeCLP3, NULL, NULL, FALSE);
	CheckForError(bSucesso);

	// ------- CRIACAO DAS THREADS -------//
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


	WaitForSingleObject(event_mailslotCriado, INFINITE); // Espera o servidor criar o mailslot

	hMailslot = CreateFile(
		L"\\\\.\\mailslot\\MyMailslot",
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	CheckForError(hMailslot != INVALID_HANDLE_VALUE);

	// ------- ENCERRAMENTO DA APLICACAO -------//
	// Aguarda t�rmino das threads
	dwRet = WaitForMultipleObjects(5, hThreads, TRUE, INFINITE);
	CheckForError((dwRet >= WAIT_OBJECT_0) && (dwRet < WAIT_OBJECT_0 + 5));

	// Fecha todos os handles das threads
	for (int i = 0; i < 5; ++i)
		CloseHandle(hThreads[i]);
	
	// Fecha os handles dos mutexes e semaforos
	CloseHandle(hMutexNSEQ);
	CloseHandle(hMutexMailslot);
	CloseHandle(hMutexListaCheia);
	CloseHandle(hMutexpLivre1);
	CloseHandle(hLista1Livre);
	CloseHandle(hLista2Livre);
	CloseHandle(hLista1Ocup);
	CloseHandle(hLista2Ocup);

	// Fecha os handles para os eventos
	CloseHandle(event_ESC);		
	CloseHandle(event_M);			
	CloseHandle(event_P);
	CloseHandle(event_R);
	CloseHandle(event_1);
	CloseHandle(event_2);
	CloseHandle(event_lista1);	
	CloseHandle(event_lista2);
	CloseHandle(event_mailslotCriado);

	// Fecha o handle para o mailslot
	CloseHandle(hMailslot);

	//Fecha o handle dos temporizadores
	for (int i = 0; i < 3; ++i)
		CloseHandle(hTimerCLP[i]);

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
	ListaCheiaNotificada = FALSE;

	// Cria vetores de eventos
	HANDLE event_CLP[2] = { event_1, event_2 };
	HANDLE hEventoMutexNSEQ[3] = { event_CLP[i], event_ESC, hMutexNSEQ };
	HANDLE hEventoMutexpLivre1[3] = { event_CLP[i], event_ESC, hMutexpLivre1 };
	HANDLE hEventoCLPBloqueado[2] = { event_CLP[i], event_ESC };
	HANDLE hEventoLista1Livre[3] = { event_CLP[i], event_ESC, hLista1Livre };
	HANDLE hEventoLista2Livre[3] = { event_CLP[i], event_ESC, hLista2Livre };
	HANDLE hEventoTimer[3] = { event_CLP[i], event_ESC, hTimerCLP[i]};

	do {
		// ESTADO DESBLOQUEADO
		if (estadoLeitura == DESBLOQUEADO) {
			// Espera pelo o encerramento do programa, pelo bloqueio, ou pelo timer
			ret = WaitForMultipleObjects(3, hEventoTimer, FALSE, INFINITE);
			CheckForError((ret >= WAIT_OBJECT_0) && (ret < WAIT_OBJECT_0 + 3));
			nTipoEvento = ret - WAIT_OBJECT_0;
			
			if (nTipoEvento == 0) {			// Bloqueio
				estadoLeitura = BLOQUEADO;
				continue;
			}
			else if (nTipoEvento == 1) {	// Encerramento
				printf("Tecla ESC digitada, encerrando o programa... \n");
				continue;
			}
			else if (nTipoEvento == 2) {	// Timer
				// Espera a sua vez para usar NSEQ na cria��o da mensagem 	
				ret = WaitForMultipleObjects(3, hEventoMutexNSEQ, FALSE, INFINITE);
				CheckForError((ret >= WAIT_OBJECT_0) && (ret < WAIT_OBJECT_0 + 3));
				nTipoEvento = ret - WAIT_OBJECT_0;
				if (nTipoEvento == 0) {			// Ocorreu um comando para bloquear a thread
					estadoLeitura = BLOQUEADO;
					continue;
				}
				else if (nTipoEvento == 1) {	// Ocorreu um comando para encerrar o programa
					printf("Tecla ESC digitada, encerrando o programa... \n");
					continue;
				}
				else if (nTipoEvento == 2) {   // Mutex para acessar a varivael NSEQ est� dispon�vel
					NSEQ_aux = NSEQ;
					if (NSEQ != 99999) NSEQ++;
					else NSEQ = 0;
					ReleaseMutex(hMutexNSEQ);
				}

				// Uma vez concluida a secao critica de acesso ao NSEQ: libera mutex e produz mensagem
				produzMensagem(mensagem, i + 1, NSEQ_aux);

				// Verifica se existem posi��es livres na lista1, esperando pelo tempo maximo de 10 ms
				ret = WaitForMultipleObjects(3, hEventoLista1Livre, FALSE, 10);
				CheckForError((ret >= WAIT_OBJECT_0) && (ret < WAIT_OBJECT_0 + 3));
				if (ret == WAIT_TIMEOUT) { // Caso o tempo for excedida ent�o a lista esta cheia 
					WaitForSingleObject(hMutexListaCheia, INFINITE);
					if (ListaCheiaNotificada == FALSE) {
						printf("Meu ID %d %d\n", i, ListaCheiaNotificada);
						ListaCheiaNotificada = TRUE;
						SetEvent(event_lista1); // Informa ao processo de leitura do teclado que a lista1 est� cheia
					}
					ReleaseMutex(hMutexListaCheia);
					ret = WaitForMultipleObjects(3, hEventoLista1Livre, FALSE, INFINITE); // A thread fica bloqueada 
					CheckForError((ret >= WAIT_OBJECT_0) && (ret < WAIT_OBJECT_0 + 3));
				}
				nTipoEvento = ret - WAIT_OBJECT_0;
				if (nTipoEvento == 0) {			// Ocorreu um comando para bloquear a thread
					estadoLeitura = BLOQUEADO;
					continue;
				}
				else if (nTipoEvento == 1) {	// Ocorreu um comando para encerrar o programa
					printf("Tecla ESC digitada, encerrando o programa... \n");
					continue;
				}
				else if (nTipoEvento == 2) {	// Existe uma posi�ao livre na lista 1
					ret = WaitForMultipleObjects(3, hEventoMutexpLivre1, FALSE, INFINITE);
					CheckForError((ret >= WAIT_OBJECT_0) && (ret < WAIT_OBJECT_0 + 3));
					nTipoEvento = ret - WAIT_OBJECT_0;
					if (nTipoEvento == 0) {			// Ocorreu um comando para bloquear a thread
						estadoLeitura = BLOQUEADO;
						continue;
					}
					else if (nTipoEvento == 1) {	// Ocorreu um comando para encerrar o programa
						printf("Tecla ESC digitada, encerrando o programa... \n");
						continue;
					}
					else if (nTipoEvento == 2) {    // A variavel pLivre1 pode ser acessada 
						lista1[pLivre1] = mensagem;			// A mensagem � colocada na lista na posi��o livre
						pLivre1 = (pLivre1 + 1) % 100;
						ReleaseMutex(hMutexpLivre1);				// Libera o mutex da variavel pLivre1
						ReleaseSemaphore(hLista1Ocup, 1, NULL);		// Indica que existe mensagem a ser lida
					}
				}
			}
		}
		// ESTADO BLOQUEADO
		else { 
			// Se a thread estiver bloqueada ela deve esperar pelo comando de desbloqueio ou pelo encerramento
			ret = WaitForMultipleObjects(2, hEventoCLPBloqueado, FALSE, INFINITE);
			CheckForError((ret >= WAIT_OBJECT_0) && (ret < WAIT_OBJECT_0 + 2));
			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {
				estadoLeitura = DESBLOQUEADO;
				continue;
			}
			else if (nTipoEvento == 1) {
				printf("Tecla ESC digitada, encerrando o programa... \n");
				continue;
			}
		}
	} while (nTipoEvento != 1);
	return 0;
}

DWORD WINAPI RetiraMensagem() {

	estado estadoRetiraMensagem = DESBLOQUEADO;
	DWORD ret, nTipoEvento;
	HANDLE hEventoLista1Ocup[3] = { event_R, event_ESC, hLista1Ocup };
	HANDLE hEventoRetiraMensagemBloqueada[2] = { event_R, event_ESC };
	HANDLE hEventoLista2Livre[3] = { event_R, event_ESC, hLista2Livre };
	msgType mensagem;
	almType alarmeCLP;
	BOOL bStatus;
	DWORD dwBytesEnviados;
	int NSEQ_aux;

	do {
		if (estadoRetiraMensagem == DESBLOQUEADO) {
			ret = WaitForMultipleObjects(3, hEventoLista1Ocup, FALSE, INFINITE);
			CheckForError((ret >= WAIT_OBJECT_0) && (ret < WAIT_OBJECT_0 + 3));

			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {
				estadoRetiraMensagem = BLOQUEADO;
				continue;
			}
			else if (nTipoEvento == 1) {
				printf("Tecla ESC digitada, encerrando o programa... \n");
				continue;
			}
			else if (nTipoEvento == 2) {		// Existem mensagens a serem lidas da lista1 
				mensagem = lista1[pOcupado1];	// Le mensagem da lista 1

				if (mensagem.diag == 55) {  // Deve produzir um alarme indicando falha de hardware no CLP 
					produzAlarme(alarmeCLP, mensagem.nSeq, mensagem.id);
					WaitForSingleObject(hMutexMailslot, INFINITE);
					bStatus = WriteFile(hMailslot, &alarmeCLP, sizeof(almType), &dwBytesEnviados, NULL);
					ReleaseMutex(hMutexMailslot);
				}
				else { // Diag !=55, a mensagem deve ser depositada na segunda lista 
					// Verifica se existem posicoes livres na lista2, esperando pelo tempo maximo de 10 ms
					ret = WaitForMultipleObjects(3, hEventoLista2Livre, FALSE, 10);
					CheckForError((ret >= WAIT_OBJECT_0) && (ret < WAIT_OBJECT_0 + 3));
					if (ret == WAIT_TIMEOUT) { // Caso o tempo for excedido entao a lista 2 esta cheia 
						SetEvent(event_lista2);
						ret = WaitForMultipleObjects(3, hEventoLista2Livre, FALSE, INFINITE); // A thread fica bloqueada 
						CheckForError((ret >= WAIT_OBJECT_0) && (ret < WAIT_OBJECT_0 + 3));
					}
					nTipoEvento = ret - WAIT_OBJECT_0;
					if (nTipoEvento == 0) {			// Ocorreu um comando para bloquear a thread
						estadoRetiraMensagem = BLOQUEADO;
						continue;
					}
					else if (nTipoEvento == 1) {	// Ocorreu um comando para encerrar o programa
						printf("Tecla ESC digitada, encerrando o programa... \n");
						continue;
					}
					else if (nTipoEvento == 2) {	// Existe uma posicao livre na lista 2
						lista2[pLivre2] = mensagem;						// Escreve mensagem na lista 2
						pLivre2 = (pLivre2 + 1) % 50;
						ReleaseSemaphore(hLista2Ocup, 1, NULL);			// Sinaliza que tem mensagens a serem lidas na lista 2
					}
				}
				pOcupado1 = (pOcupado1 + 1) % 100;
				ReleaseSemaphore(hLista1Livre, 1, NULL);
			}
		}
		else {
			ret = WaitForMultipleObjects(2, hEventoRetiraMensagemBloqueada, FALSE, INFINITE);
			CheckForError((ret >= WAIT_OBJECT_0) && (ret < WAIT_OBJECT_0 + 2));
			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {
				ListaCheiaNotificada = FALSE;
				estadoRetiraMensagem = DESBLOQUEADO;
				continue;
			}
			else if (nTipoEvento == 1) {
				printf("Tecla ESC digitada, encerrando o programa... \n");
				continue;
			}
		}

	} while (nTipoEvento != 1);

	return 0;
}

DWORD WINAPI MonitoraAlarme() {
	estado estadoMonitoraAlarme = DESBLOQUEADO;
	DWORD nTipoEvento, ret;
	HANDLE hEventoMonitoraAlarme[2] = { event_M, event_ESC};
	HANDLE hEventoTimer[3] = { event_M, event_ESC, hTimerCLP[2]};
	LARGE_INTEGER Preset;

	int NSEQ_aux;
	almType alarme;
	BOOL bStatus;
	DWORD dwBytesEnviados;

	do {
		if (estadoMonitoraAlarme == DESBLOQUEADO) {
			// Espera pelo o encerramento do programa, pelo bloqueio, ou pelo timer
			ret = WaitForMultipleObjects(3, hEventoTimer, FALSE, INFINITE);
			CheckForError((ret >= WAIT_OBJECT_0) && (ret < WAIT_OBJECT_0 + 3));
			nTipoEvento = ret - WAIT_OBJECT_0;

			if (nTipoEvento == 0) {			// Bloqueio
				estadoMonitoraAlarme = BLOQUEADO;
				continue;
			}
			else if (nTipoEvento == 1) {	// Encerramento
				printf("Tecla ESC digitada, encerrando o programa... \n");
				continue;
			}
			else if (nTipoEvento == 2) {	// Timer ( Thread deve ser executada ) 
				// Define o número sequencial do alarme a ser produzido
				NSEQ_aux = NSEQAlarme;
				if (NSEQAlarme != 99999) NSEQAlarme++;
				else NSEQAlarme = 0;
				// Produz o alarme e envia por meio do mailslot
				produzAlarme(alarme, NSEQ_aux);
				WaitForSingleObject(hMutexMailslot, INFINITE);
				bStatus = WriteFile(hMailslot, &alarme, sizeof(almType), &dwBytesEnviados, NULL);
				ReleaseMutex(hMutexMailslot);
				//Seta um novo tempo aleatorio
				bStatus = CancelWaitableTimer(hTimerCLP[2]);
				CheckForError(bStatus);

				Preset.QuadPart = (- 1)* randTime1a5s();
				bStatus = SetWaitableTimer(hTimerCLP[2], &Preset, 0, NULL, NULL, FALSE);
				CheckForError(bStatus);
			}

		}
		else {
			ret = WaitForMultipleObjects(2, hEventoMonitoraAlarme, FALSE, INFINITE);
			CheckForError((ret >= WAIT_OBJECT_0) && (ret < WAIT_OBJECT_0 + 2));
			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {
				estadoMonitoraAlarme = DESBLOQUEADO;
				continue;
			}
			else if (nTipoEvento == 1) {
				printf("Tecla ESC digitada, encerrando o programa... \n");
				continue;
			}
		}
	} while (nTipoEvento != 1);
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
			ret = WaitForMultipleObjects(3, hEventoLista2Ocup, FALSE, INFINITE);
			CheckForError((ret >= WAIT_OBJECT_0) && (ret < WAIT_OBJECT_0 + 3));

			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {			// Recebeu comando de bloqueio
				estadoExibeDados = BLOQUEADO;
				continue;
			}
			else if (nTipoEvento == 1) {	// Recebeu comando de encerramento
				printf("Tecla ESC digitada, encerrando o programa... \n");
				continue;
			}
			else if (nTipoEvento == 2) {	// Existem mensagens a serem lidas na lista 2
				mensagem = lista2[pOcupado2]; // Le a mensagem

				std::cout << getTIME(mensagem.timestamp) <<
					" NSEQ: "	<< setw(5) << setfill('0') << mensagem.nSeq <<
					" ID: "		<< mensagem.id <<
					" PR INT: " << setw(6) << setfill('0') << std::fixed << setprecision(1) << mensagem.presInt <<
					" PR N2: "	<< setw(6) << setfill('0') << std::fixed << setprecision(1) << mensagem.presInj <<
					" TEMP: "	<< setw(6) << setfill('0') << std::fixed << setprecision(1) << mensagem.temp << endl;
				pOcupado2 = (pOcupado2 + 1) % 50;
				ReleaseSemaphore(hLista2Livre, 1, NULL);	// Sinaliza que liberou um espaco na lista 2
			}
		}
		else {
			ret = WaitForMultipleObjects(2, hEventoExibeDadosBloqueado, FALSE, INFINITE);
			CheckForError((ret >= WAIT_OBJECT_0) && (ret < WAIT_OBJECT_0 + 2));

			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {
				estadoExibeDados = DESBLOQUEADO;
				continue;
			}
			else if (nTipoEvento == 1) {
				printf("Tecla ESC digitada, encerrando o programa... \n"); 
				continue;
			}
		}

	} while (nTipoEvento != 1);
	return 0;
}
