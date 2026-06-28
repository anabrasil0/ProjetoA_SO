#pragma once
#include <string>
#include <vector>
#include "Evento.h"

// Enumeração global para o ciclo de vida de uma tarefa no simulador.
// Fluxo principal: CRIADA → PRONTA → EXECUTANDO → FINALIZADA
//   CRIADA:         TCB existe, mas o instante de ingresso ainda não foi atingido.
//   PRONTA:         tarefa está na fila de prontos, aguardando ser escalonada.
//   EXECUTANDO:     tarefa está atribuída a uma CPU e consumindo ticks.
//   SUSPENSA_MUTEX: tarefa bloqueada aguardando liberação de um mutex (Req. 2).
//   SUSPENSA_IO:    tarefa bloqueada aguardando conclusão de operação de E/S (Req. 3).
//   FINALIZADA:     tempo_restante chegou a zero; tarefa encerrou sua execução.
enum Estado {
    CRIADA,
    PRONTA,
    EXECUTANDO,
    SUSPENSA_MUTEX,
    SUSPENSA_IO,
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
    Estado estado;             // posição atual no ciclo de vida
    std::string cor;           // cor RGB hex para colorir o bloco no Gantt (ex: "FF0000" = vermelho)
    int idade;                 // ticks acumulados na fila de prontos sem executar (PRIOPEnv)

    // Campos para eventos e mutex (Projeto B Req. 2)
    std::vector<Evento> eventos;       // lista de eventos a executar ao longo da vida da tarefa
    int ticks_executados;              // ticks de CPU consumidos desde o inicio; base para tempo_relativo
    int indice_proximo_evento;         // indice do proximo evento a verificar em `eventos`
    int mutex_aguardando_id;           // ID do mutex que bloqueia esta tarefa (-1 = nao bloqueada)
    int io_tick_conclusao;             // tick global em que a E/S conclui (-1 = nao em E/S, Req. 3)

public:
    // lista_eventos: string bruta do arquivo de config; parseada em `eventos` no construtor
    Task(int id, int ingresso, int duracao, int prioridade,
         const std::string& cor = "FFFFFF",
         const std::string& lista_eventos = "");

    // Getters
    int         get_id()                    const;
    int         get_tempo_ingresso()        const;
    int         get_duracao()               const;
    int         get_tempo_restante()        const;
    int         get_prioridade()            const;
    Estado      get_estado()                const;
    std::string get_cor()                   const;
    int         get_idade()                 const;
    int         get_ticks_executados()      const;
    int         get_indice_proximo_evento() const;
    int         get_mutex_aguardando_id()   const;
    int         get_io_tick_conclusao()     const;
    const std::vector<Evento>& get_eventos() const;

    // Setters para step_backward restaurar o TCB
    void set_tempo_restante(int t);
    void set_estado(Estado e);
    void set_idade(int i);
    void set_ticks_executados(int t);
    void set_indice_proximo_evento(int i);
    void set_mutex_aguardando_id(int id);
    void set_io_tick_conclusao(int t);

    // Operações de envelhecimento (PRIOPEnv)
    void incrementar_idade(int alpha);
    void resetar_idade();

    // Operações de execução (Req. 2)
    void incrementar_ticks_executados(); // chamado por CPU::processar_ciclo() a cada tick
    void avancar_indice_evento();        // marca o evento atual como processado
};
