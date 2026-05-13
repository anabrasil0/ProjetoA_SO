#include "PRIOPScheduler.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>

// ============================================================
// Construtor
// ============================================================
PRIOPScheduler::PRIOPScheduler(int quantum) : Scheduler(quantum) {}

// ============================================================
// priop_melhor — comparador local (auxiliar de escalonar)
// Retorna true se a tarefa 'a' deve ser preferida em relacao a 'b'.
// Usado pelo std::min_element para encontrar a melhor candidata.
// O parametro 'atual' e a tarefa que esta executando na CPU neste momento;
// ele e necessario para aplicar o criterio de evitar troca desnecessaria.
// ============================================================
static bool priop_melhor(Task* a, Task* b, Task* atual) {

    // 1. Maior prioridade estatica — criterio principal do PRIOP (Requisito 4.4)
    if (a->get_prioridade() != b->get_prioridade())
        return a->get_prioridade() > b->get_prioridade();

    // 2. Evita troca de contexto desnecessaria (Requisito 4.3)
    if (atual != nullptr) {
        if (a == atual) return true;
        if (b == atual) return false;
    }

    // 3. Quem chegou antes (Requisito 4.3)
    if (a->get_tempo_ingresso() != b->get_tempo_ingresso())
        return a->get_tempo_ingresso() < b->get_tempo_ingresso();

    // 4. Menor duracao total (Requisito 4.3)
    if (a->get_duracao() != b->get_duracao())
        return a->get_duracao() < b->get_duracao();

    // 5. Sorteio — sera registrado como evento grafico no Gantt (Requisito 4.3)
    bool sorteio = (rand() % 2 == 0);
    std::cout << "[SORTEIO] Entre T" << a->get_id()
              << " e T" << b->get_id()
              << " -> venceu T" << (sorteio ? a->get_id() : b->get_id()) << std::endl;
    return sorteio;
}

// ============================================================
// escalonar — PUBLICO (Requisito 4)
// Percorre todas as CPUs e decide qual tarefa executa em cada uma.
// Para cada CPU, tres situacoes sao tratadas nessa ordem:
//   (a) Quantum expirou: tarefa atual volta para prontos antes de tudo.
//   (b) CPU ociosa: escolhe a tarefa com maior prioridade estatica.
//   (c) CPU ocupada: preempta se existir candidata com prioridade maior.
// Toda a logica de preempcao fica aqui, sem nenhuma dependencia do Simulador.
// ============================================================
void PRIOPScheduler::escalonar(std::vector<Task*>& prontos,
                               std::vector<CPU>& cpus,
                               int relogio_global) {

    for (CPU& cpu : cpus) {

        // (a) Preempcao por quantum
        // Se a tarefa atual ja rodou 'quantum' ticks consecutivos, ela e
        // recolocada na fila de prontos e a CPU fica livre para nova escolha.
        if (cpu.esta_ocupada() && cpu.get_ticks_no_quantum() >= quantum) {
            Task* expirada = cpu.get_tarefa_atual();
            expirada->set_estado(PRONTA);
            prontos.push_back(expirada);
            cpu.set_tarefa_atual(nullptr);
        }

        Task* tarefa_atual = cpu.get_tarefa_atual();

        // (b) CPU ociosa: atribui a tarefa de maior prioridade
        if (!cpu.esta_ocupada()) {

            if (!prontos.empty()) {
                auto it = std::min_element(
                    prontos.begin(), prontos.end(),
                    [tarefa_atual](Task* a, Task* b) {
                        return priop_melhor(a, b, tarefa_atual);
                    }
                );

                Task* escolhida = *it;
                prontos.erase(it);
                escolhida->set_estado(EXECUTANDO);
                cpu.set_tarefa_atual(escolhida);
            }
        }

        // (c) CPU ocupada: preempcao por prioridade
        // So troca se a candidata tiver prioridade estritamente maior que a atual.
        else {

            if (!prontos.empty()) {
                auto it = std::min_element(
                    prontos.begin(), prontos.end(),
                    [tarefa_atual](Task* a, Task* b) {
                        return priop_melhor(a, b, tarefa_atual);
                    }
                );

                Task* candidata = *it;

                if (candidata->get_prioridade() > tarefa_atual->get_prioridade()) {
                    prontos.erase(it);

                    tarefa_atual->set_estado(PRONTA);
                    prontos.push_back(tarefa_atual);

                    candidata->set_estado(EXECUTANDO);
                    cpu.set_tarefa_atual(candidata);
                }
            }
        }
    }
}
