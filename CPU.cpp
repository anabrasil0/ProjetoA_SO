#include "CPU.h"

// ============================================================
// Construtor
// ============================================================
CPU::CPU() : tarefa_atual(nullptr), ligada(true), tempo_ocioso(0),
             tempo_desligada(0), ticks_no_quantum(0) {}

// ============================================================
// processar_ciclo — PUBLICO
// Responde a um tick do relogio global ja disparado pelo Simulador.
// Esta CPU nao controla o clock — ela apenas avanca seu estado interno
// por um ciclo quando o Simulador a instrui a fazê-lo.
// Se desligada: contabiliza o tempo sem energia e retorna imediatamente.
// Se com tarefa: decrementa tempo restante, incrementa contador de quantum
//               e detecta finalizacao.
// Se ociosa:    contabiliza o tempo sem uso.
// ============================================================
void CPU::processar_ciclo() {

    if (!ligada) {
        tempo_desligada++;
        return;
    }

    if (tarefa_atual != nullptr) {
        tarefa_atual->set_estado(EXECUTANDO);
        ticks_no_quantum++;
        tarefa_atual->set_tempo_restante(tarefa_atual->get_tempo_restante() - 1);

        if (tarefa_atual->get_tempo_restante() <= 0) {
            tarefa_atual->set_estado(FINALIZADA);
            tarefa_atual     = nullptr;
            ticks_no_quantum = 0;
        }
    }
    else {
        tempo_ocioso++;
    }
}

// ============================================================
// ligar — PUBLICO (Requisito 1.2)
// Reativa a CPU apos um periodo de desligamento.
// Chamado pelo scheduler ao encontrar trabalho disponivel.
// ============================================================
void CPU::ligar() {
    ligada = true;
}

// ============================================================
// desligar — PUBLICO (Requisito 1.2)
// Desliga a CPU quando nao ha tarefas na fila de prontos.
// Economiza recursos e deixa claro no Gantt que a CPU estava inativa.
// ============================================================
void CPU::desligar() {
    ligada = false;
}

// ============================================================
// Getters e setters
// ============================================================

bool CPU::esta_ocupada() const {
    return tarefa_atual != nullptr;
}

bool CPU::esta_ligada() const {
    return ligada;
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

int CPU::get_tempo_ocioso() const {
    return tempo_ocioso;
}

int CPU::get_tempo_desligada() const {
    return tempo_desligada;
}

int CPU::get_ticks_no_quantum() const {
    return ticks_no_quantum;
}
