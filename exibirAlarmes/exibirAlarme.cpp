/*	A principio tem apenas a thread primaria, pois so precisa aguardar os eventos de bloqueio e ESC */

#include <iostream>
#include <Windows.h>
#include<conio.h>
#include "CheckForError.h"

HANDLE event_A;
HANDLE event_ESC;
HANDLE event_dummy;

// Criação de tipos para indicar o estado da thread
enum estado {
	BLOQUEADO,
	DESBLOQUEADO
};

int main() {
	printf("Hello, I'm exibirAlarme.cpp\n");

	// Abre handles para os evento a criados
	event_ESC = OpenEvent(
		EVENT_ALL_ACCESS,			//Acesso irrestrito ao evento
		FALSE,
		(LPWSTR)"EventoESC"
	);
	if (!event_ESC)
		printf("Erro na abertura do handle para event_ESC! Codigo = %d\n", GetLastError());

	event_A = OpenEvent(
		EVENT_ALL_ACCESS,			//Acesso irrestrito ao evento
		FALSE,
		(LPWSTR)"Evento_5"
	);
	if (!event_A)
		printf("Erro na abertura do handle para event_A! Codigo = %d\n", GetLastError());

	// Trata o recebimento dos eventos
	HANDLE eventos[2] = { event_A, event_ESC};
	HANDLE eventosBloqueado[2] = { event_A, event_ESC};
	DWORD ret;
	int nTipoEvento = 0;
	estado estadoLeitura = DESBLOQUEADO; // Thread começa  desbloqueada

	int mensagem = 0;	

	do {
		if (estadoLeitura == DESBLOQUEADO) {
			// TESTE-PG: ---- PLACEHOLDER PRA ESPERAR RECEBER A MENSAGEM NO MAILSLOT ---- //
			printf("Thread esperando evento\n");
			ret = WaitForMultipleObjects(
				2,			// Espera 3 eventos 
				eventos,	// Array de eventos que espera
				FALSE,		// Espera o que acontecer primeiro
				INFINITE	// Espera por tempo indefinido
			);
			CheckForError((ret >= WAIT_OBJECT_0) && (ret < WAIT_OBJECT_0 + 3));
			nTipoEvento = ret - WAIT_OBJECT_0;

			if (nTipoEvento == 0) {
				printf("Tarefa de leitura do CLP de exibicao dos alarmes criticos foi bloqueada \n");
				estadoLeitura = BLOQUEADO;
			}
			else if (nTipoEvento == 1) {
				printf("Tecla ESC digitada, encerrando o programa... \n");
			}
			//else if (nTipoEvento == 2) {
			//  TESTE-PG: ---- PLACEHOLDER PRA ESPERAR RECEBER A MENSAGEM NO MAILSLOT ---- //	
			//}
			
		}
		else {  //Estado de bloqueio
			ret = WaitForMultipleObjects(
				2,					// Espera 3 eventos 
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
	
	std::cout << "Aplicacao encerrada, pressione Enter para fechar o prompt." << std::endl;
	_getch();

	return EXIT_SUCCESS;
}