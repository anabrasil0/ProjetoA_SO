#pragma once
#include <stdio.h>
#include "Task.h"

//Classe CPU, representa um núcleo de processamento
class CPU {
private:
	Task* tarefaAtual; //ponteiro para a tarefa atualmente em execução (se nulo, a CPU está ociosa)
	int tempoOcioso; //contador de tempo que a cpu ficou sem uso 

public:
	CPU(); // função construtora para criar a CPU
	void executarTick();
	bool estaOcupada() const;
	Task* getTarefaAtual() const;
	void setTarefaAtual(Task* novaTarefa);
	int getTempoOcioso() const;
};
//a classe cpu se conecta com a task de tal forma que: se tem tarefa -> CPU executa
//senão -> CPU fica ociosa
