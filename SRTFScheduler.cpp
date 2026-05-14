#include "SRTFScheduler.h"
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
SRTFScheduler::SRTFScheduler(int quantum) : Scheduler(quantum) {}

// ============================================================
// escalonar — PUBLICO (Requisito 4)
// Percorre todas as CPUs e decide qual tarefa executa em cada uma.
// Para cada CPU, quatro situacoes sao tratadas nessa ordem:
//
//   (a) Quantum expirou: a tarefa atual volta para a fila de prontos
//       e a CPU e liberada antes de qualquer outra decisao.
//   (b) CPU desligada: se ha trabalho, religa; senao mantém desligada.
//   (c) CPU ociosa: atribui a melhor tarefa ou desliga se fila vazia.
//   (d) CPU ocupada: preempta se houver tarefa com tempo restante menor.
//
// O comparador srtf_melhor e definido como lambda dentro deste metodo
// para que possa registrar sorteios diretamente em sorteio_ids (membro
// herdado de Scheduler), sem precisar de variaveis globais ou estaticas.
// ============================================================
void SRTFScheduler::escalonar(std::vector<Task*>& prontos,
                               std::vector<CPU>& cpus,
                               int relogio_global) {

    // Descarta sorteios do tick anterior antes de registrar os novos
    limpar_sorteios();

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
        // Comparador SRTF com registro de sorteios (Requisito 4.3)
        // Definido como lambda para capturar tarefa_atual (por valor,
        // pois muda a cada CPU) e sorteio_ids (via this, por referencia).
        //
        // Criterios em ordem:
        //   1. Menor tempo restante (regra fundamental do SRTF)
        //   2. Tarefa ja executando (evita troca de contexto desnecessaria)
        //   3. Menor instante de ingresso (quem chegou antes)
        //   4. Menor duracao total
        //   5. Sorteio — registrado em sorteio_ids para aparecer no Gantt
        // --------------------------------------------------------
        auto srtf_melhor = [this, tarefa_atual](Task* a, Task* b) -> bool {

            // Criterio 1: menor tempo restante
            if (a->get_tempo_restante() != b->get_tempo_restante())
                return a->get_tempo_restante() < b->get_tempo_restante();

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

            // Criterio 5: sorteio aleatorio — registra o vencedor para o Gantt
            bool sorteio = (rand() % 2 == 0);
            int vencedor_id = sorteio ? a->get_id() : b->get_id();
            sorteio_ids.insert(vencedor_id); // std::set evita duplicatas
            std::cout << "[SORTEIO] Entre T" << a->get_id()
                      << " e T" << b->get_id()
                      << " -> venceu T" << vencedor_id << std::endl;
            return sorteio;
        };

        // --------------------------------------------------------
        // (c) CPU ociosa (ligada, sem tarefa)
        // Busca a tarefa com menor tempo restante usando srtf_melhor.
        // Se a fila estiver vazia, desliga a CPU para economizar recursos.
        // --------------------------------------------------------
        if (!cpu.esta_ocupada()) {

            if (!prontos.empty()) {
                // Encontra a tarefa com menor tempo restante na fila de prontos
                auto it = std::min_element(prontos.begin(), prontos.end(), srtf_melhor);

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
        // (d) CPU ocupada — preempcao por SRTF
        // Compara a melhor candidata da fila com a tarefa em execucao.
        // A troca so ocorre se a candidata tiver tempo restante
        // ESTRITAMENTE menor — empate mantém a tarefa atual.
        // --------------------------------------------------------
        else {

            if (!prontos.empty()) {
                // Encontra a melhor candidata na fila de prontos
                auto it = std::min_element(prontos.begin(), prontos.end(), srtf_melhor);
                Task* candidata = *it;

                // So preempta se a candidata for genuinamente melhor
                if (candidata->get_tempo_restante() < tarefa_atual->get_tempo_restante()) {
                    prontos.erase(it); // retira a candidata da fila

                    // Devolve a tarefa atual para a fila de prontos
                    tarefa_atual->set_estado(PRONTA);
                    prontos.push_back(tarefa_atual);

                    // Atribui a candidata a CPU (quantum resetado automaticamente)
                    candidata->set_estado(EXECUTANDO);
                    cpu.set_tarefa_atual(candidata);
                }
            }
        }
    }
}
