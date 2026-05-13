/*
 * main.cpp -- Ponto de entrada do Simulador de SO Multitarefa
 *
 * Responsabilidades:
 *   1. Solicitar o arquivo de configuracao ao usuario (Requisito 3.3)
 *   2. Apresentar menu de modo de execucao (Requisito 1.5)
 *   3. Modo passo-a-passo (a): loop interativo com avancar/retroceder/inspecionar/modificar (Req. 1.5.1 e 1.5.2)
 *   4. Modo completo (b): rodar run_complete() e exibir resultado final (Req. 1.5.3)
 */

#include <iostream>
#include <string>
#include <limits>
#include <cctype>

#include "Config.h"
#include "Simulador.h"
#include "Task.h"

// ============================================================
// Helpers locais
// ============================================================

/* Converte o enum Estado para texto legivel */
static std::string estado_para_string(Estado e) {
    switch (e) {
        case CRIADA:     return "CRIADA";
        case PRONTA:     return "PRONTA";
        case EXECUTANDO: return "EXECUTANDO";
        case FINALIZADA: return "FINALIZADA";
        default:         return "DESCONHECIDO";
    }
}

/* Exibe todas as informacoes de um TCB individualmente (Requisito 1.5.1) */
static void inspecionar_tarefa(const Simulador& sim) {
    std::cout << "ID da tarefa a inspecionar: ";
    int id;
    std::cin >> id;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    for (const Task* t : sim.get_all_tasks()) {
        if (t->get_id() == id) {
            std::cout << "\n--- TCB: Tarefa " << id << " ---" << std::endl;
            std::cout << "  Ingresso       : " << t->get_tempo_ingresso() << std::endl;
            std::cout << "  Duracao total  : " << t->get_duracao()        << std::endl;
            std::cout << "  Tempo restante : " << t->get_tempo_restante() << std::endl;
            std::cout << "  Prioridade     : " << t->get_prioridade()     << std::endl;
            std::cout << "  Estado         : " << estado_para_string(t->get_estado()) << std::endl;
            return;
        }
    }
    std::cout << "Tarefa " << id << " nao encontrada." << std::endl;
}

/* Permite ao usuario modificar o estado de qualquer tarefa (Requisito 3.4) */
static void modificar_tarefa(Simulador& sim) {
    std::cout << "ID da tarefa a modificar: ";
    int id;
    std::cin >> id;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Novo estado:" << std::endl;
    std::cout << "  [1] PRONTA" << std::endl;
    std::cout << "  [2] FINALIZADA" << std::endl;
    std::cout << "Escolha: ";

    int op;
    std::cin >> op;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    Estado novo;
    switch (op) {
        case 1: novo = PRONTA;     break;
        case 2: novo = FINALIZADA; break;
        default:
            std::cout << "Opcao invalida. Nenhuma modificacao feita." << std::endl;
            return;
    }

    sim.modificar_estado_tarefa(id, novo);
}

/* Exibe as opcoes disponiveis no modo passo-a-passo */
static void exibir_menu_passo() {
    std::cout << "\n  [A] Avancar tick  [R] Retroceder  "
              << "[I] Inspecionar tarefa  [M] Modificar estado  [S] Sair" << std::endl;
    std::cout << "  Comando: ";
}

// ============================================================
// main
// ============================================================
int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "   SIMULADOR DE SO MULTITAREFA - 2026     " << std::endl;
    std::cout << "==========================================" << std::endl;

    /* Carrega o arquivo de configuracao (Requisito 3.3) */
    std::string caminho;
    std::cout << "\nArquivo de configuracao [config.txt]: ";
    std::getline(std::cin, caminho);
    if (caminho.empty()) caminho = "config.txt";

    Config config(caminho);
    Simulador simulador(config);

    /* Seleciona o modo de execucao (Requisito 1.5) */
    std::cout << "\nModo de execucao:" << std::endl;
    std::cout << "  [1] Passo-a-passo (debugger)      <- padrao" << std::endl;
    std::cout << "  [2] Execucao completa" << std::endl;
    std::cout << "Escolha [1]: ";

    std::string entrada_modo;
    std::getline(std::cin, entrada_modo);
    int modo = entrada_modo.empty() ? 1 : std::stoi(entrada_modo);

    /* --------------------------------------------------------
     * MODO B - Execucao completa (Requisito 1.5.3)
     * Roda do_tick() em loop sem exibir passos intermediarios;
     * ao final exibe apenas o estado final do sistema.
     * -------------------------------------------------------- */
    if (modo == 2) {
        std::cout << "\nExecutando simulacao completa..." << std::endl;
        simulador.run_complete();
        simulador.imprimir_status();
        return 0;
    }

    /* --------------------------------------------------------
     * MODO A - Passo-a-passo interativo (Requisito 1.5.1 e 1.5.2)
     * A cada tick o usuario pode avancar, retroceder, inspecionar
     * ou modificar manualmente o estado de qualquer tarefa.
     * -------------------------------------------------------- */
    std::cout << "\n--- MODO PASSO-A-PASSO ---" << std::endl;
    simulador.imprimir_status();

    while (true) {
        if (simulador.simulacao_concluida()) {
            std::cout << "\nSimulacao concluida no tick "
                      << simulador.get_relogio_atual() << "!" << std::endl;
            simulador.imprimir_status();
            break;
        }

        exibir_menu_passo();

        char op = '\0';
        std::cin >> op;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        op = static_cast<char>(std::toupper(static_cast<unsigned char>(op)));

        switch (op) {
            case 'A':
                /* Avanca um tick e salva snapshot para possivel retrocesso */
                simulador.step_forward();
                simulador.imprimir_status();
                break;

            case 'R':
                /* Restaura o estado do tick anterior (Requisito 1.5.2) */
                simulador.step_backward();
                simulador.imprimir_status();
                break;

            case 'I':
                /* Exibe o TCB completo de uma tarefa individual (Requisito 1.5.1) */
                inspecionar_tarefa(simulador);
                break;

            case 'M':
                /* Modifica manualmente o estado de qualquer tarefa (Requisito 3.4) */
                modificar_tarefa(simulador);
                simulador.imprimir_status();
                break;

            case 'S':
                std::cout << "Saindo da simulacao." << std::endl;
                return 0;

            default:
                std::cout << "Comando invalido. Tente novamente." << std::endl;
                break;
        }
    }

    return 0;
}
