#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>		// _getch
#include <string>
#include <process.h>	// _beginthreadex() e _endthreadex()
#include "CheckForError.h"

// Teclas a serem digitadas
#define	ESC			0x1B
#define	TECLA_1		0x31
#define TECLA_2		0x32	
#define TECLA_M		0x6D
#define TECLA_R		0x72
#define TECLA_P		0x70
#define TECLA_A		0x61
#define NUM_EVENTOS 6

// Handles para os eventos
HANDLE events[6];
HANDLE event_ESC;
HANDLE event_lista1;
HANDLE event_lista2;
HANDLE event_mailslotCriado;

// Vetor para armazenar os estados(BLOQUEADO/DESBLOQUEADO) das threads
BOOL estadosThreads[6]={ TRUE, TRUE, TRUE, TRUE, TRUE, TRUE }; // Todas as threads começam desbloqueadas (TRUE);;

// Casting para terceiro e sexto parâmetros da função _beginthreadex
typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;

// Declaração das funcões executadas pelas threads
DWORD WINAPI LeituraTeclado();
DWORD WINAPI MonitoraListas();
void printEstadoThread(int numEvento);

int main() {
	printf("----- PAINEL DE CONTROLE ----- \n");
	printf("Digite a seguinte tecla para: \n");
	printf("   1 - bloquear/desbloquear a tarefa de leitura do CLP 1\n");
	printf("   2 - bloquear/desbloquear a tarefa de leitura do CLP 2\n");
	printf("   m - bloquear/desbloquear a tarefa de monitoracao de alarmes criticos\n");
	printf("   r - bloquear/desbloquear a tarefa de retirada de mensagens\n");
	printf("   p - bloquear/desbloquear a tarefa de exibicao de dados de processo\n");
	printf("   a - bloquear/desbloquear a tarefa de exibicao de alarmes criticos\n");
	printf("   ESC - encerrar o programa\n\n");

	// --- SETUP E VARIVEIS --- //
	//Caminhos relativos dos executaveis a serem inicializados
	LPCWSTR pathMensagensCLP = L"..\\Release\\mensagensCLP.exe";
	LPCWSTR pathExibirAlarme = L"..\\Release\\exibirAlarmes.exe";

	//Variaveis usadas na criacao dos processos
	STARTUPINFO siMensagensCLP, siExibirAlarme;
	PROCESS_INFORMATION piMensagensCLP, piExibirAlarme;

	//Para criacao das threads
	HANDLE hThreads[2]; // = { ThreadTeclado, ThreadMonitoraListas }
	DWORD dwThreadTecladoId, dwThreadMonitoraListasId;

	// Retornos de erro
	BOOL status;
	DWORD dwRet;

	// --- CRIACAO DOS EVENTOS --- //
	//Criacao dos eventos de bloqueio -> ordem = [1, 2, m, r, p, a]
	for (int i = 0; i < NUM_EVENTOS; ++i) {
		std::string nomeEvento;
		nomeEvento = "Evento_" + std::to_string(i);

		events[i] = CreateEvent(
			NULL,							// Seguranca (default)
			FALSE,							// Reset automatico
			FALSE,							// Inicia desativado
			(LPWSTR)nomeEvento.c_str()		// Nome do evento
		);
		if (!events[i])
			printf("Erro na criacao de do evento %d! Codigo = %d\n", i, GetLastError());
	}

	//Criacao do evento de encerramento
	event_ESC = CreateEvent(
		NULL,							// Seguranca (default)
		TRUE,							// Reset manual (acorda todas as threads simultaneamente)
		FALSE,							// Inicia desativado
		(LPWSTR)"EventoESC"				// Nome do evento
	);
	if (!event_ESC)
		printf("Erro na criacao de do evento ESC! Codigo = %d\n", GetLastError());

	//Criacao dos eventos de lista cheia
	event_lista1 = CreateEvent(
		NULL,							// Seguranca (default)
		FALSE,							// Reset automatico
		FALSE,							// Inicia desativado
		(LPWSTR)"Evento_L1"				// Nome do evento
	);
	if (!event_lista1)
		printf("Erro na criacao de do evento lista1! Codigo = %d\n", GetLastError());

	//Criacao dos eventos de lista cheia
	event_lista2 = CreateEvent(
		NULL,							// Seguranca (default)
		FALSE,							// Reset automatico
		FALSE,							// Inicia desativado
		(LPWSTR)"Evento_L2"				// Nome do evento
	);
	if (!event_lista2)
		printf("Erro na criacao de do evento lista1! Codigo = %d\n", GetLastError());

	event_mailslotCriado = CreateEvent(
		NULL,
		FALSE,
		FALSE,
		(LPWSTR)"event_mailslotCriado"
	);
	if (!event_mailslotCriado)
		printf("Erro na criacao de do evento mailslotCriado! Codigo = %d\n", GetLastError());

	// --- CRIACAO DOS PROCESSOS --- //
	//Cria processo ExibirAlarme
	ZeroMemory(&siExibirAlarme, sizeof(siExibirAlarme));
	siExibirAlarme.cb = sizeof(siExibirAlarme);
	ZeroMemory(&piExibirAlarme, sizeof(piExibirAlarme));

	status = CreateProcess(
		pathExibirAlarme,		// Nome do arquivo executavel
		NULL,					// Argumentos de linha de comando (nenhum)
		NULL,					// Atributos de seguranca do processo (default)
		NULL,					// Atributos de seguranca das threads (default)
		FALSE,					// Heranca de handles do processo criador (nao herda)
		CREATE_NEW_CONSOLE,		// Flag para inicializar com um console diferente do processo-pai
		NULL,					// Ambiente de criacao do processo (mesmo do pai)
		NULL,					// Diretorio corrente (mesmo do pai)
		&siExibirAlarme,		// Variavel que guarda informacoes de inicializacao
		&piExibirAlarme			// Struct que guarda o PID do processo entre outras infos
	);
	if (!status)
		printf("Erro na criacao de exibirAlarme! Codigo = %d\n", GetLastError());

	//Cria processo mensagensCLP
	ZeroMemory(&siMensagensCLP, sizeof(siMensagensCLP));
	siMensagensCLP.cb = sizeof(siMensagensCLP);
	ZeroMemory(&piMensagensCLP, sizeof(piMensagensCLP));

	status = CreateProcess(
		pathMensagensCLP,		// Nome do arquivo executavel
		NULL,					// Argumentos de linha de comando (nenhum)
		NULL,					// Atributos de seguranca do processo (default)
		NULL,					// Atributos de seguranca das threads (default)
		FALSE,					// Heranca de handles do processo criador (nao herda)
		CREATE_NEW_CONSOLE,		// Flag para inicializar com um console diferente do processo-pai
		NULL,					// Ambiente de criacao do processo (mesmo do pai)
		NULL,					// Diretorio corrente (mesmo do pai)
		&siMensagensCLP,		// Variavel que guarda informacoes de inicializacao
		&piMensagensCLP			// Struct que guarda o PID do processo entre outras infos
	);
	if (!status)
		printf("Erro na criacao de mensagensCLP! Codigo = %d\n", GetLastError());



	// --- CRIACAO DAS THREADS SECUNDARIAS --- //
	hThreads[0] = (HANDLE)_beginthreadex(
		NULL,										// Opcoes de seguranca (default)
		0,											// Tamanho da pilha (default)
		(CAST_FUNCTION)LeituraTeclado,				// Logica da thread (casting necessário)
		NULL,										// Argumentos da thread (nao tem)
		0,											// Flags da thread (nao tem)	
		(CAST_LPDWORD)&dwThreadTecladoId			// Variavel que guarda o Id (casting necessario)
	);
	if (!hThreads[0])
		printf("Erro na criacao da thread LeituraTeclado! Codigo = %d\n", GetLastError());

	hThreads[1] = (HANDLE)_beginthreadex(
		NULL,										// Opcoes de seguranca (default)
		0,											// Tamanho da pilha (default)
		(CAST_FUNCTION)MonitoraListas,				// Logica da thread (casting necessário)
		NULL,										// Argumentos da thread (nao tem)
		0,											// Flags da thread (nao tem)	
		(CAST_LPDWORD)&dwThreadMonitoraListasId		// Variavel que guarda o Id (casting necessario)
	);
	if (!hThreads[1])
		printf("Erro na criacao da thread MonitoraListas! Codigo = %d\n", GetLastError());


	// --- ENCERRAMENTO DO PROGRAMA --- //
	// ACHO que tem que esperar encerrar os processos ANTES, aqui

	// Aguarda termino das threads
	dwRet = WaitForMultipleObjects(2, hThreads, TRUE, INFINITE);
	if (dwRet != WAIT_OBJECT_0)
		printf("Erro no fechamento das threads! Codigo = %d\n", GetLastError());

	// Fecha handle de eventos de bloqueio
	for (int i = 0; i < NUM_EVENTOS; ++i) {
		status = CloseHandle(events[i]);
		if (!status)
			printf("Erro no fechamento do handle do evento %d! Codigo = %d\n", i, GetLastError());
	}

	// Fecha handle do evento ESC
	status = CloseHandle(event_ESC);
	if (!status)
		printf("Erro no fechamento do handle do evento ESC! Codigo = %d\n", GetLastError());

	// Fecha handle do evento lista1
	status = CloseHandle(event_lista1);
	if (!status)
		printf("Erro no fechamento do handle do evento de lista1! Codigo = %d\n", GetLastError());

	// Fecha handle do evento mailslotCriado
	status = CloseHandle(event_mailslotCriado);
	if (!status)
		printf("Erro no fechamento do handle do evento mailslotCriado! Codigo = %d\n", GetLastError());
	
	return EXIT_SUCCESS;
}

DWORD WINAPI LeituraTeclado() {
	BOOL status;	//retorno de erro
	int nTecla;		//tecla digitada

	// Espera uma tecla ser digitada ate ESC
	do {
		//printf("Aguardando comando\n");	
		nTecla = _getch();	// Isso aqui vai dar errado pra quando a lista estiver cheia

		switch (nTecla) {

		case TECLA_1:
			status = SetEvent(events[0]);
			printEstadoThread(0);
			break;

		case TECLA_2:
			status = SetEvent(events[1]);
			printEstadoThread(1);
			break;
		case TECLA_M:
			status = SetEvent(events[2]);
			printEstadoThread(2);
			break;
		case TECLA_R:
			status = SetEvent(events[3]);
			printEstadoThread(3);
			break;
		case TECLA_P:
			status = SetEvent(events[4]);
			printEstadoThread(4);
			break;
		case TECLA_A:
			status = SetEvent(events[5]);
			printEstadoThread(5);
			break;
		case ESC:
			status = SetEvent(event_ESC);
			break;
		}
		if (!status)
			printf("Erro no disparo do evento! Codigo = %d\n", GetLastError());
	} while (nTecla != ESC);

	return 0;
}

DWORD WINAPI MonitoraListas() {

	//Pra quando tiver o evento de lista cheia (1 ou 2)
	HANDLE eventos[3] = { event_ESC, event_lista1};
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
			printf("Lista 1 cheia \n");
		}
		else if (nTipoEvento == 0) {
			printf("Tecla ESC digitada, encerrando o programa... \n");
		}
	} while (nTipoEvento != 0);

	return 0;
}

void printEstadoThread(int numEvento) {

	switch (numEvento) {

	case 0:
		printf("Tarefa de Leitura do CLP1 ");
		break;
	case 1:
		printf("Tarefa de Leitura do CLP2 ");
		break;
	case 2:
		printf("Tarefa de Monitoracao dos Alarmes Criticos ");
		break;
	case 3:
		printf("Tarefa de Retirada de Mensagens ");
		break;
	case 4:
		printf("Tarefa de Exibir Dados do Processo ");
		break;
	case 5:
		printf("Tarefa de Exibir Alarmes ");
		break;
	}
	if (estadosThreads[numEvento] == TRUE) {
		printf("foi bloqueada\n");
		estadosThreads[numEvento] = FALSE;
	}
	else {
		printf("foi desbloqueada\n");
		estadosThreads[numEvento] = TRUE;
	}
}