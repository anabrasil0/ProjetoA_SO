#include "SRTFScheduler.h"
#include "Task.h"
#include "CPU.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>

// ============================================================
// Construtor
// ============================================================
SRTFScheduler::SRTFScheduler(int quantum) : Scheduler(quantum) {}

// ============================================================
// srtf_melhor — comparador local (auxiliar de escalonar)
// Retorna true se a tarefa 'a' deve ser preferida em relacao a 'b'.
// Usado pelo std::min_element para encontrar a melhor candidata.
// O parametro 'atual' e a tarefa que esta executando na CPU neste momento;
// ele e necessario para aplicar o criterio de evitar troca desnecessaria.
// ============================================================
static bool srtf_melhor(Task* a, Task* b, Task* atual) {

    // 1. Menor tempo restante — criterio principal do SRTF
    if (a->get_tempo_restante() != b->get_tempo_restante())
        return a->get_tempo_restante() < b->get_tempo_restante();

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
// Para cada CPU, quatro situacoes sao tratadas nessa ordem:
//   (a) Quantum expirou: tarefa volta para prontos antes de qualquer decisao.
//   (b) CPU desligada: religar se houver trabalho; senao manter desligada.
//   (c) CPU ociosa: atribuir tarefa ou desligar se fila vazia (Requisito 1.2).
//   (d) CPU ocupada: preemptar se candidata tiver tempo restante menor.
// Toda a logica de preempcao e desligamento fica aqui, no scheduler.
// ============================================================
void SRTFScheduler::escalonar(std::vector<Task*>& prontos,
                               std::vector<CPU>& cpus,
                               int relogio_global) {

    for (CPU& cpu : cpus) {

        // (a) Preempcao por quantum
        // Apenas CPUs ligadas e ocupadas podem ter quantum esgotado.
        if (cpu.esta_ligada() && cpu.esta_ocupada() && cpu.get_ticks_no_quantum() >= quantum) {
            Task* expirada = cpu.get_tarefa_atual();
            expirada->set_estado(PRONTA);
            prontos.push_back(expirada);
            cpu.set_tarefa_atual(nullptr);
        }

        // (b) CPU desligada: religar se houver trabalho disponivel
        if (!cpu.esta_ligada()) {
            if (!prontos.empty()) {
                cpu.ligar();
                // cai no bloco (c) para receber a tarefa
            } else {
                continue; // sem tarefas, permanece desligada
            }
        }

        Task* tarefa_atual = cpu.get_tarefa_atual();

        // (c) CPU ociosa: atribui a melhor tarefa ou desliga (Requisito 1.2)
        if (!cpu.esta_ocupada()) {

            if (!prontos.empty()) {
                auto it = std::min_element(
                    prontos.begin(), prontos.end(),
                    [tarefa_atual](Task* a, Task* b) {
                        return srtf_melhor(a, b, tarefa_atual);
                    }
                );

                Task* escolhida = *it;
                prontos.erase(it);
                escolhida->set_estado(EXECUTANDO);
                cpu.set_tarefa_atual(escolhida);
            } else {
                // Fila vazia e CPU sem tarefa: desligar para nao ficar ociosa
                cpu.desligar();
            }
        }

        // (d) CPU ocupada: preempcao por SRTF
        // So troca se a candidata tiver tempo restante estritamente menor.
        else {

            if (!prontos.empty()) {
                auto it = std::min_element(
                    prontos.begin(), prontos.end(),
                    [tarefa_atual](Task* a, Task* b) {
                        return srtf_melhor(a, b, tarefa_atual);
                    }
                );

                Task* candidata = *it;

                if (candidata->get_tempo_restante() < tarefa_atual->get_tempo_restante()) {
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
