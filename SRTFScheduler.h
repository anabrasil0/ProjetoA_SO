// Algoritmo SRTF
//a CPU deve sempre executar a tarefa com menor tempo_restante
#pragma once
#include "Scheduler.h"

class SRTFScheduler : public Scheduler {
	public:
		SRTFScheduler();
		virtual void escalonar(std::vector<Task*>& prontos, std::vector<CPU>& cpus, int relogio_global) override;
};

