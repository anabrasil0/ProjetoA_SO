#include "SRTFScheduler.h"

#include <algorithm>

SRTFScheduler::SRTFScheduler() {

}

void SRTFScheduler::escalonar(std::vector<Task*>& prontos,
    std::vector<CPU>& cpus,
    int relogio_global) {

    for (CPU& cpu : cpus) {

        Task* tarefa_atual = cpu.get_tarefa_atual();

        // =========================================================
        // CPU OCIOSA
        // =========================================================
        if (!cpu.esta_ocupada()) {

            if (!prontos.empty()) {

                auto it = std::min_element(
                    prontos.begin(),
                    prontos.end(),

                    [tarefa_atual](Task* a, Task* b) {

                        // 1. Menor tempo restante
                        if (a->get_tempo_restante() != b->get_tempo_restante()) {
                            return a->get_tempo_restante() <
                                b->get_tempo_restante();
                        }

                        // 2. Evita troca de contexto desnecessária
                        if (tarefa_atual != nullptr) {

                            if (a == tarefa_atual) return true;
                            if (b == tarefa_atual) return false;
                        }

                        // 3. Quem chegou antes
                        if (a->get_tempo_ingresso() != b->get_tempo_ingresso()) {
                            return a->get_tempo_ingresso() <
                                b->get_tempo_ingresso();
                        }

                        // 4. Menor duração total
                        if (a->get_duracao() != b->get_duracao()) {
                            return a->get_duracao() <
                                b->get_duracao();
                        }

                        // 5. Desempate final temporário
                        return a->get_id() < b->get_id();
                    }
                );

                Task* nova_tarefa = *it;

                // remove da fila
                prontos.erase(it);

                // atualiza estado
                nova_tarefa->set_estado(EXECUTANDO);

                // envia para CPU
                cpu.set_tarefa_atual(nova_tarefa);
            }
        }

        // =========================================================
        // CPU OCUPADA (PREEMPÇÃO)
        // =========================================================
        else {

            if (!prontos.empty()) {

                auto it = std::min_element(
                    prontos.begin(),
                    prontos.end(),

                    [tarefa_atual](Task* a, Task* b) {

                        // 1. Menor tempo restante
                        if (a->get_tempo_restante() != b->get_tempo_restante()) {
                            return a->get_tempo_restante() <
                                b->get_tempo_restante();
                        }

                        // 2. Evita troca de contexto desnecessária
                        if (tarefa_atual != nullptr) {

                            if (a == tarefa_atual) return true;
                            if (b == tarefa_atual) return false;
                        }

                        // 3. Quem chegou antes
                        if (a->get_tempo_ingresso() != b->get_tempo_ingresso()) {
                            return a->get_tempo_ingresso() <
                                b->get_tempo_ingresso();
                        }

                        // 4. Menor duração total
                        if (a->get_duracao() != b->get_duracao()) {
                            return a->get_duracao() <
                                b->get_duracao();
                        }

                        // 5. Desempate final temporário
                        return a->get_id() < b->get_id();
                    }
                );

                Task* melhor_tarefa = *it;

                // Só faz preempção se a nova realmente for melhor
                if (melhor_tarefa->get_tempo_restante() <
                    tarefa_atual->get_tempo_restante()) {

                    // remove nova tarefa da fila primeiro
                    prontos.erase(it);

                    // tarefa antiga volta para pronta
                    tarefa_atual->set_estado(PRONTA);
                    prontos.push_back(tarefa_atual);

                    // nova tarefa assume CPU
                    melhor_tarefa->set_estado(EXECUTANDO);

                    cpu.set_tarefa_atual(melhor_tarefa);
                }
            }
        }
    }
}