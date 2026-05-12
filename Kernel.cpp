#include "Kernel.h"
#include "SRTFScheduler.h"
#include "PrioPScheduler.h"
#include <iostream>
#include <algorithm>

// ============================================================
// Construtor
// ============================================================
Kernel::Kernel(Config& config) {

    // Normaliza o nome do algoritmo para maiúsculas (Requisito 3.3.2)
    // Config já normaliza o algoritmo para maiúsculas no parse (Requisito 3.3.2)
    std::string algoritmo = config.get_algoritmo();

    // Instancia o escalonador correto (polimorfismo — Requisito 4.2)
    if (algoritmo == "SRTF") {
        escalonador = new SRTFScheduler();
    } else if (algoritmo == "PRIOP") {
        escalonador = new PrioPScheduler();
    } else {
        std::cout << "Aviso: Algoritmo desconhecido. Usando PRIOP por padrao." << std::endl;
        escalonador = new PrioPScheduler();
    }

    relogio_global = 0;
    quantum        = config.get_quantum();
    qtde_cpus      = config.get_cpus();

    // Cria as CPUs (Requisito 1.2)
    for (int i = 0; i < qtde_cpus; i++) {
        cpus.push_back(CPU());
    }

    // Cria os TCBs a partir dos dados brutos (Requisito 1.3)
    for (const auto& t_data : config.get_tarefas()) {
        Task* nova_task = new Task(
            t_data.id,
            t_data.ingresso,
            t_data.duracao,
            t_data.prioridade
        );
        all_tasks.push_back(nova_task);
    }

    std::cout << "Kernel inicializado com " << qtde_cpus
              << " CPUs e algoritmo " << config.get_algoritmo() << std::endl;
}

// ============================================================
// Destrutor
// ============================================================
Kernel::~Kernel() {
    for (Task* t : all_tasks) {
        delete t;
    }
    all_tasks.clear();

    if (escalonador != nullptr) {
        delete escalonador;
    }

    std::cout << "Memoria do Kernel limpa com sucesso." << std::endl;
}

// ============================================================
// do_tick — PRIVADO
// Lógica pura de um tick: chegada → escalonamento → execução → avanço.
// Não salva snapshot; isso é responsabilidade de step_forward.
// Separar aqui permite que run_complete() chame do_tick() diretamente,
// sem o custo de salvar estado a cada tick.
// ============================================================
void Kernel::do_tick() {

    // 1. Admite tarefas que chegam neste tick na fila de prontos
    verificar_chegada_tarefas();

    // 2. Escalonador decide qual tarefa vai para cada CPU (Requisito 4)
    escalonador->agendar(prontos, cpus, relogio_global);

    // 3. Cada CPU executa um tick na sua tarefa atual
    for (size_t i = 0; i < cpus.size(); i++) {
        cpus[i].executar_tick();
    }

    // 4. Remove da fila de prontos as tarefas que finalizaram
    prontos.erase(
        std::remove_if(prontos.begin(), prontos.end(),
            [](Task* t) { return t->get_estado() == FINALIZADA; }),
        prontos.end()
    );

    // 5. Avança o relógio global (Requisito 1.1)
    relogio_global++;
}

// ============================================================
// step_forward — PÚBLICO (Requisito 1.5 opção a)
// Salva o estado ANTES de avançar para permitir o retrocesso.
// É esse método que a interface chama no modo passo-a-passo.
// ============================================================
void Kernel::step_forward() {
    if (simulacao_concluida()) {
        std::cout << "Simulacao ja concluida." << std::endl;
        return;
    }

    // Snapshot tirado ANTES da mudança de estado, para que step_backward
    // consiga restaurar o tick anterior com fidelidade.
    salvar_estado();

    do_tick();
}

// ============================================================
// step_backward — PÚBLICO (Requisito 1.5.2)
// Restaura o estado do tick anterior a partir do histórico.
// ============================================================
void Kernel::step_backward() {
    if (historico.empty()) {
        std::cout << "Aviso: Nao ha estados para retroceder. Inicio atingido." << std::endl;
        return;
    }

    // Recupera e remove o topo da pilha de histórico
    Snapshot ultimo_estado = historico.back();
    historico.pop_back();

    // Restaura o relógio e as CPUs
    relogio_global = ultimo_estado.relogio;
    cpus           = ultimo_estado.estado_cpus;

    // Restaura o estado interno de cada tarefa pelo ID
    for (const auto& s_task : ultimo_estado.estado_tarefas) {
        for (Task* t : all_tasks) {
            if (t->get_id() == s_task.id) {
                t->set_tempo_restante(s_task.tempo_restante);
                t->set_estado(s_task.estado);
                break;
            }
        }
    }

    // Reconstrói a fila de prontos a partir do estado restaurado
    prontos.clear();
    for (Task* t : all_tasks) {
        if (t->get_estado() == PRONTA) {
            prontos.push_back(t);
        }
    }

    std::cout << "Sistema retrocedido para o tempo: " << relogio_global << std::endl;
}

// ============================================================
// run_complete — PÚBLICO (Requisito 1.5 opção b)
// Executa a simulação até o fim sem intervenção humana.
// Chama do_tick() diretamente, sem salvar snapshots, pois o modo
// completo não precisa de retrocesso (Requisito 1.5.3).
// ============================================================
void Kernel::run_complete() {
    while (!simulacao_concluida()) {
        do_tick();
    }
    std::cout << "Simulacao completa finalizada no tick " << relogio_global << std::endl;
}

// ============================================================
// verificar_chegada_tarefas — PRIVADO
// Admite na fila de prontos toda tarefa cujo ingresso == tick atual.
// ============================================================
void Kernel::verificar_chegada_tarefas() {
    for (Task* t : all_tasks) {
        if (t->get_tempo_ingresso() == relogio_global && t->get_estado() == PRONTA) {
            prontos.push_back(t);
            std::cout << "[Tick " << relogio_global << "] Tarefa " << t->get_id()
                      << " entrou na fila." << std::endl;
        }
    }
}

// ============================================================
// salvar_estado — PRIVADO
// Tira um snapshot completo do sistema (relógio + CPUs + TCBs).
// Chamado por step_forward antes de do_tick.
// ============================================================
void Kernel::salvar_estado() {
    Snapshot snap;
    snap.relogio    = relogio_global;
    snap.estado_cpus = cpus;           // cópia por valor do vetor de CPUs

    for (Task* t : all_tasks) {
        Snapshot::TaskState t_state;
        t_state.id             = t->get_id();
        t_state.tempo_restante = t->get_tempo_restante();
        t_state.estado         = t->get_estado();
        snap.estado_tarefas.push_back(t_state);
    }

    historico.push_back(snap);
}

// ============================================================
// simulacao_concluida — PÚBLICO
// ============================================================
bool Kernel::simulacao_concluida() const {
    if (!prontos.empty()) return false;

    for (const CPU& cpu : cpus) {
        if (cpu.esta_ocupada()) return false;
    }

    for (const Task* t : all_tasks) {
        if (t->get_estado() != FINALIZADA) return false;
    }

    return true;
}

// ============================================================
// imprimir_status — PÚBLICO
// ============================================================
void Kernel::imprimir_status() {
    std::cout << "\n========================================" << std::endl;
    std::cout << " TEMPO ATUAL: " << relogio_global
              << " | ALGORITMO: " << (escalonador ? "ATIVO" : "N/A") << std::endl;
    std::cout << "========================================" << std::endl;

    std::cout << " [CPUs]" << std::endl;
    for (size_t i = 0; i < cpus.size(); i++) {
        std::cout << "  CPU " << i << ": ";
        if (cpus[i].esta_ocupada()) {
            Task* t = cpus[i].get_tarefa_atual();
            std::cout << "[ID: " << t->get_id() << "] - "
                      << t->get_tempo_restante() << "s restantes" << std::endl;
        } 
        else {
            std::cout << "IDLE (Ociosa)" << std::endl;
        }
    }

    std::cout << "\n [FILA DE PRONTOS]" << std::endl;
    if (prontos.empty()) {
        std::cout << "  (Vazia)" << std::endl;
    } 
    else {
        std::cout << "  ";
        for (Task* t : prontos) {
            std::cout << "T" << t->get_id() << " ";
        }
        std::cout << std::endl;
    }

    int concluido = 0;
    for (Task* t : all_tasks) {
        if (t->get_estado() == FINALIZADA) concluido++;
    }
    std::cout << "\n Progresso: " << concluido << "/" << all_tasks.size()
              << " tarefas concluidas." << std::endl;
    std::cout << "========================================\n" << std::endl;
}
