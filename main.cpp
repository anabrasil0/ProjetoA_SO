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
        case CRIADA:         return "CRIADA";
        case PRONTA:         return "PRONTA";
        case EXECUTANDO:     return "EXECUTANDO";
        case SUSPENSA_MUTEX: return "SUSPENSA (mutex)";
        case SUSPENSA_IO:    return "SUSPENSA (E/S)";
        case FINALIZADA:     return "FINALIZADA";
        default:             return "DESCONHECIDO";
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
    if (config.get_algoritmo() == "PRIOPENV")
        std::cout << "  Alpha     : " << config.get_alpha() << " (envelhecimento/tick)" << std::endl;
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
    std::cout << "  [1] SRTF  [2] PRIOP  [3] PRIOPEnv (com envelhecimento)  [Enter] Manter" << std::endl;
    std::cout << "Escolha: ";
    std::string entrada;
    std::getline(std::cin, entrada);
    if      (entrada == "1") config.set_algoritmo("SRTF");
    else if (entrada == "2") config.set_algoritmo("PRIOP");
    else if (entrada == "3") config.set_algoritmo("PRIOPENV");

    int q = ler_inteiro_com_padrao("Quantum", config.get_quantum());
    if (q > 0) config.set_quantum(q);

    int n = ler_inteiro_com_padrao("Numero de CPUs", config.get_cpus());
    if (n > 0 && n < 2) {
        std::cout << "Numero minimo de CPUs e 2. Ajustando para 2." << std::endl;
        n = 2;
    }
    if (n > 0) config.set_cpus(n);

    // Alpha so e relevante para PRIOPEnv; oferece configuracao apenas se aplicavel
    if (config.get_algoritmo() == "PRIOPENV") {
        int a = ler_inteiro_com_padrao("Alpha (incremento de prioridade por tick de espera)", config.get_alpha());
        if (a > 0) config.set_alpha(a);
    }

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

/* Adiciona uma nova tarefa pelo menu, sem precisar editar o arquivo de
 * configuracao ou o codigo (Requisito 3.1: configurar o conjunto de tarefas
 * antes da simulacao). Usa Config::adicionar_tarefa, que ja existia mas
 * ainda nao estava exposta em nenhum menu. */
static void config_adicionar_tarefa(Config& config) {
    std::cout << "\n--- ADICIONAR TAREFA ---" << std::endl;

    int id;
    if (!ler_inteiro("ID da nova tarefa: ", id)) return;

    // O ID precisa ser unico (Requisito 1.3: TCB identificado por ID unico)
    for (const auto& t : config.get_tarefas()) {
        if (t.id == id) {
            std::cout << "Erro: ja existe uma tarefa com ID " << id
                       << ". Use outro ID ou edite a existente." << std::endl;
            return;
        }
    }

    TaskData td;
    td.id = id;

    std::cout << "Cor (RGB hex, ex: FF0000) [FFFFFF]: ";
    std::getline(std::cin, td.cor);
    if (td.cor.empty()) td.cor = "FFFFFF";

    if (!ler_inteiro("Tick de ingresso: ", td.ingresso))   return;
    if (!ler_inteiro("Duracao (ticks): ",  td.duracao))    return;
    if (!ler_inteiro("Prioridade: ",       td.prioridade)) return;

    std::cout << "Lista de eventos (Projeto B; ex: ML1:2,MU1:6 ou IO:3-4) [nenhum]: ";
    std::getline(std::cin, td.lista_eventos);

    config.adicionar_tarefa(td);
    std::cout << "Tarefa " << id << " adicionada com sucesso." << std::endl;
}

/* Menu interativo de configuracao pre-simulacao (Requisito 3.1 e 3.2) */
static void menu_configuracao(Config& config) {
    while (true) {
        exibir_config_atual(config);
        std::cout << "\n  [1] Alterar algoritmo / quantum / CPUs" << std::endl;
        std::cout << "  [2] Listar tarefas" << std::endl;
        std::cout << "  [3] Editar tarefa" << std::endl;
        std::cout << "  [4] Iniciar simulacao" << std::endl;
        std::cout << "  [5] Adicionar tarefa" << std::endl;

        int op;
        if (!ler_inteiro("Escolha: ", op)) continue;

        switch (op) {
            case 1: alterar_parametros(config);      break;
            case 2: listar_config_tarefas(config);   break;
            case 3: config_editar_tarefa(config);    break;
            case 4: return;
            case 5: config_adicionar_tarefa(config); break;
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
            std::cout << "  Ingresso         : " << t->get_tempo_ingresso()        << std::endl;
            std::cout << "  Duracao total    : " << t->get_duracao()               << std::endl;
            std::cout << "  Tempo restante   : " << t->get_tempo_restante()        << std::endl;
            std::cout << "  Prioridade       : " << t->get_prioridade()            << std::endl;
            std::cout << "  Idade (espera)   : " << t->get_idade()                 << " ticks" << std::endl;
            std::cout << "  Ticks executados : " << t->get_ticks_executados()      << std::endl;
            std::cout << "  Eventos restantes: " << (t->get_eventos().size() - t->get_indice_proximo_evento()) << std::endl;
            if (t->get_mutex_aguardando_id() >= 0)
                std::cout << "  Aguardando mutex : " << t->get_mutex_aguardando_id() << std::endl;
            std::cout << "  Estado           : " << estado_para_string(t->get_estado()) << std::endl;
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
// Argumentos de linha de comando (Requisito 4.2)
//
// O escalonador ja e plugavel no nivel de codigo (classe abstrata Scheduler +
// subclasses). Esta secao completa isso no nivel de USO: permite que quem
// adicionar um novo algoritmo (nova subclasse de Scheduler + novo "else if"
// em Simulador::Simulador) o selecione direto pela linha de comando, sem
// precisar editar o arquivo de configuracao nem navegar pelo menu interativo
// a cada teste. Isso facilita testar/comparar algoritmos em lote (scripts).
//
// Uso:
//   simulador.exe [arquivo_config] [--algoritmo NOME] [--quantum N]
//                 [--cpus N] [--alpha N] [--help]
//
// Qualquer argumento nao reconhecido como flag (nao comeca com '-') e
// tratado como o caminho do arquivo de configuracao.
// ============================================================
struct ArgsCLI {
    std::string arquivo_config;
    std::string algoritmo;   // vazio = nao sobrescrever o que vier do arquivo
    int quantum = -1;        // -1 = nao sobrescrever
    int cpus    = -1;
    int alpha   = -1;
    bool pedir_ajuda = false;
};

static void imprimir_ajuda_cli() {
    std::cout << "Uso: simulador [arquivo_config] [opcoes]\n"
              << "\nOpcoes:\n"
              << "  --algoritmo, -a NOME   Sobrescreve o algoritmo de escalonamento\n"
              << "                         (ex.: SRTF, PRIOP, PRIOPEnv, ou um novo\n"
              << "                         algoritmo cadastrado em Simulador.cpp)\n"
              << "  --quantum,   -q N      Sobrescreve o quantum\n"
              << "  --cpus,      -c N      Sobrescreve a quantidade de CPUs (minimo 2)\n"
              << "  --alpha         N      Sobrescreve o alpha (usado pelo PRIOPEnv)\n"
              << "  --help,      -h        Exibe esta mensagem e encerra\n"
              << std::endl;
}

static ArgsCLI parse_argumentos(int argc, char* argv[]) {
    ArgsCLI args;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        auto proximo_valor = [&](const std::string& nome_flag) -> std::string {
            if (i + 1 >= argc) {
                std::cout << "Aviso: flag " << nome_flag
                          << " informada sem valor; sera ignorada." << std::endl;
                return "";
            }
            return argv[++i];
        };

        if (arg == "--help" || arg == "-h") {
            args.pedir_ajuda = true;
        } else if (arg == "--algoritmo" || arg == "-a") {
            args.algoritmo = proximo_valor(arg);
        } else if (arg == "--quantum" || arg == "-q") {
            std::string v = proximo_valor(arg);
            if (!v.empty()) args.quantum = std::stoi(v);
        } else if (arg == "--cpus" || arg == "-c") {
            std::string v = proximo_valor(arg);
            if (!v.empty()) args.cpus = std::stoi(v);
        } else if (arg == "--alpha") {
            std::string v = proximo_valor(arg);
            if (!v.empty()) args.alpha = std::stoi(v);
        } else if (!arg.empty() && arg[0] == '-') {
            std::cout << "Aviso: flag desconhecida \"" << arg << "\" ignorada." << std::endl;
        } else if (args.arquivo_config.empty()) {
            args.arquivo_config = arg;
        }
    }
    return args;
}

// ============================================================
// main
// ============================================================
int main(int argc, char* argv[]) {
    std::cout << "====================================" << std::endl;
    std::cout << "   SIMULADOR DE SO MULTITAREFA      " << std::endl;
    std::cout << "====================================" << std::endl;

    ArgsCLI args = parse_argumentos(argc, argv);
    if (args.pedir_ajuda) {
        imprimir_ajuda_cli();
        return 0;
    }

    /* Carrega o arquivo de configuracao (Requisito 3.3).
     * Se o caminho ja veio pela linha de comando, pula a pergunta interativa
     * (permite rodar o simulador em lote/scripts sem interacao humana). */
    std::string nome_arquivo = args.arquivo_config;
    if (nome_arquivo.empty()) {
        std::cout << "Arquivo de configuracao: ";
        std::getline(std::cin, nome_arquivo);
        if (nome_arquivo.empty()) nome_arquivo = "caso-teste-mc-001-priop.txt";
    } else {
        std::cout << "Arquivo de configuracao: " << nome_arquivo
                   << " (via linha de comando)" << std::endl;
    }
    Config config(nome_arquivo);

    /* Sobrescreve os parametros informados via linha de comando (Requisito 4.2).
     * Os setters ja validam/normalizam os valores (ex.: minimo de 2 CPUs,
     * algoritmo em maiusculas), entao reaproveitamos as mesmas regras da
     * configuracao via arquivo/menu interativo. */
    if (!args.algoritmo.empty()) {
        config.set_algoritmo(args.algoritmo);
        std::cout << "Algoritmo sobrescrito via linha de comando: "
                   << config.get_algoritmo() << std::endl;
    }
    if (args.quantum > 0) config.set_quantum(args.quantum);
    if (args.cpus    > 0) config.set_cpus(args.cpus);
    if (args.alpha   > 0) config.set_alpha(args.alpha);

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
