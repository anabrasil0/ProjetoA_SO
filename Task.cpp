#include "Task.h"

// Construtor da classe Task, que inicializa os atributos com os valores fornecidos
Task::Task(int id, int ingresso, int duracao, int prioridade) :
	id(id), tempoIngresso(ingresso), duracao(duracao),
	tempoRestante(duracao), prioridade(prioridade)
	//inicialmente, o tempo restante é igual à duração total
{
	estado = PRONTA; //todas as tarefas começam no estado pronta
};

// Métodos de acesso (getters) para os atributos privados da classe Task
int Task::getId() const {
	return id; // Retorna o identificador da tarefa
};

int Task::getTempoIngresso() const {
	return tempoIngresso; // Retorna o momento de chegada da tarefa no sistema
};

int Task::getDuracao() const {
	return duracao; // Retorna o tempo necessário para execução da tarefa
};

int Task::getTempoRestante() const {
	return tempoRestante; // Retorna o tempo que falta para finalizar a execução da tarefa
};

int Task::getPrioridade() const {
	return prioridade; // Retorna o nível de prioridade da tarefa no sistema
};

Estado Task::getEstado() const {
	return estado; // Retorna o estado atual da tarefa
};