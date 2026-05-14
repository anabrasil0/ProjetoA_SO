#pragma once

#include <vector>
#include <string>

#include "Config.h"
#include "Task.h"
#include "CPU.h"
#include "Scheduler.h"
#include "GanttLog.h"

// Snapshot — foto completa do sistema em um tick, usada para retroceder (Requisito 1.5.2).
// Salvo por step_forward antes de cada do_tick(); restaurado por step_backward().
struct Snapshot {
    int              relogio;       // valor do relogio_global no momento do snapshot
    std::vector<CPU> estado_cpus;   // copia por valor das CPUs: captura tarefa_atual, ligada, quantum

    // Estado mínimo de cada tarefa necessário para desfazer um tick
    struct TaskState {
        int    id;             // identifica a qual tarefa este estado pertence
        int    tempo_restante; // progresso a ser restaurado (quanto faltava naquele tick)
        Estado estado;         // posição no ciclo de vida a ser restaurada
    };
    std::vector<TaskState> estado_tarefas; // um TaskState para cada tarefa do sistema
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

    GanttLog gantt_log; // historico de estados para o Gantt (Requisito 2)

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
    const GanttLog&           get_gantt_log() const { return gantt_log; }

    void imprimir_status();

    // Modifica manualmente o estado de qualquer tarefa (Requisito 3.4)
    // Atualiza tambem a fila de prontos e as CPUs para manter consistencia interna.
    void modificar_estado_tarefa(int id, Estado novo_estado);
};
