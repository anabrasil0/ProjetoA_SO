#pragma once
#include <vector>
#include <set>

// Forward declarations
class Task;
class CPU;

// Classe base abstrata para os algoritmos de escalonamento (Requisito 4.2)
// Novos algoritmos devem herdar desta classe e implementar escalonar().
// O quantum e recebido aqui para que cada scheduler enforce a preempcao por
// tempo sem que o Simulador precise conhecer essa logica.
//
// sorteio_ids armazena os IDs das tarefas escolhidas por sorteio aleatorio
// durante o ultimo escalonar(). O Simulador le esses IDs apos escalonar()
// para marcar o evento grafico correspondente no Gantt (Requisito 4.3).
// Usar std::set evita duplicatas, pois std::min_element pode chamar o
// comparador multiplas vezes com o mesmo par de tarefas.
class Scheduler {
protected:
    int           quantum;     // maximo de ticks consecutivos que uma tarefa pode rodar
    std::set<int> sorteio_ids; // IDs das tarefas escolhidas por sorteio neste tick

public:
    explicit Scheduler(int quantum) : quantum(quantum) {}
    virtual ~Scheduler() {}

    virtual void escalonar(std::vector<Task*>& prontos,
                           std::vector<CPU>& cpus,
                           int relogio_global) = 0;

    // Limpa os sorteios registrados; chamado no inicio de cada escalonar()
    void limpar_sorteios() { sorteio_ids.clear(); }

    // Retorna os IDs das tarefas escolhidas por sorteio neste tick
    const std::set<int>& get_sorteio_ids() const { return sorteio_ids; }
};
