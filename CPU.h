#pragma once
#include "Task.h"

// Classe CPU — representa um nucleo de processamento do sistema simulado.
// Uma CPU pode estar em tres situacoes: desligada, ociosa (ligada sem tarefa)
// ou ocupada (ligada executando uma tarefa).
// O desligamento ocorre quando nao ha tarefas na fila de prontos (Requisito 1.2);
// a CPU e religada pelo scheduler assim que uma nova tarefa se torna disponivel.
class CPU {
private:
    Task* tarefa_atual;      // tarefa em execucao (nullptr = ociosa ou desligada)
    bool  ligada;            // false = desligada por ausencia de trabalho (Requisito 1.2)
    int   tempo_ocioso;      // ticks ligada sem tarefa (util para estatisticas)
    int   tempo_desligada;   // ticks total que a CPU ficou desligada
    int   ticks_no_quantum;  // ticks consecutivos da tarefa atual; reset ao trocar de tarefa

public:
    CPU();

    // Chamado pelo Simulador a cada tick do relogio global para que a CPU
    // atualize seu proprio estado: decrementa tempo restante da tarefa,
    // detecta finalizacao, contabiliza tempo ocioso/desligado e ticks do quantum.
    // Nao dispara o tick — apenas responde ao tick ja disparado pelo Simulador.
    void  processar_ciclo();

    bool  esta_ocupada()         const; // true se ha tarefa em execucao
    bool  esta_ligada()          const; // true se a CPU nao esta desligada
    Task* get_tarefa_atual()     const;
    int   get_tempo_ocioso()     const;
    int   get_tempo_desligada()  const;
    int   get_ticks_no_quantum() const;

    void  set_tarefa_atual(Task* nova_tarefa);
    void  ligar();    // liga a CPU (chamado pelo scheduler ao encontrar trabalho)
    void  desligar(); // desliga a CPU (chamado pelo scheduler quando fila vazia)
};
