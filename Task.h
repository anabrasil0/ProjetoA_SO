#include <stdio.h>

// Enumeração para representar os estados das tarefas
enum Estado {
	PRONTA, 
	EXECUTANDO, 
	FINALIZADA
};

//Classe das tarefas, com os atributos necessários para o escalonamento
class Task {
private:
	int id; //identificador
	//cor
	int tempoIngresso; //momento de chegada no sistema
	int duracao; //tempo necessário para execução (fixa)
	int tempoRestante; //tempo que falta para finalizar a execução (altera)
	int prioridade; //nível de prioridade no sistema
	Estado estado;
	std::string listaEventos; //não implementado nesse projeto

public:
	Task(int id, int ingresso, int duracao, int prioridade); // função construtora p/ criar a tarefa
	int getId() const;
	int getTempoIngresso() const;
	int getDuracao() const;
	int getTempoRestante() const;
	int getPrioridade() const;
	Estado getEstado() const;
};