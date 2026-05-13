#pragma once

#include <vector>
#include <string>

#include "Config.h"
#include "Task.h"
#include "CPU.h"
#include "Scheduler.h"

// Snapshot — foto completa do sistema em um tick, usada para retroceder (Requisito 1.5.2)
struct Snapshot {
    int relogio;
    std::vector<CPU> estado_cpus;

    struct TaskState {
        int    id;
        int    tempo_restante;
        Estado estado;
    };
    std::vector<TaskState> estado_tarefas;
};

class Simulador {
private:
    int relogio_global;         // contador de ticks do sistema (Requisito 1.1)
    int quantum;                // período máximo de execução por tarefa
    int qtde_cpus;              // quantidade de processadores

    std::vector<CPU>   cpus;        // lista de processadores simulados
    std::vector<Task*> all_tasks;   // todas as tarefas carregadas do arquivo (TCBs)
    std::vector<Task*> prontos;     // tarefas aguardando execução

    Scheduler* escalonador;         // algoritmo de escalonamento ativo (polimórfico)

    std::vector<Snapshot> historico; // pilha de estados para retroceder

    // Métodos auxiliares privados
    void verificar_chegada_tarefas(); // move tarefas de all_tasks para prontos
    void salvar_estado();             // registra snapshot no histórico
    void do_tick();                   // lógica pura de um tick (sem salvar snapshot)

public:
    Simulador(Config& config);
    ~Simulador();

    // Controle da simulação
    void step_forward();   // salva estado e avança 1 tick (modo passo-a-passo)
    void step_backward();  // restaura o tick anterior
    void run_complete();   // executa até o fim sem intervenção humana

    // Verificadores de estado
    bool simulacao_concluida() const;
    int  get_relogio_atual()   const { return relogio_global; }

    // Getters para a interface gráfica
    const std::vector<CPU>&   get_cpus()      const { return cpus; }
    const std::vector<Task*>& get_all_tasks() const { return all_tasks; }

    void imprimir_status();

    // Modifica manualmente o estado de qualquer tarefa (Requisito 3.4)
    // Atualiza tambem a fila de prontos e as CPUs para manter consistencia interna.
    void modificar_estado_tarefa(int id, Estado novo_estado);
};
