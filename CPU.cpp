#include "CPU.h"

CPU::CPU() : tarefa_atual(nullptr), tempo_ocioso(0) {}

// Avança um tick na tarefa atual; se ela terminar, libera a CPU
void CPU::executar_tick() {
    if (tarefa_atual != nullptr) {
        tarefa_atual->set_estado(EXECUTANDO);
        tarefa_atual->set_tempo_restante(tarefa_atual->get_tempo_restante() - 1);

        if (tarefa_atual->get_tempo_restante() <= 0) {
            tarefa_atual->set_estado(FINALIZADA);
            tarefa_atual = nullptr;
        }
    } 
    else {
        tempo_ocioso++;
    }
}

bool CPU::esta_ocupada() const {
    return tarefa_atual != nullptr;
}

Task* CPU::get_tarefa_atual() const {
    return tarefa_atual;
}

void CPU::set_tarefa_atual(Task* nova_tarefa) {
    tarefa_atual = nova_tarefa;
}

int CPU::get_tempo_ocioso() const {
    return tempo_ocioso;
}
