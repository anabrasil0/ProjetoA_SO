#include "CPU.h"

CPU::CPU() : tarefa_atual(nullptr), tempo_ocioso(0), ticks_no_quantum(0) {}

// Avança um tick na tarefa atual; se ela terminar, libera a CPU
// ============================================================
// processar_ciclo — PUBLICO
// Responde a um tick do relogio global ja disparado pelo Simulador.
// Esta CPU nao controla o clock — ela apenas avanca seu estado interno
// por um ciclo quando o Simulador a instrui a fazê-lo.
// Se houver tarefa em execucao: decrementa tempo restante, incrementa
// contador de quantum e detecta finalizacao.
// Se estiver ociosa: contabiliza o tempo sem uso.
// ============================================================
void CPU::processar_ciclo() {
    if (tarefa_atual != nullptr) {
        tarefa_atual->set_estado(EXECUTANDO);
        ticks_no_quantum++;
        tarefa_atual->set_tempo_restante(tarefa_atual->get_tempo_restante() - 1);

        if (tarefa_atual->get_tempo_restante() <= 0) {
            tarefa_atual->set_estado(FINALIZADA);
            tarefa_atual   = nullptr;
            ticks_no_quantum = 0;
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
    // Reseta o contador de quantum sempre que a tarefa muda
    if (nova_tarefa != tarefa_atual) {
        ticks_no_quantum = 0;
    }
    tarefa_atual = nova_tarefa;
}

int CPU::get_ticks_no_quantum() const {
    return ticks_no_quantum;
}

int CPU::get_tempo_ocioso() const {
    return tempo_ocioso;
}
