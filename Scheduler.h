#pragma once
#include <vector>

// Forward declarations
class Task;
class CPU;

// Classe base abstrata para os algoritmos de escalonamento (Requisito 4.2)
// Novos algoritmos devem herdar desta classe e implementar escalonar().
// O quantum e recebido aqui para que cada scheduler enforce a preempcao por
// tempo sem que o Simulador precise conhecer essa logica.
class Scheduler {
protected:
    int quantum; // maximo de ticks consecutivos que uma tarefa pode rodar

public:
    explicit Scheduler(int quantum) : quantum(quantum) {}
    virtual ~Scheduler() {}
    virtual void escalonar(std::vector<Task*>& prontos, std::vector<CPU>& cpus, int relogio_global) = 0;
};
