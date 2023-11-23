/*	A principio tem apenas a thread primaria, pois so precisa aguardar os eventos de bloqueio e ESC */
#include "../bibliotecaTp.h"
#include <iostream>
#include <iomanip>
#include <Windows.h>
#include <conio.h>
#include "CheckForError.h"


HANDLE event_A;
HANDLE event_ESC;
HANDLE event_mailslot;
HANDLE event_mailslotCriado;
HANDLE hMailslot;

using namespace std;


int main() {
	printf("Tarefa de exibicao de alarmes em execucao\n");

	almType MsgBuffer;
	BOOL bStatus;
	DWORD dwBytesLidos;

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

	event_mailslot = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE,
		FALSE,
		(LPWSTR)"event_mailslot"
	);
	if (!event_mailslot)
		printf("Erro na criacao de do evento mailslot! Codigo = %d\n", GetLastError());

	event_mailslotCriado = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE,
		FALSE,
		(LPWSTR)"event_mailslotCriado"
	);
	if (!event_mailslotCriado)
		printf("Erro na criacao de do evento mailslotCriado Codigo = %d\n", GetLastError());

	hMailslot = CreateMailslot(
		L"\\\\.\\mailslot\\MyMailslot",
		0,
		MAILSLOT_WAIT_FOREVER,
		NULL);
	CheckForError(hMailslot != INVALID_HANDLE_VALUE);

	SetEvent(event_mailslotCriado);

	// Trata o recebimento dos eventos
	HANDLE eventos[3] = { event_A, event_ESC, event_mailslot };
	HANDLE eventosBloqueado[2] = { event_A, event_ESC };
	DWORD ret;
	int nTipoEvento = 0;
	estado estadoLeitura = DESBLOQUEADO; // Thread começa  desbloqueada

	int mensagem = 0;



	do {
		if (estadoLeitura == DESBLOQUEADO) {

			ret = WaitForMultipleObjects(
				3,			// Espera 2 eventos 
				eventos,	// Array de eventos que espera
				FALSE,		// Espera o que acontecer primeiro
				INFINITE	// Espera por tempo indefinido
			);
			CheckForError((ret >= WAIT_OBJECT_0) && (ret < WAIT_OBJECT_0 + 1));
			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {
				printf("Tarefa de leitura do CLP de exibicao dos alarmes criticos foi bloqueada \n");
				estadoLeitura = BLOQUEADO;
			}
			else if (nTipoEvento == 1) {
				printf("Tecla ESC digitada, encerrando o programa... \n");
			}
			else if (nTipoEvento == 2) { // recebeu mailslot
				bStatus = ReadFile(hMailslot, &MsgBuffer, sizeof(almType), &dwBytesLidos, NULL);
				CheckForError(bStatus);
				cout << setw(2) << setfill('0') << MsgBuffer.timestamp.wHour << ":"
					<< setw(2) << setfill('0') << MsgBuffer.timestamp.wMinute << ":"
					<< setw(2) << setfill('0') << MsgBuffer.timestamp.wSecond
					<< " NSEQ: " << setw(5) << setfill('0') << MsgBuffer.nSeq << " ";
			   printMensagemAlarme(MsgBuffer.id);
			}

		}
		else {  //Estado de bloqueio
			ret = WaitForMultipleObjects(
				2,					// Espera 2 eventos 
				eventosBloqueado,	// Array de eventos que espera
				FALSE,				// Espera o que acontecer primeiro
				INFINITE			// Espera por tempo indefinido
			);
			CheckForError((ret >= WAIT_OBJECT_0) && (ret < WAIT_OBJECT_0 + 3));

			nTipoEvento = ret - WAIT_OBJECT_0;
			if (nTipoEvento == 0) {
				printf("Tarefa de leitura do CLP de exibicao dos alarmes criticos foi desbloqueada \n");
				estadoLeitura = DESBLOQUEADO;
			}
			else if (nTipoEvento == 1) {
				printf("Tecla ESC digitada, encerrando o programa... \n");
			}
		}
	} while (nTipoEvento != 1); // Ate ESC ser escolhido

	return EXIT_SUCCESS;
}

