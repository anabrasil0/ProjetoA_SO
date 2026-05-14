#include "Task.h"

// ============================================================
// Construtor
// Inicializa o TCB (Task Control Block) com os dados vindos do
// arquivo de configuracao. O tempo_restante e igual a duracao no
// inicio: ele sera decrementado a cada tick que a tarefa executar.
// O estado inicial e CRIADA — a transicao para PRONTA so acontece
// no tick em que o tempo de ingresso for atingido (em Simulador).
// ============================================================
Task::Task(int id, int ingresso, int duracao, int prioridade, const std::string& cor)
    : id(id),
      tempo_ingresso(ingresso),  // instante em que a tarefa entra no sistema
      duracao(duracao),          // tempo total fixo que a tarefa precisa de CPU
      tempo_restante(duracao),   // comeca igual a duracao; diminui a cada tick executado
      prioridade(prioridade),    // valor estatico usado pelo escalonador PRIOP
      estado(CRIADA),            // ainda nao esta pronta para executar
      cor(cor)                   // codigo hex RGB para identificacao visual no Gantt
{}

// ============================================================
// Getters — retornam os atributos do TCB sem permitir alteracao
// externa direta. Todos sao const porque nao modificam o objeto.
// ============================================================

// Identificador unico da tarefa, definido no arquivo de config
int Task::get_id() const {
    return id;
}

// Tick em que a tarefa deve chegar ao sistema e entrar na fila de prontos
int Task::get_tempo_ingresso() const {
    return tempo_ingresso;
}

// Duracao total original da tarefa (nunca muda apos criacao)
int Task::get_duracao() const {
    return duracao;
}

// Tempo que ainda falta para a tarefa terminar (decrementado pela CPU a cada tick)
int Task::get_tempo_restante() const {
    return tempo_restante;
}

// Prioridade estatica: valor maior = maior prioridade no escalonador PRIOP
int Task::get_prioridade() const {
    return prioridade;
}

// Estado atual no ciclo de vida: CRIADA -> PRONTA -> EXECUTANDO -> FINALIZADA
Estado Task::get_estado() const {
    return estado;
}

// Cor em hexadecimal RGB (ex: "FF0000" = vermelho) usada para pintar o bloco no Gantt
std::string Task::get_cor() const {
    return cor;
}

// ============================================================
// Setters — usados em dois contextos:
//   1. set_tempo_restante: chamado por CPU::processar_ciclo a cada tick
//      para decrementar o progresso da tarefa.
//   2. set_estado: chamado pelo scheduler (PRONTA/EXECUTANDO), pela CPU
//      (FINALIZADA) e pelo Simulador em step_backward para restaurar
//      o estado de um tick anterior (Requisito 1.5.2).
// ============================================================

// Atualiza o tempo restante; normalmente chamado com (tempo_restante - 1) por processar_ciclo
void Task::set_tempo_restante(int t) {
    tempo_restante = t;
}

// Atualiza o estado no ciclo de vida da tarefa
void Task::set_estado(Estado e) {
    estado = e;
}
