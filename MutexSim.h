#pragma once
#include <vector>

// MutexSim — simulacao de um mutex binario (exclusao mutua) — Req. 2.
//
// Armazena apenas IDs de tarefas (int), sem ponteiros Task*, para desacoplar
// a estrutura de dados do simulador e facilitar a serializacao para step_backward.
// O Simulador.cpp e responsavel por encontrar os ponteiros Task* a partir dos IDs
// e por alterar os estados das tarefas (SUSPENSA_MUTEX, PRONTA, etc.).
//
// Semantica de exclusao mutua (Req. 2.7):
//   - Apenas uma tarefa pode ser dona por vez (mutex binario).
//   - Se uma tarefa tenta lock enquanto outra ja e dona, ela bloqueia e entra
//     na fila de espera (FIFO).
//   - No unlock, a tarefa da frente da fila se torna a nova dona (transferencia
//     direta de posse) e e despertada pelo Simulador.
class MutexSim {
private:
    int id;                // numero do mutex (definido na lista_eventos)
    int dono_id;           // ID da tarefa que possui o mutex; -1 = livre
    std::vector<int> fila; // IDs das tarefas aguardando, em ordem de chegada (FIFO)

public:
    explicit MutexSim(int id);

    int  get_id()     const;
    int  get_dono_id() const;
    const std::vector<int>& get_fila() const;
    bool esta_livre() const;

    // Tenta adquirir o mutex para tarefa_id.
    // Retorna true se adquirido (tarefa_id se torna dona).
    // Retorna false se o mutex esta ocupado (tarefa_id vai para a fila de espera).
    bool tentar_lock(int tarefa_id);

    // Libera o mutex detido por tarefa_id.
    // Retorna o ID da proxima tarefa a despertar (agora dona), ou -1 se fila vazia.
    // O Simulador e responsavel por acordar a tarefa e atualizar seu estado.
    int unlock(int tarefa_id);

    // Setters para restauracao do estado em step_backward (Req. 1.5.2)
    void set_dono_id(int id);
    void set_fila(const std::vector<int>& f);
};
