#pragma once

#include <iostream>
#include <vector>
#include <string>

#include "Config.h"
#include "Task.h"   // Classe que representa o TCB
#include "CPU.h"    // Classe que representa o processador
#include "Scheduler.h" // Interface para SRTF e PRIOP

// Struct para Retroceder ou Undo (requisito 1.5.2)
struct Snapshot {
    int relogio;
    std::vector<CPU> estadoCPUs;

    struct TaskState {
        int id;
        int tempoRestante;
        std::string estado;
    };
    std::vector<TaskState> estadoTarefas;
};

class Kernel {
private:

    int relogio_global;                // Contador de ticks do sistema 
    int quantum;                      // Período máximo de execução por tarefa 
    int qtde_cpus;                    // Quantidade de processadores 

    // Estruturas de dados (Vetores)
	// Utilizamos os vetores como listas de tarefas e CPUs, pois eles permitem fácil adição e remoção de elementos, 
    // além de fornecerem acesso rápido por índice. 
    std::vector<CPU> cpus;            // Lista de processadores simulados 
    std::vector<Task*> all_tasks;  // Todas as tarefas carregadas do arquivo 
    std::vector<Task*> prontos;   // Tarefas aguardando execução 

    // Escalonador Polimórfico 
    // Ponteiro para o algoritmo escolhido (SRTF ou PRIOP)
    Scheduler* escalonador;

    // Histórico para retroceder
    std::vector<Snapshot> historico;

    // Métodos Auxiliares Privados (Encapsulamento)
    void verificarChegadaTarefas();     // Move tarefas de 'all_tasks' para 'prontos'
    void salvarEstado();                // Registra o snapshot atual no histórico

public:

	//Contrutor e Destrutor
    Kernel(Config& config);            
    ~Kernel();

    // Controle da Simulação
	void proximoTick();               // Avança 1 segundo
	void retroceder();                // Volta 1 segundo

    // Verificadores de Estado
    bool simulacaoConcluida() const;    // Retorna true se todas as tarefas terminaram
    int getRelogioAtual() const { return relogio_global; }

    // Getters para Interface (Gráfico de Gantt e Status)
    const std::vector<CPU>& getCPUs() const { return cpus; }
    const std::vector<Task*>& getAllTasks() const { return all_tasks; }

    // Relatórios
    void imprimirStatus();           // Exibe  o estado atual no console

};


