#include "../bibliotecaTp.h"
#include <iostream>
#include <iomanip>
#include <Windows.h>
#include <process.h>
#include <conio.h>
#include "../CheckForError.h"
#define _CHECKERROR	1	// Ativa fun��o CheckForError

HANDLE event_A;
HANDLE event_ESC;
HANDLE event_mailslotCriado;
HANDLE hMailslot;
BOOL ESC;
using namespace std;

estado estadoExibirAlarme = DESBLOQUEADO; // Thread começa  desbloqueada

// Casting para terceiro e sexto parâmetros da função _beginthreadex
typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;

// Declaração das funcões executadas pelas threads
DWORD WINAPI ExibirAlarme();
DWORD WINAPI MonitoraEvento();


int main() {
	printf("Tarefa de exibicao de alarmes em execucao\n");

	HANDLE hThreads[2];
	DWORD dwThreadExibirAlarmeID, dwThreadMonitoraEventoId;
	DWORD dwRet;
	ESC = FALSE;


	// Abre handles para os evento a criados
	event_ESC = OpenEvent(
		EVENT_MODIFY_STATE | SYNCHRONIZE,
		FALSE,
		(LPWSTR)"EventoESC"
	);
	if (!event_ESC)
		printf("Erro na abertura do handle para event_ESC! Codigo = %d\n", GetLastError());

	event_A = OpenEvent(
		EVENT_MODIFY_STATE | SYNCHRONIZE,
		FALSE,
		(LPWSTR)"Evento_5"
	);
	if (!event_A)
		printf("Erro na abertura do handle para event_A! Codigo = %d\n", GetLastError());

	event_mailslotCriado = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE,
		FALSE,
		(LPWSTR)"event_mailslotCriado"
	);
	if (!event_mailslotCriado)
		printf("Erro na criacao de do evento mailslotCriado Codigo = %d\n", GetLastError());

	// --- CRIACAO DAS THREADS SECUNDARIAS --- //
	hThreads[0] = (HANDLE)_beginthreadex(
		NULL,										// Opcoes de seguranca (default)
		0,											// Tamanho da pilha (default)
		(CAST_FUNCTION)ExibirAlarme,				// Logica da thread (casting necessário)
		NULL,										// Argumentos da thread (nao tem)
		0,											// Flags da thread (nao tem)	
		(CAST_LPDWORD)&dwThreadExibirAlarmeID		// Variavel que guarda o Id (casting necessario)
	);
	if (!hThreads[0])
		printf("Erro na criacao da thread ExibirAlarme Codigo = %d\n", GetLastError());

	hThreads[1] = (HANDLE)_beginthreadex(
		NULL,										// Opcoes de seguranca (default)
		0,											// Tamanho da pilha (default)
		(CAST_FUNCTION)MonitoraEvento,				// Logica da thread (casting necessário)
		NULL,										// Argumentos da thread (nao tem)
		0,											// Flags da thread (nao tem)	
		(CAST_LPDWORD)&dwThreadMonitoraEventoId		// Variavel que guarda o Id (casting necessario)
	);
	if (!hThreads[1])
		printf("Erro na criacao da thread MonitoraEvento! Codigo = %d\n", GetLastError());

	// Aguarda t?rmino das threads
	dwRet = WaitForMultipleObjects(2, hThreads, TRUE, INFINITE);
	CheckForError((dwRet >= WAIT_OBJECT_0) && (dwRet < WAIT_OBJECT_0 + 2));

	// Fecha todos os handles de objetos do kernel
	for (int i = 0; i < 2; ++i)
		CloseHandle(hThreads[i]);

	CloseHandle(event_A);
	CloseHandle(event_ESC);
	CloseHandle(event_mailslotCriado);
	CloseHandle(hMailslot);

	return EXIT_SUCCESS;
}

DWORD WINAPI ExibirAlarme() {

	almType MsgBuffer;
	BOOL bStatus;
	DWORD dwBytesLidos;
	DWORD messages;
	DWORD bytesNextMessage;
	DWORD messagesSize;

	hMailslot = CreateMailslot(
		L"\\\\.\\mailslot\\MyMailslot",
		0,
		MAILSLOT_WAIT_FOREVER,
		NULL);
	CheckForError(hMailslot != INVALID_HANDLE_VALUE);
	SetEvent(event_mailslotCriado);
	
		do {
			if (estadoExibirAlarme == DESBLOQUEADO) {
				if (GetMailslotInfo(hMailslot, NULL, &bytesNextMessage, &messages, &messagesSize) == FALSE) {
					cout << "Erro ao obter informações do mailslot. Código de erro: " << GetLastError() << endl;
					CloseHandle(hMailslot);
					return 1;
				}
				if (messages > 0) { // Existem mensagens no mailslot a serem lidas
					do {
						bStatus = ReadFile(hMailslot, &MsgBuffer, sizeof(almType), &dwBytesLidos, NULL);
						CheckForError(bStatus);
						cout << setw(2) << setfill('0') << MsgBuffer.timestamp.wHour << ":"
							<< setw(2) << setfill('0') << MsgBuffer.timestamp.wMinute << ":"
							<< setw(2) << setfill('0') << MsgBuffer.timestamp.wSecond
							<< " NSEQ: " << setw(5) << setfill('0') << MsgBuffer.nSeq << " ";
						printMensagemAlarme(MsgBuffer.id);
						messages--;

					} while (messages != 0); // Enquanto tiver mensagens no mailslot (messages > 0 ) a thread vai ler 
											 // e printar esses alarmes 
				}
			}
		} while (ESC==FALSE);

	return 0;
}

DWORD WINAPI MonitoraEvento() {
	
	// Eventos que a thread tem que monitorar o acontecimento 
	HANDLE eventos[2] = { event_ESC, event_A};
	int nTipoEvento;
	DWORD ret;

	do {
		ret = WaitForMultipleObjects(
			2,					// Espera 2 eventos 
			eventos,			// Array de eventos que espera
			FALSE,				// Espera o que acontecer primeiro
			INFINITE			// Espera por tempo indefinido
		);
		CheckForError((ret >= WAIT_OBJECT_0) && (ret < WAIT_OBJECT_0 + 2));
		nTipoEvento = ret - WAIT_OBJECT_0;
		
		if (nTipoEvento == 1) {
			if (estadoExibirAlarme == BLOQUEADO) estadoExibirAlarme = DESBLOQUEADO;
			else  estadoExibirAlarme = BLOQUEADO; 
		}
		else if (nTipoEvento == 0) {
			printf("Tecla ESC digitada, encerrando o programa... \n");
			ESC = TRUE;
		}
	} while (nTipoEvento != 0);

	return 0;
}