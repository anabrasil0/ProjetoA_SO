#pragma once
#include <vector>

// Forward declarations
class Task;
class CPU;

// Classe base abstrata para os algoritmos de escalonamento (Requisito 4.2)
// Novos algoritmos devem herdar desta classe e implementar agendar().
class Scheduler {
public:
    virtual ~Scheduler() {}
    virtual void agendar(std::vector<Task*>& prontos, std::vector<CPU>& cpus, int relogio_global) = 0;
};
