#pragma once
#include <string>

// Enumeração global para representar os estados possíveis de uma tarefa
enum Estado {
    CRIADA,
    PRONTA,
    EXECUTANDO,
    FINALIZADA
};

// Classe Task — representa o TCB (Task Control Block) de cada tarefa (Requisito 1.3)
class Task {
private:
    int id;
    int tempo_ingresso;     // instante de chegada no sistema
    int duracao;            // tempo total necessário para execução (fixo)
    int tempo_restante;     // tempo que ainda falta para terminar (decrementado a cada tick)
    int prioridade;
    Estado estado;
    std::string lista_eventos; // reservado para o Projeto B

public:
    Task(int id, int ingresso, int duracao, int prioridade);

    // Getters
    int    get_id()             const;
    int    get_tempo_ingresso() const;
    int    get_duracao()        const;
    int    get_tempo_restante() const;
    int    get_prioridade()     const;
    Estado get_estado()         const;

    // Setters (necessários para step_backward restaurar o TCB)
    void set_tempo_restante(int t);
    void set_estado(Estado e);
};
