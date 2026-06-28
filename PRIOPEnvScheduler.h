#pragma once
#include "Scheduler.h"

// Classe PRIOPEnvScheduler — implementa o escalonador Preemptivo por Prioridades
// com Envelhecimento (PRIOPEnv) exigido no Projeto B, Requisito 1.
//
// Motivacao: o escalonador PRIOP simples pode causar inanicao — tarefas de baixa
// prioridade nunca sao escolhidas se chegam novas tarefas de alta prioridade.
// O envelhecimento resolve isso: a cada tick que uma tarefa espera na fila de prontos
// sem executar, sua prioridade EFETIVA aumenta em `alpha`. Assim, mesmo uma tarefa
// de prioridade 1 acaba superando uma de prioridade 5 depois de tempo suficiente.
//
// Criterio de selecao: prioridade_efetiva = prioridade_estatica + (idade * alpha),
// onde `idade` e o numero de ticks acumulados na fila sem executar.
//
// Preempcao ocorre quando: (1) quantum expira; (2) chega candidata com
// prioridade_efetiva maior que a prioridade_estatica da tarefa em execucao.
// A tarefa em execucao nao envelhece (nao esta na fila de prontos).
//
// Criterios de desempate (Projeto B Req. 1.3), em ordem:
//   1. Maior prioridade estatica (nao efetiva) — distingue tarefas com mesma efetiva
//   2. Tarefa que ja estava executando (evita troca de contexto desnecessaria)
//   3. Menor instante de ingresso (quem chegou antes)
//   4. Menor duracao total
//   5. Sorteio
class PRIOPEnvScheduler : public Scheduler {
private:
    int alpha; // incremento de prioridade efetiva por tick de espera

public:
    PRIOPEnvScheduler(int quantum, int alpha);
    virtual void escalonar(std::vector<Task*>& prontos, std::vector<CPU>& cpus, int relogio_global) override;
};
