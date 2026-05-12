#pragma once
#include <vector>

// Forward declarations
class Task;
class CPU;

// Classe base abstrata para os algoritmos de escalonamento (Requisito 4.2)
// Novos algoritmos devem herdar desta classe e implementar escalonar().
class Scheduler {
public:
    virtual ~Scheduler() {}
    virtual void escalonar(std::vector<Task*>& prontos, std::vector<CPU>& cpus, int relogio_global) = 0;
};
