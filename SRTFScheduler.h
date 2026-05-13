#pragma once
#include "Scheduler.h"

// Classe SRTFScheduler — implementa o algoritmo Shortest Remaining Time First (Requisito 4).
// A CPU sempre executa a tarefa com menor tempo_restante dentre as prontas.
// Preempcao ocorre em dois casos: (1) o quantum da tarefa expira; (2) chega uma tarefa
// com tempo restante menor que o da tarefa em execucao.
// Criterios de desempate (Requisito 4.3), aplicados nessa ordem:
//   1. Tarefa que ja estava executando (evita troca de contexto desnecessaria)
//   2. Menor instante de ingresso
//   3. Menor duracao total
//   4. Sorteio
class SRTFScheduler : public Scheduler {
public:
    explicit SRTFScheduler(int quantum);
    virtual void escalonar(std::vector<Task*>& prontos, std::vector<CPU>& cpus, int relogio_global) override;
};
