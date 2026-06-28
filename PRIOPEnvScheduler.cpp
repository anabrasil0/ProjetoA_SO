#include "PRIOPEnvScheduler.h"
#include "Task.h"
#include "CPU.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>

// ============================================================
// Construtor
// Repassa o quantum para Scheduler e armazena o alpha localmente.
// ============================================================
PRIOPEnvScheduler::PRIOPEnvScheduler(int quantum, int alpha)
    : Scheduler(quantum), alpha(alpha) {}

// ============================================================
// escalonar — PUBLICO (Projeto B Req. 1)
// Logica identica ao PRIOPScheduler, mas com duas diferencas-chave:
//
//   (0) ANTES do loop de CPUs: incrementa a idade de todas as tarefas
//       que estao atualmente na fila de prontos. Isso simula o tick de
//       espera — a tarefa fica mais "velha" (prioritaria) a cada ciclo.
//
//   Dentro do loop de CPUs:
//   (a) Quantum expirado: tarefa volta para prontos com idade zerada —
//       ela estava rodando, logo nao acumulou espera durante a execucao.
//   (b) CPU desligada: religa se ha trabalho pendente.
//   (c) CPU ociosa: atribui a tarefa de maior prioridade EFETIVA e zera
//       sua idade (comecar a executar "rejuvenesce" a tarefa).
//   (d) CPU ocupada: preempta se candidata tem prioridade EFETIVA maior
//       do que a prioridade ESTATICA da tarefa em execucao. A comparacao
//       usa efetiva vs. estatica porque a tarefa em execucao nao envelhece.
// ============================================================
void PRIOPEnvScheduler::escalonar(std::vector<Task*>& prontos,
                                   std::vector<CPU>& cpus,
                                   int relogio_global) {

    // Descarta sorteios do tick anterior antes de registrar os novos
    limpar_sorteios();

    // --------------------------------------------------------
    // (0) Envelhecimento: incrementa a idade de todas as tarefas
    //     que estao esperando na fila de prontos neste tick.
    //     Feito uma unica vez, ANTES do loop de CPUs, para que o
    //     incremento reflita exatamente um tick de espera.
    //     Tarefas EXECUTANDO nao estao em `prontos`, entao nao envelhecem.
    // --------------------------------------------------------
    for (Task* t : prontos) {
        t->incrementar_idade(alpha);
    }

    for (CPU& cpu : cpus) {

        // --------------------------------------------------------
        // (a) Preempcao por quantum
        // Se a tarefa atual esgotou o tempo maximo consecutivo, ela
        // volta para a fila de prontos. A idade e zerada porque a
        // tarefa acabou de executar — ela "rejuvenesce" ao sair da CPU.
        // --------------------------------------------------------
        if (cpu.esta_ligada() && cpu.esta_ocupada() && cpu.get_ticks_no_quantum() >= quantum) {
            Task* expirada = cpu.get_tarefa_atual();
            expirada->set_estado(PRONTA);
            expirada->resetar_idade(); // acabou de executar: volta sem credito de espera
            prontos.push_back(expirada);
            cpu.set_tarefa_atual(nullptr);
        }

        // --------------------------------------------------------
        // (b) CPU desligada
        // Se ha tarefas prontas, religa para que (c) possa atribuir.
        // --------------------------------------------------------
        if (!cpu.esta_ligada()) {
            if (!prontos.empty()) {
                cpu.ligar();
            } else {
                continue;
            }
        }

        Task* tarefa_atual = cpu.get_tarefa_atual();

        // --------------------------------------------------------
        // Comparador priopenv_melhor
        // Seleciona a tarefa de maior prioridade EFETIVA.
        // Empates sao resolvidos pelos criterios do Projeto B Req. 1.3.
        //
        // prioridade_efetiva(t) = t->get_prioridade() + t->get_idade()
        // (alpha ja foi aplicado dentro de incrementar_idade)
        //
        // Criterios de desempate (nessa ordem):
        //   1. Maior prioridade estatica (distingue quando a efetiva empata)
        //   2. Tarefa ja executando (evita troca de contexto desnecessaria)
        //   3. Menor instante de ingresso
        //   4. Menor duracao total
        //   5. Sorteio
        // --------------------------------------------------------
        auto prioridade_efetiva = [](Task* t) {
            return t->get_prioridade() + t->get_idade();
        };

        auto priopenv_melhor = [this, tarefa_atual, &prioridade_efetiva](Task* a, Task* b) -> bool {

            int pef_a = prioridade_efetiva(a);
            int pef_b = prioridade_efetiva(b);

            // Criterio principal: maior prioridade efetiva (prioridade + envelhecimento)
            if (pef_a != pef_b)
                return pef_a > pef_b;

            // Criterio 1 de desempate: maior prioridade ESTATICA (Projeto B Req. 1.3)
            if (a->get_prioridade() != b->get_prioridade())
                return a->get_prioridade() > b->get_prioridade();

            // Criterio 2: evita troca de contexto desnecessaria
            if (tarefa_atual != nullptr) {
                if (a == tarefa_atual) return true;
                if (b == tarefa_atual) return false;
            }

            // Criterio 3: menor instante de ingresso
            if (a->get_tempo_ingresso() != b->get_tempo_ingresso())
                return a->get_tempo_ingresso() < b->get_tempo_ingresso();

            // Criterio 4: menor duracao total
            if (a->get_duracao() != b->get_duracao())
                return a->get_duracao() < b->get_duracao();

            // Criterio 5: sorteio — registra vencedor para o Gantt
            bool sorteio = (rand() % 2 == 0);
            int vencedor_id = sorteio ? a->get_id() : b->get_id();
            sorteio_ids.insert(vencedor_id);
            std::cout << "[SORTEIO] Entre T" << a->get_id()
                      << " e T" << b->get_id()
                      << " -> venceu T" << vencedor_id << std::endl;
            return sorteio;
        };

        // --------------------------------------------------------
        // (c) CPU ociosa: atribui a tarefa de maior prioridade efetiva.
        //     Ao iniciar a execucao, a idade e zerada — a tarefa foi
        //     escolhida, entao o envelhecimento cumpriu seu papel.
        // --------------------------------------------------------
        if (!cpu.esta_ocupada()) {

            if (!prontos.empty()) {
                auto it = std::min_element(prontos.begin(), prontos.end(), priopenv_melhor);
                Task* escolhida = *it;
                prontos.erase(it);
                escolhida->set_estado(EXECUTANDO);
                escolhida->resetar_idade(); // inicia execucao: zera o credito de espera
                cpu.set_tarefa_atual(escolhida);
            } else {
                cpu.desligar();
            }
        }

        // --------------------------------------------------------
        // (d) CPU ocupada: preempcao por envelhecimento.
        //     Compara a prioridade EFETIVA da melhor candidata da fila
        //     com a prioridade ESTATICA da tarefa em execucao.
        //     Usamos a estatica da tarefa em execucao porque ela nao
        //     esta envelhecendo (nao esta em prontos).
        //     So preempta se a efetiva da candidata for ESTRITAMENTE
        //     maior — empate mantem a tarefa atual.
        // --------------------------------------------------------
        else {

            if (!prontos.empty()) {
                auto it = std::min_element(prontos.begin(), prontos.end(), priopenv_melhor);
                Task* candidata = *it;

                int efetiva_candidata  = prioridade_efetiva(candidata);
                int estatica_executando = tarefa_atual->get_prioridade();

                if (efetiva_candidata > estatica_executando) {
                    prontos.erase(it);

                    // Devolve a tarefa atual para a fila de prontos.
                    // A idade ja era 0 (foi zerada quando ela comeou a executar),
                    // entao ela entra novamente na fila sem credito de espera acumulado
                    // durante a execucao — justo, pois ela acabou de rodar.
                    tarefa_atual->set_estado(PRONTA);
                    tarefa_atual->resetar_idade();
                    prontos.push_back(tarefa_atual);

                    candidata->set_estado(EXECUTANDO);
                    candidata->resetar_idade(); // candidata inicia execucao: zera espera
                    cpu.set_tarefa_atual(candidata);
                }
            }
        }
    }
}
