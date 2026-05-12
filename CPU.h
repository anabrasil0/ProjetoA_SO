#pragma once
#include "Task.h"

// Classe CPU — representa um núcleo de processamento do sistema simulado
class CPU {
private:
    Task* tarefa_atual; // ponteiro para a tarefa em execução (nullptr = ociosa)
    int   tempo_ocioso; // contador de ticks que a CPU ficou sem uso

public:
    CPU();

    void   executar_tick();             // executa um tick na tarefa atual
    bool   esta_ocupada()      const;   // retorna true se há tarefa em execução
    Task*  get_tarefa_atual()  const;   // retorna ponteiro para a tarefa atual
    void   set_tarefa_atual(Task* nova_tarefa);
    int    get_tempo_ocioso()  const;
};
