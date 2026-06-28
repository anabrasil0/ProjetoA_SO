#include "MutexSim.h"
#include <iostream>

// ============================================================
// Construtor
// Cria o mutex identificado por `id`, inicialmente livre (dono_id = -1).
// ============================================================
MutexSim::MutexSim(int id) : id(id), dono_id(-1) {}

// ============================================================
// Getters
// ============================================================
int  MutexSim::get_id()      const { return id; }
int  MutexSim::get_dono_id() const { return dono_id; }
const std::vector<int>& MutexSim::get_fila() const { return fila; }
bool MutexSim::esta_livre()  const { return dono_id == -1; }

// ============================================================
// tentar_lock — PUBLICO (Req. 2.2 e 2.7)
// Tenta conceder o mutex a tarefa_id.
//   Livre:  atribui dono_id = tarefa_id, retorna true (adquiriu).
//   Ocupado: insere tarefa_id no final da fila FIFO, retorna false (bloqueou).
// O Simulador e quem muda o estado da tarefa para SUSPENSA_MUTEX se retornar false.
// ============================================================
bool MutexSim::tentar_lock(int tarefa_id) {
    if (dono_id == -1) {
        dono_id = tarefa_id;
        std::cout << "[Mutex " << id << "] T" << tarefa_id
                  << " adquiriu o mutex." << std::endl;
        return true;
    }
    std::cout << "[Mutex " << id << "] T" << tarefa_id
              << " bloqueou aguardando mutex (dono atual: T" << dono_id << ")." << std::endl;
    fila.push_back(tarefa_id);
    return false;
}

// ============================================================
// unlock — PUBLICO (Req. 2.3 e 2.7)
// Libera o mutex detido por tarefa_id.
//   Sem fila: dono_id = -1 (mutex livre), retorna -1.
//   Com fila: o primeiro da fila torna-se o novo dono (transferencia direta
//             de posse, sem passar pelo estado livre), retorna seu ID.
// O Simulador e quem acorda a tarefa retornada (muda para PRONTA).
// Imprime aviso se tarefa_id nao e o dono atual (operacao invalida).
// ============================================================
int MutexSim::unlock(int tarefa_id) {
    if (dono_id != tarefa_id) {
        std::cout << "[Mutex " << id << "] Aviso: T" << tarefa_id
                  << " tentou liberar mutex sem ser o dono (dono: T" << dono_id
                  << "). Operacao ignorada." << std::endl;
        return -1;
    }

    if (fila.empty()) {
        dono_id = -1; // mutex fica livre
        std::cout << "[Mutex " << id << "] T" << tarefa_id
                  << " liberou o mutex (agora livre)." << std::endl;
        return -1;
    }

    // Transfere diretamente para o proximo da fila
    int proximo = fila.front();
    fila.erase(fila.begin());
    dono_id = proximo;
    std::cout << "[Mutex " << id << "] T" << tarefa_id
              << " liberou. T" << proximo << " adquiriu (despertada)." << std::endl;
    return proximo;
}

// ============================================================
// Setters para step_backward
// ============================================================
void MutexSim::set_dono_id(int id)               { dono_id = id; }
void MutexSim::set_fila(const std::vector<int>& f) { fila    = f; }
