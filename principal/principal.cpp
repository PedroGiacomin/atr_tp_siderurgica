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

// Handles para os eventos correspondentes a cada tecla
HANDLE events[6];		// Ordem dos eventos : [1, 2, m, r, p, a]
HANDLE event_ESC;		// Para a tecla esc
/*HANDLE event_dummy;	*/	// TESTE PG - Placeholder para o evento de lista cheia

// Casting para terceiro e sexto par�metros da fun��o _beginthreadex
typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;

// Declara��o das func�es executadas pelas threads
DWORD WINAPI LeituraTeclado();
DWORD WINAPI MonitoraListas();

int main() {
	printf("Hello, I'm principal.cpp\n");
	
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
	//Criacao dos eventos de bloqueio
	for (int i = 0; i < NUM_EVENTOS; ++i) {
		std::string nomeEvento;
		nomeEvento = "Evento_" + std::to_string(i);
		std::cout << nomeEvento << std::endl;

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


	// --- CRIACAO DOS PROCESSOS --- //
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
	
	// --- CRIACAO DAS THREADS SECUNDARIAS --- //
	hThreads[0] = (HANDLE)_beginthreadex(
		NULL,										// Opcoes de seguranca (default)
		0,											// Tamanho da pilha (default)
		(CAST_FUNCTION)LeituraTeclado,				// Logica da thread (casting necess�rio)
		NULL,										// Argumentos da thread (nao tem)
		0,											// Flags da thread (nao tem)	
		(CAST_LPDWORD)&dwThreadTecladoId			// Variavel que guarda o Id (casting necessario)
	);
	if (!hThreads[0])
		printf("Erro na criacao da thread LeituraTeclado! Codigo = %d\n", GetLastError());

	hThreads[1] = (HANDLE)_beginthreadex(
		NULL,										// Opcoes de seguranca (default)
		0,											// Tamanho da pilha (default)
		(CAST_FUNCTION)MonitoraListas,				// Logica da thread (casting necess�rio)
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
		printf("Erro na criacao da thread MonitoraListas! Codigo = %d\n", GetLastError());

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

	std::cout << "Aplicacao encerrada, pressione Enter para fechar o prompt." << std::endl;
	std::cin.get();

	return EXIT_SUCCESS;
}

DWORD WINAPI LeituraTeclado() {
	BOOL status;	//retorno de erro
	int nTecla;		//tecla digitada
	
	// Espera uma tecla ser digitada ate ESC
	do {
		printf("Digite uma tecla para encerrar a tarefa que quiser\n");	//pode tirar isso depois
		nTecla = _getch();	// Isso aqui vai dar errado pra quando a lista estiver cheia

		switch (nTecla) {
		case TECLA_1:
			status = SetEvent(events[0]);
			printf("Enviou evento 1\n");
			break;
		case TECLA_2:
			status = SetEvent(events[1]);
			break;
		case TECLA_M:
			status = SetEvent(events[2]);
			break;
		case TECLA_R:
			status = SetEvent(events[3]);
			break;
		case TECLA_P:
			status = SetEvent(events[4]);
			break;
		case TECLA_A:
			status = SetEvent(events[5]);
			printf("Enviou evento A\n");
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

	Sleep(500);
	// Abertura do evento de lista cheia criado por outro processo
	HANDLE event_dummy = OpenEvent(
		EVENT_ALL_ACCESS,			//Acesso irrestrito ao evento
		FALSE,
		L"EventoDummy"
	);
	if (!event_dummy)
		printf("Erro na abertura do handle para event_dummy! Codigo = %d\n", GetLastError());
	//Pra quando tiver o evento de lista cheia
	HANDLE eventos[2] = { event_ESC, event_dummy};
	int nTipoEvento = -1;
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
		std::cout << nTipoEvento << std::endl;
		if (nTipoEvento == 1) {
			printf("Lista 1 cheia \n");
		}
		else if (nTipoEvento == 0) {
			printf("Evento de encerramento \n");
		}
	} while (nTipoEvento != 0);

	return 0;
}