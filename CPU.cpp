#include "CPU.h"

CPU::CPU() : tarefaAtual(nullptr), tempoOcioso(0) {
	// Inicializa a CPU sem nenhuma tarefa e com tempo ocioso zero
};

void CPU::executarTick() {
	if (tarefaAtual != nullptr) { 
		tarefaAtual->estado = EXECUTANDO;
		tarefaAtual->tempoRestante--;
	
		if (tarefaAtual->tempoRestante <= 0) {
			tarefaAtual->estado = FINALIZADA;
			tarefaAtual = nullptr;
		}
	}
	else {
		tempoOcioso++;
	}
};

bool CPU::estaOcupada() const {
	return tarefaAtual != nullptr; // Retorna true se há uma tarefa em execução, false se a CPU está ociosa
};

Task* CPU::getTarefaAtual() const {
	return tarefaAtual; // Retorna um ponteiro para a tarefa em execução
};

void CPU::setTarefaAtual(Task* novaTarefa) {
	tarefaAtual = novaTarefa; // Define a tarefa atual para a CPU
};

int CPU::getTempoOcioso() const {
	return tempoOcioso; // Retorna o tempo total que a CPU ficou ociosa
};
