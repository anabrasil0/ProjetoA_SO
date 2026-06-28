#include "CPU.h"

// ============================================================
// Construtor
// Toda CPU nasce ligada, sem tarefa e com contadores zerados.
// O scheduler pode desligá-la logo no primeiro tick se nao houver
// tarefas disponiveis (Requisito 1.2).
// ============================================================
CPU::CPU()
    : tarefa_atual(nullptr), // sem tarefa no inicio
      ligada(true),          // ligada por padrao; o scheduler decide desligar
      tempo_ocioso(0),       // nenhum tick ocioso acumulado
      tempo_desligada(0),    // nenhum tick desligado acumulado
      ticks_no_quantum(0)    // contador de quantum zerado
{}

// ============================================================
// processar_ciclo — PUBLICO
// Responde a um tick do relogio global ja disparado pelo Simulador.
// Esta CPU nao controla o clock — ela apenas avanca seu estado interno
// por um ciclo quando o Simulador a instrui a faze-lo.
//
// Tres situacoes possiveis:
//   1. CPU desligada: acumula tempo desligado e retorna sem fazer nada.
//   2. CPU ocupada:   executa um ciclo da tarefa atual; se ela terminar,
//                     marca como FINALIZADA e libera a CPU.
//   3. CPU ociosa:    acumula tempo ocioso (ligada mas sem tarefa).
// ============================================================
void CPU::processar_ciclo() {

    // Situacao 1: CPU desligada por ausencia de tarefas (Requisito 1.2)
    if (!ligada) {
        tempo_desligada++; // contabiliza o tempo sem energia para estatisticas
        return;            // nada mais a fazer neste ciclo
    }

    // Situacao 2: CPU ocupada — executa um ciclo da tarefa atual
    if (tarefa_atual != nullptr) {
        tarefa_atual->set_estado(EXECUTANDO); // garante que o estado reflita a execucao
        ticks_no_quantum++;                   // incrementa o uso do quantum desta tarefa

        // Contabiliza mais um tick de CPU consumido por esta tarefa.
        // ticks_executados e a base para disparar eventos relativos ao inicio (Req. 2.4).
        tarefa_atual->incrementar_ticks_executados();

        // Decrementa o progresso restante da tarefa em 1 tick
        tarefa_atual->set_tempo_restante(tarefa_atual->get_tempo_restante() - 1);

        // Verifica se a tarefa esgotou seu tempo de execucao
        if (tarefa_atual->get_tempo_restante() <= 0) {
            tarefa_atual->set_estado(FINALIZADA); // marca a tarefa como concluida
            tarefa_atual     = nullptr;           // libera a CPU para a proxima escala
            ticks_no_quantum = 0;                 // reseta o contador de quantum
        }
    }
    // Situacao 3: CPU ligada mas sem tarefa atribuida
    else {
        tempo_ocioso++; // contabiliza ciclo desperdicado para estatisticas
    }
}

// ============================================================
// ligar — PUBLICO (Requisito 1.2)
// Reativa a CPU apos um periodo de desligamento.
// Chamado pelo scheduler ao encontrar trabalho disponivel na fila
// de prontos. Depois de ligar, a CPU cai no bloco de CPU ociosa
// dentro do mesmo tick e ja recebe uma tarefa.
// ============================================================
void CPU::ligar() {
    ligada = true;
}

// ============================================================
// desligar — PUBLICO (Requisito 1.2)
// Desliga a CPU quando nao ha tarefas prontas na fila.
// Economiza recursos simulados e deixa claro no Gantt que a CPU
// estava inativa naquele periodo. O scheduler volta a liga-la
// assim que uma tarefa entrar na fila de prontos.
// ============================================================
void CPU::desligar() {
    ligada = false;
}

// ============================================================
// Getters simples
// ============================================================

// Retorna true se ha uma tarefa atribuida (tarefa_atual != nullptr)
bool CPU::esta_ocupada() const {
    return tarefa_atual != nullptr;
}

// Retorna true se a CPU esta ligada (pode estar ociosa ou executando)
bool CPU::esta_ligada() const {
    return ligada;
}

// Retorna o ponteiro para a tarefa em execucao (nullptr se ociosa/desligada)
Task* CPU::get_tarefa_atual() const {
    return tarefa_atual;
}

// Total de ticks em que a CPU esteve ligada mas sem tarefa atribuida
int CPU::get_tempo_ocioso() const {
    return tempo_ocioso;
}

// Total de ticks em que a CPU esteve desligada
int CPU::get_tempo_desligada() const {
    return tempo_desligada;
}

// Ticks consecutivos que a tarefa atual ja usou neste quantum
int CPU::get_ticks_no_quantum() const {
    return ticks_no_quantum;
}

// ============================================================
// set_tarefa_atual — PUBLICO
// Atribui ou remove a tarefa em execucao nesta CPU.
// O contador de quantum e resetado APENAS quando a tarefa muda,
// ou seja, se a mesma tarefa for re-atribuida (ex: preempcao falsa),
// o contador continua de onde parou — evitando acumulo incorreto.
// ============================================================
void CPU::set_tarefa_atual(Task* nova_tarefa) {
    // So reseta o quantum se a tarefa realmente mudou
    if (nova_tarefa != tarefa_atual) {
        ticks_no_quantum = 0; // nova tarefa comeca com quantum do zero
    }
    tarefa_atual = nova_tarefa; // atribui (ou remove, se nullptr) a tarefa
}
