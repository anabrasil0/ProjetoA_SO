#pragma once
#include <vector>
#include <string>

// Estado de cada tarefa durante um tick, para fins de visualizacao no Gantt.
// SUSPENSA_MUTEX e SUSPENSA_IO tem renderizacao distinta (Req. 2.9).
enum class TipoGantt {
    NAO_CHEGOU,     // tarefa ainda nao entrou no sistema
    PRONTA,         // na fila de prontos, aguardando CPU
    EXECUTANDO,     // rodando em uma CPU
    SUSPENSA_MUTEX, // bloqueada aguardando liberacao de mutex (Req. 2)
    SUSPENSA_IO,    // bloqueada aguardando conclusao de E/S (Req. 3)
    FINALIZADA      // execucao concluida
};

// Entrada de uma tarefa em um tick especifico
struct EntradaGantt {
    int         tarefa_id;
    std::string cor;           // hex RGB da tarefa (ex: "FF0000")
    TipoGantt   tipo;
    int         cpu_id;        // indice da CPU (-1 se nao estiver executando)
    bool        evento_chegada      = false; // tarefa chegou neste tick (Req. 2.2)
    bool        evento_termino      = false; // tarefa termina neste tick (Req. 2.2)
    bool        evento_sorteio      = false; // tarefa escolhida por sorteio (Req. 4.3)
    bool        evento_mutex_lock   = false; // tarefa tentou adquirir mutex neste tick (Req. 2.8)
    bool        evento_mutex_unlock = false; // tarefa liberou mutex neste tick (Req. 2.8)
    bool        evento_io           = false; // tarefa iniciou operacao de E/S neste tick (Req. 3)
};

// Estado de uma CPU em um tick especifico (para a linha de CPUs no Gantt)
enum class EstadoCPU { DESLIGADA, OCIOSA, EXECUTANDO };

struct EntradaCPU {
    int         cpu_id;
    EstadoCPU   estado;
    int         tarefa_id  = -1;       // ID da tarefa em execucao (-1 se nao houver)
    std::string cor        = "FFFFFF"; // cor da tarefa em execucao (para colorir a celula)
};

// Snapshot do sistema inteiro em um tick
struct TickGantt {
    int tick;
    std::vector<EntradaGantt> entradas; // uma entrada por tarefa
    std::vector<EntradaCPU>   cpus;     // uma entrada por CPU (Requisito 1.2)
};

// ============================================================
// GanttLog — acumula os snapshots tick a tick
// ============================================================
class GanttLog {
public:
    void registrar(const TickGantt& t);

    // Remove todas as entradas com tick >= valor dado (usado no step_backward)
    void truncar_apos(int tick);

    const std::vector<TickGantt>& get_ticks() const;

private:
    std::vector<TickGantt> ticks;
};
