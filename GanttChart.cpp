#include "GanttChart.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <string>
#include <vector>
#include <cstdlib>

// ============================================================
// Helpers de cor ANSI (24-bit true color)
// ============================================================

static void parse_hex(const std::string& hex, int& r, int& g, int& b) {
    std::string h = hex;
    while (h.size() < 6) h = "0" + h;
    try {
        r = std::stoi(h.substr(0, 2), nullptr, 16);
        g = std::stoi(h.substr(2, 2), nullptr, 16);
        b = std::stoi(h.substr(4, 2), nullptr, 16);
    } catch (...) {
        r = 180; g = 180; b = 180;
    }
}

static std::string ansi_bg(int r, int g, int b) {
    return "\033[48;2;" + std::to_string(r) + ";"
                        + std::to_string(g) + ";"
                        + std::to_string(b) + "m";
}

static const std::string FG_BRANCO = "\033[97m";
static const std::string RESET     = "\033[0m";

// Largura do conteudo de cada celula (em caracteres)
static const int W = 5;

// ============================================================
// print_celula — imprime uma celula da secao de TAREFAS
// Eventos de chegada e termino recebem marcadores visuais (Requisito 2.2)
// Sorteio substitui o ultimo char do label por '?' (Requisito 4.3)
// ============================================================
static void print_celula(const EntradaGantt* e) {
    if (!e || e->tipo == TipoGantt::NAO_CHEGOU) {
        std::cout << std::string(W, ' ');
        return;
    }

    switch (e->tipo) {

        case TipoGantt::EXECUTANDO: {
            int r, g, b;
            parse_hex(e->cor, r, g, b);

            std::string label;
            if (e->evento_chegada && e->evento_termino) {
                label = ">C" + std::to_string(e->cpu_id + 1) + "! ";
            } else if (e->evento_chegada) {
                label = ">C" + std::to_string(e->cpu_id + 1) + "  ";
            } else if (e->evento_termino) {
                label = "C" + std::to_string(e->cpu_id + 1) + "! ";
                if (label.size() < (size_t)W) label += " ";
            } else {
                label = "CPU" + std::to_string(e->cpu_id + 1) + " ";
            }

            label = label.substr(0, W);

            // Eventos substituem o ultimo char pelo marcador correspondente (Req. 2.8 e 3)
            if (!label.empty()) {
                if      (e->evento_mutex_lock)   label.back() = 'L'; // lock de mutex
                else if (e->evento_mutex_unlock) label.back() = 'U'; // unlock de mutex
                else if (e->evento_io)           label.back() = 'I'; // inicio de E/S (Req. 3)
                else if (e->evento_sorteio)      label.back() = '?'; // escolha por sorteio
            }

            std::cout << ansi_bg(r, g, b) << FG_BRANCO << label << RESET;
            break;
        }

        case TipoGantt::PRONTA: {
            // Mostra 'L' se uma tarefa foi despertada de mutex neste tick (woke up)
            std::string label = e->evento_chegada ? " >>  " : "  .  ";
            std::cout << label;
            break;
        }

        case TipoGantt::SUSPENSA_MUTEX:
            // Fundo vermelho escuro; 'L' indica que o motivo e espera de mutex (Req. 2.9)
            std::cout << ansi_bg(139, 0, 0) << FG_BRANCO
                      << (e->evento_mutex_lock ? " MLk " : " MUT ") << RESET;
            break;

        case TipoGantt::SUSPENSA_IO:
            // Fundo azul escuro; "IO> " indica primeiro tick de E/S (Req. 2.9 e 3)
            std::cout << ansi_bg(0, 0, 139) << FG_BRANCO
                      << (e->evento_io ? " IO> " : " I/O ") << RESET;
            break;

        case TipoGantt::FINALIZADA:
            std::cout << " fim ";
            break;

        default:
            std::cout << std::string(W, ' ');
    }
}

// ============================================================
// print_celula_cpu — imprime uma celula da secao de CPUs (Requisito 1.2)
// EXECUTANDO: fundo colorido da tarefa + ID da tarefa
// OCIOSA:     traco indicando CPU ligada sem trabalho
// DESLIGADA:  fundo escuro + "OFF" indicando CPU sem energia
// ============================================================
static void print_celula_cpu(const EntradaCPU* ec) {
    if (!ec) {
        std::cout << std::string(W, ' ');
        return;
    }

    switch (ec->estado) {

        case EstadoCPU::EXECUTANDO: {
            int r, g, b;
            parse_hex(ec->cor, r, g, b);
            // Monta label "T{id}" com padding ate W chars
            std::string label = "T" + std::to_string(ec->tarefa_id);
            while (label.size() < (size_t)W) label += " ";
            label = label.substr(0, W);
            std::cout << ansi_bg(r, g, b) << FG_BRANCO << label << RESET;
            break;
        }

        case EstadoCPU::OCIOSA:
            // Traco: CPU ligada mas sem tarefa atribuida
            std::cout << "  -  ";
            break;

        case EstadoCPU::DESLIGADA:
            // Fundo escuro: CPU sem energia por ausencia de tarefas (Requisito 1.2)
            std::cout << ansi_bg(40, 40, 40) << FG_BRANCO << " OFF " << RESET;
            break;
    }
}

// ============================================================
// Linha separadora horizontal: +-----+-----+...+-----+
// ============================================================
static void linha_sep(int n) {
    std::cout << "        +";
    for (int i = 0; i < n; i++) {
        std::cout << std::string(W, '-') << "+";
    }
    std::cout << "\n";
}

// Linha separadora dupla (marca a divisao entre secao de CPUs e Tarefas)
static void linha_sep_dupla(int n) {
    std::cout << "        +";
    for (int i = 0; i < n; i++) {
        std::cout << std::string(W, '=') << "+";
    }
    std::cout << "\n";
}

// ============================================================
// exibir_terminal — PUBLICO (Requisito 2.1, 2.2, 2.3 e 1.2)
// Exibe o Gantt em duas secoes:
//   [CPUs]    — mostra o estado de cada processador por tick
//   [Tarefas] — mostra o estado de cada tarefa por tick
// ============================================================
void GanttChart::exibir_terminal(const GanttLog& log) {
    const auto& ticks = log.get_ticks();
    if (ticks.empty()) return;

    // IDs de tarefas em ordem decrescente (Requisito 2.5)
    std::vector<int> ids;
    for (const auto& e : ticks[0].entradas) ids.push_back(e.tarefa_id);
    std::sort(ids.begin(), ids.end(), std::greater<int>());

    int n      = (int)ticks.size();
    int n_cpus = (int)ticks[0].cpus.size();

    std::cout << "\n=== GANTT ===\n";

    // Numeros dos ticks no cabecalho
    std::cout << "         ";
    for (const auto& t : ticks)
        std::cout << std::right << std::setw(W) << t.tick << " ";
    std::cout << "\n";

    // ---- Secao de CPUs (Requisito 1.2) ----
    std::cout << " [CPUs]\n";
    for (int ci = 0; ci < n_cpus; ci++) {
        linha_sep(n);
        std::cout << "  CPU" << std::left << std::setw(2) << (ci + 1) << " |";
        for (const auto& tick : ticks) {
            const EntradaCPU* found = nullptr;
            for (const auto& ec : tick.cpus)
                if (ec.cpu_id == ci) { found = &ec; break; }
            print_celula_cpu(found);
            std::cout << "|";
        }
        std::cout << "\n";
    }

    // Separador duplo entre as duas secoes
    linha_sep_dupla(n);

    // ---- Secao de Tarefas ----
    std::cout << " [Tarefas]\n";
    for (int id : ids) {
        linha_sep(n);
        std::cout << "  T" << std::left << std::setw(4) << id << " |";
        for (const auto& tick : ticks) {
            const EntradaGantt* found = nullptr;
            for (const auto& e : tick.entradas)
                if (e.tarefa_id == id) { found = &e; break; }
            print_celula(found);
            std::cout << "|";
        }
        std::cout << "\n";
    }

    linha_sep(n);

    // Numeros dos ticks no rodape
    std::cout << "         ";
    for (const auto& t : ticks)
        std::cout << std::right << std::setw(W) << t.tick << " ";
    std::cout << "\n";

    // ---- Legenda ----
    std::cout << "\nLegenda:\n";

    // Cor de cada tarefa
    std::vector<int> ids_asc = ids;
    std::sort(ids_asc.begin(), ids_asc.end());
    for (int id : ids_asc) {
        std::string cor = "FFFFFF";
        for (const auto& tick : ticks) {
            for (const auto& e : tick.entradas)
                if (e.tarefa_id == id) { cor = e.cor; goto prox_tarefa; }
        }
        prox_tarefa:
        int r, g, b;
        parse_hex(cor, r, g, b);
        std::cout << "  T" << id << " = "
                  << ansi_bg(r, g, b) << FG_BRANCO << " Txx " << RESET
                  << " bloco colorido (secao CPUs) ou label (secao Tarefas)\n";
    }

    // Itens de CPU
    std::cout << "  " << ansi_bg(40,40,40) << FG_BRANCO << " OFF " << RESET
              << " = processador desligado (sem tarefas no sistema)\n";
    std::cout << "   -   = processador ocioso (ligado sem tarefa atribuida)\n";

    // Itens de tarefa
    std::cout << "   .   = tarefa aguardando CPU (fila de prontos)\n";
    std::cout << "   >>  = tarefa chegou no sistema neste tick\n";
    std::cout << "   !   = tarefa termina neste tick (ultimo ciclo)\n";
    std::cout << "   L   = tarefa adquiriu ou tentou adquirir mutex neste tick\n";
    std::cout << "   U   = tarefa liberou mutex neste tick\n";
    std::cout << "   I   = tarefa iniciou operacao de E/S neste tick (Req. 3)\n";
    std::cout << "   ?   = tarefa escolhida por sorteio aleatorio neste tick\n";
    std::cout << "  " << ansi_bg(139,0,0) << FG_BRANCO << " MUT " << RESET
              << " = tarefa suspensa aguardando mutex (Req. 2.9)\n";
    std::cout << "  " << ansi_bg(0,0,139) << FG_BRANCO << " I/O " << RESET
              << " = tarefa suspensa aguardando E/S (Req. 2.9)\n";
    std::cout << "   fim = tarefa ja finalizada\n";
    std::cout << "       = tarefa ainda nao chegou\n";
}

// ============================================================
// exportar_svg — PUBLICO (Requisito 2.4)
// Gera um arquivo SVG com o grafico de Gantt completo, incluindo
// a secao de CPUs (Requisito 1.2) e a secao de Tarefas.
// Abre em qualquer navegador sem necessidade de bibliotecas.
// ============================================================
void GanttChart::exportar_svg(const GanttLog& log, const std::string& filename) {
    const auto& ticks = log.get_ticks();
    if (ticks.empty()) {
        std::cout << "Sem dados para exportar." << std::endl;
        return;
    }

    // IDs de tarefas em ordem decrescente (Requisito 2.5)
    std::vector<int> ids;
    for (const auto& e : ticks[0].entradas) ids.push_back(e.tarefa_id);
    std::sort(ids.begin(), ids.end(), std::greater<int>());

    int n_ticks = (int)ticks.size();
    int n_tasks = (int)ids.size();
    int n_cpus  = (int)ticks[0].cpus.size();

    // Dimensoes em pixels
    const int MARGIN    = 15;
    const int LABEL_W   = 58;  // largura da coluna de labels (CPU1, T1, etc.)
    const int CELL_W    = 52;
    const int CELL_H    = 36;
    const int HDR_H     = 26;  // altura do cabecalho/rodape de ticks
    const int TITLE_H   = 38;
    const int SEC_LBL_H = 18;  // altura do label de secao ([CPUs] / [Tarefas])
    const int SEP_H     = 10;  // gap visual entre as duas secoes
    const int LEG_ITEM  = 22;

    int grid_left = MARGIN + LABEL_W;
    int grid_w    = n_ticks * CELL_W;

    // Posicoes verticais das duas secoes dentro do grid
    int grid_top      = TITLE_H + HDR_H;
    int cpu_lbl_y     = grid_top;
    int cpu_rows_y    = cpu_lbl_y  + SEC_LBL_H;
    int cpu_rows_h    = n_cpus  * CELL_H;
    int task_lbl_y    = cpu_rows_y + cpu_rows_h + SEP_H;
    int task_rows_y   = task_lbl_y + SEC_LBL_H;
    int task_rows_h   = n_tasks * CELL_H;
    int total_grid_h  = (task_rows_y - grid_top) + task_rows_h;
    int footer_num_y  = grid_top + total_grid_h + HDR_H - 5;
    int legend_top    = grid_top + total_grid_h + HDR_H + 12;
    int legend_rows   = 1 + n_tasks + 2 + 7; // titulo + cores + OFF/- + itens tarefa (incl. I de E/S)
    int svg_w         = MARGIN + LABEL_W + grid_w + MARGIN;
    int svg_h         = legend_top + legend_rows * LEG_ITEM + MARGIN;

    // Abre o arquivo (fallback para TEMP em Windows/Linux/macOS)
    std::string path_final = filename;
    std::ofstream f(path_final);
    if (!f.is_open()) {
        const char* temp_dir = std::getenv("TEMP");
        if (!temp_dir) temp_dir = std::getenv("TMP");
        if (!temp_dir) temp_dir = std::getenv("TMPDIR");
        if (temp_dir) {
            path_final = std::string(temp_dir) + "/" + filename;
            f.open(path_final);
        }
    }
    if (!f.is_open()) {
        std::cout << "Erro: nao foi possivel criar o arquivo SVG." << std::endl;
        return;
    }

    // ----- cabecalho SVG -----
    f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      << "<svg xmlns=\"http://www.w3.org/2000/svg\" "
      << "width=\"" << svg_w << "\" height=\"" << svg_h << "\">\n"
      << "<rect width=\"" << svg_w << "\" height=\"" << svg_h << "\" fill=\"white\"/>\n";

    // ----- titulo -----
    f << "<text x=\"" << MARGIN << "\" y=\"" << (TITLE_H - 8) << "\" "
      << "font-family=\"monospace\" font-size=\"17\" font-weight=\"bold\" fill=\"#222\">"
      << "Grafico de Gantt</text>\n";

    // ----- numeros de tick (cabecalho) -----
    for (int i = 0; i < n_ticks; i++) {
        int cx = grid_left + i * CELL_W + CELL_W / 2;
        f << "<text x=\"" << cx << "\" y=\"" << (TITLE_H + HDR_H - 5) << "\" "
          << "font-family=\"monospace\" font-size=\"11\" text-anchor=\"middle\" fill=\"#555\">"
          << ticks[i].tick << "</text>\n";
    }

    // =========================================================
    // Secao CPUs
    // =========================================================

    // Label da secao
    f << "<text x=\"" << MARGIN << "\" y=\"" << (cpu_lbl_y + 13) << "\" "
      << "font-family=\"monospace\" font-size=\"12\" font-weight=\"bold\" fill=\"#444\">"
      << "[CPUs]</text>\n";

    // Linhas horizontais da secao de CPUs
    for (int r = 0; r <= n_cpus; r++) {
        int y = cpu_rows_y + r * CELL_H;
        f << "<line x1=\"" << grid_left << "\" y1=\"" << y << "\" "
          << "x2=\"" << (grid_left + grid_w) << "\" y2=\"" << y << "\" "
          << "stroke=\"#bbb\" stroke-width=\"1\"/>\n";
    }

    // Linhas verticais da secao de CPUs
    for (int i = 0; i <= n_ticks; i++) {
        int x = grid_left + i * CELL_W;
        f << "<line x1=\"" << x << "\" y1=\"" << cpu_rows_y << "\" "
          << "x2=\"" << x << "\" y2=\"" << (cpu_rows_y + cpu_rows_h) << "\" "
          << "stroke=\"#bbb\" stroke-width=\"1\"/>\n";
    }

    // Celulas das CPUs
    for (int ci = 0; ci < n_cpus; ci++) {
        int row_y = cpu_rows_y + ci * CELL_H;

        // Label da CPU
        f << "<text x=\"" << (grid_left - 5) << "\" y=\"" << (row_y + CELL_H / 2 + 5) << "\" "
          << "font-family=\"monospace\" font-size=\"12\" text-anchor=\"end\" fill=\"black\">"
          << "CPU" << (ci + 1) << "</text>\n";

        for (int i = 0; i < n_ticks; i++) {
            int cx = grid_left + i * CELL_W;
            int cy = row_y;

            const EntradaCPU* ec = nullptr;
            for (const auto& en : ticks[i].cpus)
                if (en.cpu_id == ci) { ec = &en; break; }

            if (!ec) continue;

            std::string fill     = "white";
            std::string text_col = "#444";
            std::string label;

            switch (ec->estado) {
                case EstadoCPU::EXECUTANDO:
                    fill     = "#" + ec->cor;
                    text_col = "white";
                    label    = "T" + std::to_string(ec->tarefa_id);
                    break;
                case EstadoCPU::OCIOSA:
                    fill  = "#f0f0f0";
                    label = "-";
                    break;
                case EstadoCPU::DESLIGADA:
                    fill     = "#2a2a2a";
                    text_col = "white";
                    label    = "OFF";
                    break;
            }

            if (fill != "white") {
                f << "<rect x=\"" << cx << "\" y=\"" << cy << "\" "
                  << "width=\"" << CELL_W << "\" height=\"" << CELL_H << "\" "
                  << "fill=\"" << fill << "\"/>\n";
            }
            if (!label.empty()) {
                f << "<text x=\"" << (cx + CELL_W / 2) << "\" y=\"" << (cy + CELL_H / 2 + 5) << "\" "
                  << "font-family=\"monospace\" font-size=\"12\" font-weight=\"bold\" "
                  << "text-anchor=\"middle\" fill=\"" << text_col << "\">"
                  << label << "</text>\n";
            }
        }
    }

    // Linha separadora grossa entre as secoes
    int sep_line_y = cpu_rows_y + cpu_rows_h + SEP_H / 2;
    f << "<line x1=\"" << (MARGIN) << "\" y1=\"" << sep_line_y << "\" "
      << "x2=\"" << (grid_left + grid_w) << "\" y2=\"" << sep_line_y << "\" "
      << "stroke=\"#666\" stroke-width=\"2\" stroke-dasharray=\"4,2\"/>\n";

    // =========================================================
    // Secao Tarefas
    // =========================================================

    // Label da secao
    f << "<text x=\"" << MARGIN << "\" y=\"" << (task_lbl_y + 13) << "\" "
      << "font-family=\"monospace\" font-size=\"12\" font-weight=\"bold\" fill=\"#444\">"
      << "[Tarefas]</text>\n";

    // Linhas horizontais da secao de tarefas
    for (int r = 0; r <= n_tasks; r++) {
        int y = task_rows_y + r * CELL_H;
        f << "<line x1=\"" << grid_left << "\" y1=\"" << y << "\" "
          << "x2=\"" << (grid_left + grid_w) << "\" y2=\"" << y << "\" "
          << "stroke=\"#bbb\" stroke-width=\"1\"/>\n";
    }

    // Linhas verticais da secao de tarefas
    for (int i = 0; i <= n_ticks; i++) {
        int x = grid_left + i * CELL_W;
        f << "<line x1=\"" << x << "\" y1=\"" << task_rows_y << "\" "
          << "x2=\"" << x << "\" y2=\"" << (task_rows_y + task_rows_h) << "\" "
          << "stroke=\"#bbb\" stroke-width=\"1\"/>\n";
    }

    // Celulas das tarefas
    for (int r = 0; r < n_tasks; r++) {
        int id    = ids[r];
        int row_y = task_rows_y + r * CELL_H;

        // Label da tarefa
        f << "<text x=\"" << (grid_left - 5) << "\" y=\"" << (row_y + CELL_H / 2 + 5) << "\" "
          << "font-family=\"monospace\" font-size=\"13\" text-anchor=\"end\" fill=\"black\">"
          << "T" << id << "</text>\n";

        for (int i = 0; i < n_ticks; i++) {
            int cx = grid_left + i * CELL_W;
            int cy = row_y;

            const EntradaGantt* e = nullptr;
            for (const auto& en : ticks[i].entradas)
                if (en.tarefa_id == id) { e = &en; break; }

            std::string fill     = "white";
            std::string text_col = "#444";
            std::string label;

            if (e) {
                switch (e->tipo) {
                    case TipoGantt::EXECUTANDO:
                        fill = "#" + e->cor;
                        text_col = "white";
                        if (e->evento_chegada && e->evento_termino)
                            label = ">C" + std::to_string(e->cpu_id + 1) + "!";
                        else if (e->evento_chegada)
                            label = ">C" + std::to_string(e->cpu_id + 1);
                        else if (e->evento_termino)
                            label = "C" + std::to_string(e->cpu_id + 1) + "!";
                        else
                            label = "CPU" + std::to_string(e->cpu_id + 1);
                        break;

                    case TipoGantt::PRONTA:
                        fill = "#f5f5f5"; text_col = "#666";
                        label = e->evento_chegada ? ">>" : ".";
                        break;
                    case TipoGantt::SUSPENSA_MUTEX:
                        // Vermelho escuro — bloqueada por mutex (Req. 2.9)
                        fill = "#8B0000"; text_col = "white";
                        label = e->evento_mutex_lock ? "MLk" : "MUT";
                        break;
                    case TipoGantt::SUSPENSA_IO:
                        // Azul escuro — bloqueada por E/S (Req. 2.9)
                        fill = "#00008B"; text_col = "white"; label = "I/O"; break;
                    case TipoGantt::FINALIZADA:
                        fill = "#ebebeb"; text_col = "#aaa";  label = "fim"; break;
                    default: break;
                }
            }

            if (fill != "white") {
                f << "<rect x=\"" << cx << "\" y=\"" << cy << "\" "
                  << "width=\"" << CELL_W << "\" height=\"" << CELL_H << "\" "
                  << "fill=\"" << fill << "\"/>\n";
            }
            if (!label.empty()) {
                f << "<text x=\"" << (cx + CELL_W / 2) << "\" y=\"" << (cy + CELL_H / 2 + 5) << "\" "
                  << "font-family=\"monospace\" font-size=\"11\" text-anchor=\"middle\" "
                  << "fill=\"" << text_col << "\">" << label << "</text>\n";
            }

            // Marcadores no canto superior direito (Req. 2.8 e 3)
            if (e && (e->evento_mutex_lock || e->evento_mutex_unlock)) {
                std::string mc = (fill == "white" || fill == "#f5f5f5" || fill == "#ebebeb")
                                 ? "#8B0000" : "white";
                std::string ml = e->evento_mutex_lock ? "L" : "U";
                f << "<text x=\"" << (cx + CELL_W - 3) << "\" y=\"" << (cy + 10) << "\" "
                  << "font-family=\"monospace\" font-size=\"10\" font-weight=\"bold\" "
                  << "text-anchor=\"end\" fill=\"" << mc << "\">" << ml << "</text>\n";
            }
            // E/S iniciada: pequeno 'I' no canto superior direito (Req. 3)
            else if (e && e->evento_io) {
                f << "<text x=\"" << (cx + CELL_W - 3) << "\" y=\"" << (cy + 10) << "\" "
                  << "font-family=\"monospace\" font-size=\"10\" font-weight=\"bold\" "
                  << "text-anchor=\"end\" fill=\"white\">I</text>\n";
            }
            // Sorteio: pequeno "?" no canto superior direito (Requisito 4.3)
            else if (e && e->evento_sorteio) {
                std::string sq = (fill == "white" || fill == "#f5f5f5" || fill == "#ebebeb")
                                 ? "#cc4400" : "white";
                f << "<text x=\"" << (cx + CELL_W - 3) << "\" y=\"" << (cy + 10) << "\" "
                  << "font-family=\"monospace\" font-size=\"10\" font-weight=\"bold\" "
                  << "text-anchor=\"end\" fill=\"" << sq << "\">?</text>\n";
            }
        }
    }

    // ----- numeros de tick (rodape) -----
    for (int i = 0; i < n_ticks; i++) {
        int cx = grid_left + i * CELL_W + CELL_W / 2;
        f << "<text x=\"" << cx << "\" y=\"" << footer_num_y << "\" "
          << "font-family=\"monospace\" font-size=\"11\" text-anchor=\"middle\" fill=\"#555\">"
          << ticks[i].tick << "</text>\n";
    }

    // ----- legenda -----
    int ly = legend_top;
    f << "<text x=\"" << MARGIN << "\" y=\"" << ly << "\" "
      << "font-family=\"monospace\" font-size=\"13\" font-weight=\"bold\" fill=\"black\">Legenda:</text>\n";
    ly += LEG_ITEM;

    auto leg = [&](const std::string& fill, const std::string& stroke,
                   const std::string& inner, const std::string& desc) {
        f << "<rect x=\"" << MARGIN << "\" y=\"" << (ly - 14) << "\" "
          << "width=\"32\" height=\"17\" fill=\"" << fill << "\" stroke=\"" << stroke << "\"/>\n";
        if (!inner.empty()) {
            f << "<text x=\"" << (MARGIN + 16) << "\" y=\"" << ly << "\" "
              << "font-family=\"monospace\" font-size=\"10\" text-anchor=\"middle\" fill=\"#ddd\">"
              << inner << "</text>\n";
        }
        f << "<text x=\"" << (MARGIN + 38) << "\" y=\"" << ly << "\" "
          << "font-family=\"monospace\" font-size=\"12\" fill=\"black\">" << desc << "</text>\n";
        ly += LEG_ITEM;
    };

    // Cores das tarefas (ordem crescente)
    std::vector<int> ids_asc = ids;
    std::sort(ids_asc.begin(), ids_asc.end());
    for (int id : ids_asc) {
        std::string cor = "CCCCCC";
        bool achou = false;
        for (const auto& tick : ticks) {
            if (achou) break;
            for (const auto& e : tick.entradas)
                if (e.tarefa_id == id) { cor = e.cor; achou = true; break; }
        }
        f << "<rect x=\"" << MARGIN << "\" y=\"" << (ly - 14) << "\" "
          << "width=\"32\" height=\"17\" fill=\"#" << cor << "\" stroke=\"#999\"/>\n";
        f << "<text x=\"" << (MARGIN + 38) << "\" y=\"" << ly << "\" "
          << "font-family=\"monospace\" font-size=\"12\" fill=\"black\">"
          << "T" << id << " = cor da tarefa na secao CPUs e Tarefas</text>\n";
        ly += LEG_ITEM;
    }

    // Itens de CPU
    leg("#2a2a2a", "#111", "OFF", "processador desligado (sem tarefas no sistema)");
    leg("#f0f0f0", "#bbb", "-",   "processador ocioso (ligado sem tarefa atribuida)");

    // Itens de tarefa
    leg("#f5f5f5", "#bbb", ".",    "tarefa aguardando CPU (fila de prontos)");
    leg("#f5f5f5", "#bbb", ">>",   "tarefa chegou neste tick");
    leg("white",   "#bbb", "!",    "tarefa termina neste tick (ultimo ciclo)");
    leg("white",   "#bbb", "L",    "tarefa adquiriu/tentou mutex neste tick (Req. 2.8)");
    leg("white",   "#bbb", "U",    "tarefa liberou mutex neste tick (Req. 2.8)");
    leg("#00008B", "#111", "I",    "tarefa iniciou operacao de E/S neste tick (Req. 3)");
    leg("white",   "#bbb", "?",    "tarefa escolhida por sorteio aleatorio");
    leg("#8B0000", "#111", "MUT",  "tarefa suspensa aguardando mutex (Req. 2.9)");
    leg("#00008B", "#111", "I/O",  "tarefa suspensa aguardando E/S (Req. 2.9)");
    leg("#ebebeb", "#bbb", "fim",  "tarefa ja finalizada");

    f << "</svg>\n";
    f.close();

    std::cout << "Grafico exportado para: " << path_final
              << "  (abra no navegador)" << std::endl;
}
