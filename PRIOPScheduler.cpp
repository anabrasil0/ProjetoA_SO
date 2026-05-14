#include "PRIOPScheduler.h"
#include "Task.h"
#include "CPU.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>

// ============================================================
// Construtor
// Repassa o quantum para a classe base Scheduler, que o armazena
// no atributo protegido `quantum` acessivel por esta subclasse.
// ============================================================
PRIOPScheduler::PRIOPScheduler(int quantum) : Scheduler(quantum) {}

// ============================================================
// priop_melhor — comparador local (auxiliar de escalonar)
// Retorna true se a tarefa 'a' deve ser preferida em relacao a 'b'.
// Usado como predicado de std::min_element para encontrar a melhor
// candidata na fila de prontos.
//
// O parametro 'atual' e a tarefa que ja esta executando na CPU;
// ele permite aplicar o criterio de evitar troca desnecessaria quando
// duas tarefas empatam em prioridade.
// ============================================================
static bool priop_melhor(Task* a, Task* b, Task* atual) {

    // Criterio 1: maior prioridade estatica — regra fundamental do PRIOP (Requisito 4.4).
    // Valor numerico maior significa prioridade mais alta no sistema.
    if (a->get_prioridade() != b->get_prioridade())
        return a->get_prioridade() > b->get_prioridade(); // maior vence

    // Criterio 2 (empate de prioridade): evita troca de contexto desnecessaria.
    // Se 'a' ou 'b' ja esta executando, mante-la e mais eficiente do que trocar.
    if (atual != nullptr) {
        if (a == atual) return true;  // 'a' ja esta rodando; prefere mante-la
        if (b == atual) return false; // 'b' ja esta rodando; prefere mante-la
    }

    // Criterio 3 (ainda empatado): quem chegou primeiro ao sistema tem prioridade.
    // Favorece tarefas mais antigas, reduzindo o risco de starvation.
    if (a->get_tempo_ingresso() != b->get_tempo_ingresso())
        return a->get_tempo_ingresso() < b->get_tempo_ingresso();

    // Criterio 4 (ainda empatado): menor duracao total original.
    // Entre tarefas identicas em todos os outros aspectos, prefere a mais curta.
    if (a->get_duracao() != b->get_duracao())
        return a->get_duracao() < b->get_duracao();

    // Criterio 5 (desempate final): sorteio aleatorio.
    // Garante que o algoritmo sempre tome uma decisao mesmo no pior caso.
    bool sorteio = (rand() % 2 == 0); // 50% de chance para cada tarefa
    std::cout << "[SORTEIO] Entre T" << a->get_id()
              << " e T" << b->get_id()
              << " -> venceu T" << (sorteio ? a->get_id() : b->get_id()) << std::endl;
    return sorteio;
}

// ============================================================
// escalonar — PUBLICO (Requisito 4)
// Percorre todas as CPUs e decide qual tarefa executa em cada uma.
// Para cada CPU, quatro situacoes sao tratadas nessa ordem:
//
//   (a) Quantum expirou: a tarefa atual volta para a fila de prontos
//       e a CPU e liberada antes de qualquer outra decisao.
//   (b) CPU desligada: se ha trabalho, religa; senao mantém desligada.
//   (c) CPU ociosa: atribui a melhor tarefa ou desliga se fila vazia.
//   (d) CPU ocupada: preempta se houver tarefa com prioridade maior.
//
// Toda a logica de preempcao e desligamento fica aqui, no scheduler,
// mantendo o Simulador livre de regras de politica.
// ============================================================
void PRIOPScheduler::escalonar(std::vector<Task*>& prontos,
                               std::vector<CPU>& cpus,
                               int relogio_global) {

    for (CPU& cpu : cpus) {

        // --------------------------------------------------------
        // (a) Preempcao por quantum
        // Verifica se a tarefa atual esgotou seu tempo maximo consecutivo.
        // So pode acontecer em CPUs ligadas E ocupadas.
        // A tarefa volta para a fila de prontos (estado PRONTA) e a CPU
        // e liberada (tarefa_atual = nullptr) para receber uma nova tarefa.
        // --------------------------------------------------------
        if (cpu.esta_ligada() && cpu.esta_ocupada() && cpu.get_ticks_no_quantum() >= quantum) {
            Task* expirada = cpu.get_tarefa_atual(); // tarefa que esgotou o quantum
            expirada->set_estado(PRONTA);            // volta para a fila de prontos
            prontos.push_back(expirada);             // reinsere na fila para ser reescalonada
            cpu.set_tarefa_atual(nullptr);           // libera a CPU (quantum sera resetado em set_tarefa_atual)
        }

        // --------------------------------------------------------
        // (b) CPU desligada
        // Se ha tarefas prontas, religa a CPU para que o bloco (c)
        // possa atribuir uma tarefa a ela ainda neste mesmo tick.
        // Se a fila continua vazia, pula para a proxima CPU.
        // --------------------------------------------------------
        if (!cpu.esta_ligada()) {
            if (!prontos.empty()) {
                cpu.ligar(); // reativa a CPU; cai naturalmente no bloco (c) abaixo
            } else {
                continue; // fila vazia: permanece desligada, passa para a proxima CPU
            }
        }

        Task* tarefa_atual = cpu.get_tarefa_atual(); // tarefa em execucao agora (ou nullptr)

        // --------------------------------------------------------
        // (c) CPU ociosa (ligada, sem tarefa)
        // Busca a tarefa de maior prioridade na fila de prontos usando priop_melhor.
        // Se a fila estiver vazia, desliga a CPU para economizar recursos.
        // --------------------------------------------------------
        if (!cpu.esta_ocupada()) {

            if (!prontos.empty()) {
                // Encontra a tarefa de maior prioridade na fila de prontos
                auto it = std::min_element(
                    prontos.begin(), prontos.end(),
                    [tarefa_atual](Task* a, Task* b) {
                        return priop_melhor(a, b, tarefa_atual);
                    }
                );

                Task* escolhida = *it;            // tarefa selecionada para executar
                prontos.erase(it);                // remove da fila de prontos
                escolhida->set_estado(EXECUTANDO);// atualiza estado no TCB
                cpu.set_tarefa_atual(escolhida);  // atribui a CPU (reseta o quantum)
            } else {
                // Fila vazia e CPU sem tarefa: desliga para nao ficar ociosa
                cpu.desligar();
            }
        }

        // --------------------------------------------------------
        // (d) CPU ocupada — preempcao por prioridade (Requisito 4.4)
        // Compara a melhor candidata da fila com a tarefa em execucao.
        // A troca so ocorre se a candidata tiver prioridade ESTRITAMENTE
        // maior — empate mantém a tarefa atual (evita troca de contexto
        // desnecessaria, criterio 2 de priop_melhor).
        // --------------------------------------------------------
        else {

            if (!prontos.empty()) {
                // Encontra a tarefa de maior prioridade na fila de prontos
                auto it = std::min_element(
                    prontos.begin(), prontos.end(),
                    [tarefa_atual](Task* a, Task* b) {
                        return priop_melhor(a, b, tarefa_atual);
                    }
                );

                Task* candidata = *it;

                // So preempta se a candidata tiver prioridade genuinamente maior
                if (candidata->get_prioridade() > tarefa_atual->get_prioridade()) {
                    prontos.erase(it); // retira a candidata da fila

                    // Devolve a tarefa atual para a fila de prontos
                    tarefa_atual->set_estado(PRONTA);
                    prontos.push_back(tarefa_atual);

                    // Atribui a candidata a CPU (quantum resetado automaticamente)
                    candidata->set_estado(EXECUTANDO);
                    cpu.set_tarefa_atual(candidata);
                }
                // Se candidata.prioridade <= atual.prioridade: sem preempcao,
                // a tarefa atual continua rodando normalmente.
            }
        }
    }
}
