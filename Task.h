#pragma once
#include <string>

// Enumeração global para o ciclo de vida de uma tarefa no simulador.
// Fluxo principal: CRIADA → PRONTA → EXECUTANDO → FINALIZADA
//   CRIADA:     TCB existe, mas o instante de ingresso ainda não foi atingido.
//   PRONTA:     tarefa está na fila de prontos, aguardando ser escalonada.
//   EXECUTANDO: tarefa está atribuída a uma CPU e consumindo ticks.
//   FINALIZADA: tempo_restante chegou a zero; tarefa encerrou sua execução.
enum Estado {
    CRIADA,
    PRONTA,
    EXECUTANDO,
    FINALIZADA
};

// Classe Task — representa o TCB (Task Control Block) de cada tarefa (Requisito 1.3).
// Encapsula todos os dados e o estado de uma tarefa ao longo da simulação.
class Task {
private:
    int id;                    // identificador único da tarefa (definido no config.txt)
    int tempo_ingresso;        // tick em que a tarefa chega ao sistema e entra na fila de prontos
    int duracao;               // duração total original em ticks (valor fixo, nunca muda)
    int tempo_restante;        // ticks que faltam para terminar; decrementado pela CPU a cada ciclo
    int prioridade;            // prioridade estática usada pelo PRIOP; valor maior = mais prioritário
    Estado estado;             // posição atual no ciclo de vida: CRIADA→PRONTA→EXECUTANDO→FINALIZADA
    std::string cor;           // cor RGB hex para colorir o bloco no Gantt (ex: "FF0000" = vermelho)
    std::string lista_eventos; // reservado para eventos de I/O do Projeto B

public:
    Task(int id, int ingresso, int duracao, int prioridade, const std::string& cor = "FFFFFF");

    // Getters
    int         get_id()             const;
    int         get_tempo_ingresso() const;
    int         get_duracao()        const;
    int         get_tempo_restante() const;
    int         get_prioridade()     const;
    Estado      get_estado()         const;
    std::string get_cor()            const;

    // Setters (necessários para step_backward restaurar o TCB)
    void set_tempo_restante(int t);
    void set_estado(Estado e);
};
