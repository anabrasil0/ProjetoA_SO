#pragma once
#include "Scheduler.h"

class PROIPScheduler : public Scheduler {
	public:
		PROIPScheduler();
		virtual void escalonar(std::vector<Task*>& prontos, std::vector<CPU>& cpus, int relogio_global) override;
};