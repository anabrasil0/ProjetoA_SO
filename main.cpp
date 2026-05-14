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
#include <iomanip>
#include <string>
#include <limits>
#include <cctype>

#include "Config.h"
#include "Simulador.h"
#include "Task.h"
#include "GanttChart.h"

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

// Le uma linha e tenta converter para inteiro.
// Retorna true e preenche 'resultado' em caso de sucesso.
// Retorna false e exibe mensagem de erro se a entrada nao for um numero valido.
// Usar esta funcao evita que cin entre em estado de falha (loop infinito)
// quando o usuario digita texto onde se espera numero (ex: "T1" em vez de "1").
static bool ler_inteiro(const std::string& prompt, int& resultado) {
    std::cout << prompt;
    std::string entrada;
    std::getline(std::cin, entrada);

    try {
        size_t pos;
        resultado = std::stoi(entrada, &pos);
        // Verifica se havia lixo apos o numero (ex: "1abc")
        if (pos != entrada.size()) {
            std::cout << "Erro: \"" << entrada << "\" nao e um numero valido. "
                      << "Digite apenas o numero (ex: 1, 2, 3)." << std::endl;
            return false;
        }
        return true;
    } catch (...) {
        std::cout << "Erro: \"" << entrada << "\" nao e um numero valido. "
                  << "Digite apenas o numero (ex: 1, 2, 3)." << std::endl;
        return false;
    }
}

// Le inteiro com valor padrao — Enter mantem o valor atual sem exibir erro
static int ler_inteiro_com_padrao(const std::string& prompt, int padrao) {
    std::cout << prompt << " [" << padrao << "]: ";
    std::string entrada;
    std::getline(std::cin, entrada);
    if (entrada.empty()) return padrao;
    try {
        size_t pos;
        int v = std::stoi(entrada, &pos);
        if (pos == entrada.size()) return v;
    } catch (...) {}
    std::cout << "Entrada invalida, mantendo " << padrao << "." << std::endl;
    return padrao;
}

// ============================================================
// Menu de configuracao pre-simulacao (Requisito 3.1)
// ============================================================

static void exibir_config_atual(const Config& config) {
    std::cout << "\n--- CONFIGURACAO DO SISTEMA ---" << std::endl;
    std::cout << "  Algoritmo : " << config.get_algoritmo() << std::endl;
    std::cout << "  Quantum   : " << config.get_quantum() << " ticks" << std::endl;
    std::cout << "  CPUs      : " << config.get_cpus() << std::endl;
    std::cout << "  Tarefas   : " << config.get_tarefas().size() << std::endl;
}

static void listar_config_tarefas(const Config& config) {
    const auto& tarefas = config.get_tarefas();
    if (tarefas.empty()) {
        std::cout << "  (Nenhuma tarefa cadastrada)" << std::endl;
        return;
    }
    std::cout << "\n"
              << std::right
              << std::setw(4)  << "ID"
              << std::setw(10) << "Ingresso"
              << std::setw(9)  << "Duracao"
              << std::setw(12) << "Prioridade"
              << std::endl;
    std::cout << "  " << std::string(33, '-') << std::endl;
    for (const auto& t : tarefas) {
        std::cout << std::right
                  << std::setw(4)  << t.id
                  << std::setw(10) << t.ingresso
                  << std::setw(9)  << t.duracao
                  << std::setw(12) << t.prioridade
                  << std::endl;
    }
}

static void alterar_parametros(Config& config) {
    std::cout << "\n--- ALTERAR PARAMETROS ---" << std::endl;

    std::cout << "Algoritmo atual: " << config.get_algoritmo() << std::endl;
    std::cout << "  [1] SRTF  [2] PRIOP  [Enter] Manter" << std::endl;
    std::cout << "Escolha: ";
    std::string entrada;
    std::getline(std::cin, entrada);
    if (entrada == "1") config.set_algoritmo("SRTF");
    else if (entrada == "2") config.set_algoritmo("PRIOP");

    int q = ler_inteiro_com_padrao("Quantum", config.get_quantum());
    if (q > 0) config.set_quantum(q);

    int n = ler_inteiro_com_padrao("Numero de CPUs", config.get_cpus());
    if (n > 0) config.set_cpus(n);

    std::cout << "Parametros atualizados." << std::endl;
}

static void config_editar_tarefa(Config& config) {
    listar_config_tarefas(config);

    int id;
    if (!ler_inteiro("\nID da tarefa a editar: ", id)) return;

    const auto& tarefas = config.get_tarefas();
    const TaskData* existente = nullptr;
    for (const auto& t : tarefas) {
        if (t.id == id) { existente = &t; break; }
    }

    if (existente == nullptr) {
        std::cout << "Tarefa " << id << " nao encontrada." << std::endl;
        return;
    }

    TaskData td = *existente;
    std::cout << "\n(Pressione Enter para manter o valor atual)" << std::endl;
    td.ingresso   = ler_inteiro_com_padrao("Tick de ingresso", td.ingresso);
    td.duracao    = ler_inteiro_com_padrao("Duracao (ticks)", td.duracao);
    td.prioridade = ler_inteiro_com_padrao("Prioridade", td.prioridade);

    config.editar_tarefa(id, td);
    std::cout << "Tarefa " << id << " atualizada com sucesso." << std::endl;
}

/* Menu interativo de configuracao pre-simulacao (Requisito 3.1 e 3.2) */
static void menu_configuracao(Config& config) {
    while (true) {
        exibir_config_atual(config);
        std::cout << "\n  [1] Alterar algoritmo / quantum / CPUs" << std::endl;
        std::cout << "  [2] Listar tarefas" << std::endl;
        std::cout << "  [3] Editar tarefa" << std::endl;
        std::cout << "  [4] Iniciar simulacao" << std::endl;

        int op;
        if (!ler_inteiro("Escolha: ", op)) continue;

        switch (op) {
            case 1: alterar_parametros(config);    break;
            case 2: listar_config_tarefas(config); break;
            case 3: config_editar_tarefa(config);  break;
            case 4: return;
            default: std::cout << "Opcao invalida." << std::endl; break;
        }
    }
}

/* Exibe todas as informacoes de um TCB individualmente (Requisito 1.5.1) */
static void inspecionar_tarefa(const Simulador& sim) {
    int id;
    if (!ler_inteiro("ID da tarefa a inspecionar (ex: 1): ", id)) return;

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
    std::cout << "Erro: Tarefa com ID " << id << " nao existe no sistema." << std::endl;
}

/* Permite ao usuario modificar o estado de qualquer tarefa (Requisito 3.4) */
static void modificar_tarefa(Simulador& sim) {
    int id;
    if (!ler_inteiro("ID da tarefa a modificar (ex: 1): ", id)) return;

    int op;
    std::cout << "Novo estado:" << std::endl;
    std::cout << "  [1] PRONTA" << std::endl;
    std::cout << "  [2] FINALIZADA" << std::endl;
    if (!ler_inteiro("Escolha: ", op)) return;

    Estado novo;
    switch (op) {
        case 1: novo = PRONTA;     break;
        case 2: novo = FINALIZADA; break;
        default:
            std::cout << "Erro: opcao invalida. Nenhuma modificacao feita." << std::endl;
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
    std::cout << "====================================" << std::endl;
    std::cout << "   SIMULADOR DE SO MULTITAREFA      " << std::endl;
    std::cout << "====================================" << std::endl;

    /* Carrega o arquivo de configuracao (Requisito 3.3) */
    Config config("config.txt");

    /* Menu de configuracao pre-simulacao (Requisito 3.1) */
    std::cout << "\nRevise e ajuste a configuracao antes de iniciar." << std::endl;
    menu_configuracao(config);

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
        GanttChart::exibir_terminal(simulador.get_gantt_log());
        GanttChart::exportar_svg(simulador.get_gantt_log());
        return 0;
    }

    /* --------------------------------------------------------
     * MODO A - Passo-a-passo interativo (Requisito 1.5.1 e 1.5.2)
     * A cada tick o usuario pode avancar, retroceder, inspecionar
     * ou modificar manualmente o estado de qualquer tarefa.
     * -------------------------------------------------------- */
    std::cout << "\n--- MODO PASSO-A-PASSO ---" << std::endl;
    simulador.imprimir_status();

    // Controla se o aviso de conclusao ja foi exibido, para nao repetir a cada comando
    bool conclusao_exibida = false;

    while (true) {
        if (simulador.simulacao_concluida() && !conclusao_exibida) {
            std::cout << "\nSimulacao concluida no tick "
                      << simulador.get_relogio_atual() << "!"
                      << " Use [R] para retroceder ou [S] para sair." << std::endl;
            simulador.imprimir_status();
            conclusao_exibida = true;
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
                GanttChart::exibir_terminal(simulador.get_gantt_log());
                break;

            case 'R':
                /* Restaura o estado do tick anterior (Requisito 1.5.2) */
                simulador.step_backward();
                conclusao_exibida = false; // simulacao pode nao estar mais concluida
                simulador.imprimir_status();
                GanttChart::exibir_terminal(simulador.get_gantt_log());
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
                GanttChart::exportar_svg(simulador.get_gantt_log());
                return 0;

            default:
                std::cout << "Comando invalido. Tente novamente." << std::endl;
                break;
        }
    }

    return 0;
}
