#include "Task.h"

// Construtor: inicializa o TCB com os dados da tarefa; tempo_restante começa igual à duração
Task::Task(int id, int ingresso, int duracao, int prioridade)
    : id(id), tempo_ingresso(ingresso), duracao(duracao),
      tempo_restante(duracao), prioridade(prioridade), estado(CRIADA)
{}

int Task::get_id() const {
    return id;
}

int Task::get_tempo_ingresso() const {
    return tempo_ingresso;
}

int Task::get_duracao() const {
    return duracao;
}

int Task::get_tempo_restante() const {
    return tempo_restante;
}

int Task::get_prioridade() const {
    return prioridade;
}

Estado Task::get_estado() const {
    return estado;
}

void Task::set_tempo_restante(int t) {
    tempo_restante = t;
}

void Task::set_estado(Estado e) {
    estado = e;
}
