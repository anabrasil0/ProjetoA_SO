#pragma once
#include <vector>
#include <string>

// Estado de cada tarefa durante um tick, para fins de visualizacao no Gantt
enum class TipoGantt {
    NAO_CHEGOU,  // tarefa ainda nao entrou no sistema
    PRONTA,      // na fila de prontos, aguardando CPU
    EXECUTANDO,  // rodando em uma CPU
    SUSPENSA,    // bloqueada (I/O, mutex, etc.) — reservado para Projeto B
    FINALIZADA   // execucao concluida
};

// Entrada de uma tarefa em um tick especifico
struct EntradaGantt {
    int         tarefa_id;
    std::string cor;           // hex RGB da tarefa (ex: "FF0000")
    TipoGantt   tipo;
    int         cpu_id;        // indice da CPU (-1 se nao estiver executando)
    bool        evento_chegada = false;  // tarefa chegou neste tick (Requisito 2.2)
    bool        evento_termino = false;  // tarefa termina neste tick (Requisito 2.2)
    bool        evento_sorteio = false;  // tarefa foi escolhida por sorteio neste tick (Requisito 4.3)
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
