//O scheduler (escalonador) é a parte do sistema que decide quem usa a CPU e quando.
#pragma once
#include <vector>

//Forward declarations para conhecer as classes sem incluir os arquivos completos
class Task;
class CPU;

//Classe base abstrata para os algoritmos de escalonamento

class Scheduler {
	public:
	virtual ~Scheduler() {} // Destrutor virtual para garantir limpeza em classes derivadas
	virtual void agendar(std::vector<Task*>& prontos, std::vector<CPU>& cpus, int relogio) = 0; // Método puro para escalonamento
};
