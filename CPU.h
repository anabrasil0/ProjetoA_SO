#pragma once
#include "Task.h"

// Classe CPU — representa um núcleo de processamento do sistema simulado
class CPU {
private:
    Task* tarefa_atual;      // ponteiro para a tarefa em execução (nullptr = ociosa)
    int   tempo_ocioso;      // contador de ticks que a CPU ficou sem uso
    int   ticks_no_quantum;  // ticks consecutivos da tarefa atual; reset ao trocar de tarefa

public:
    CPU();

    // Chamado pelo Simulador a cada tick do relogio global para que a CPU
    // atualize seu proprio estado: decrementa o tempo restante da tarefa,
    // detecta finalizacao e contabiliza tempo ocioso e ticks do quantum.
    // Nao dispara o tick — apenas responde ao tick ja disparado pelo Simulador.
    void   processar_ciclo();
    bool   esta_ocupada()      const;   // retorna true se há tarefa em execução
    Task*  get_tarefa_atual()  const;   // retorna ponteiro para a tarefa atual
    void   set_tarefa_atual(Task* nova_tarefa);
    int    get_tempo_ocioso()  const;
    int    get_ticks_no_quantum() const; // ticks consecutivos da tarefa atual (para controle de quantum)
};
