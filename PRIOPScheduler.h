#pragma once
#include "Scheduler.h"

// Classe PRIOPScheduler — implementa o algoritmo Preemptivo por Prioridades (Requisito 4).
// A CPU sempre executa a tarefa de maior prioridade estatica dentre as prontas.
// Preempcao ocorre em dois casos: (1) o quantum da tarefa expira; (2) chega uma tarefa
// com prioridade estatica maior que a da tarefa em execucao.
// Criterios de desempate (Requisito 4.4 e 4.3), aplicados nessa ordem:
//   1. Maior prioridade estatica
//   2. Tarefa que ja estava executando (evita troca de contexto desnecessaria)
//   3. Menor instante de ingresso
//   4. Menor duracao total
//   5. Sorteio
class PRIOPScheduler : public Scheduler {
public:
    explicit PRIOPScheduler(int quantum);
    virtual void escalonar(std::vector<Task*>& prontos, std::vector<CPU>& cpus, int relogio_global) override;
};
