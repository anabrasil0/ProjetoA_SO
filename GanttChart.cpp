#include "GanttChart.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <string>
#include <vector>

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
// Imprime o conteudo de uma celula com a cor/simbolo corretos
// Eventos de chegada e termino recebem marcadores visuais (Requisito 2.2)
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
                // Chegada: seta indicando entrada da tarefa
                label = ">C" + std::to_string(e->cpu_id + 1) + "  ";
            } else if (e->evento_termino) {
                // Termino: exclamacao indicando fim da tarefa
                label = "C" + std::to_string(e->cpu_id + 1) + "! ";
                if (label.size() < (size_t)W) label += " ";
            } else {
                label = "CPU" + std::to_string(e->cpu_id + 1) + " ";
            }

            label = label.substr(0, W);
            std::cout << ansi_bg(r, g, b) << FG_BRANCO << label << RESET;
            break;
        }

        case TipoGantt::PRONTA:
            if (e->evento_chegada) {
                // Chegada na fila: seta de entrada bem visivel
                std::cout << " >>  ";
            } else {
                std::cout << "  .  ";
            }
            break;

        case TipoGantt::SUSPENSA:
            std::cout << ansi_bg(0, 0, 0) << FG_BRANCO << " SUS " << RESET;
            break;

        case TipoGantt::FINALIZADA:
            std::cout << " fim ";
            break;

        default:
            std::cout << std::string(W, ' ');
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

// ============================================================
// exibir_terminal — PUBLICO (Requisito 2.1 e 2.3)
// ============================================================
void GanttChart::exibir_terminal(const GanttLog& log) {
    const auto& ticks = log.get_ticks();
    if (ticks.empty()) return;

    // IDs em ordem decrescente: maior ID no topo (Requisito 2.5)
    std::vector<int> ids;
    for (const auto& e : ticks[0].entradas) ids.push_back(e.tarefa_id);
    std::sort(ids.begin(), ids.end(), std::greater<int>());

    int n = (int)ticks.size();

    std::cout << "\n=== GANTT ===\n";

    // Numeros dos ticks no cabecalho
    std::cout << "         ";
    for (const auto& t : ticks) {
        std::cout << std::right << std::setw(W) << t.tick << " ";
    }
    std::cout << "\n";

    // Uma linha por tarefa, separadas por bordas
    for (int id : ids) {
        linha_sep(n);

        // Label da tarefa
        std::cout << "  T" << std::left << std::setw(4) << id << " |";

        // Celulas do tick
        for (const auto& tick : ticks) {
            const EntradaGantt* found = nullptr;
            for (const auto& e : tick.entradas) {
                if (e.tarefa_id == id) { found = &e; break; }
            }
            print_celula(found);
            std::cout << "|";
        }
        std::cout << "\n";
    }

    linha_sep(n);

    // Numeros dos ticks no rodape
    std::cout << "         ";
    for (const auto& t : ticks) {
        std::cout << std::right << std::setw(W) << t.tick << " ";
    }
    std::cout << "\n";

    // --------------------------------------------------------
    // Legenda com blocos coloridos reais (sem hexadecimal)
    // --------------------------------------------------------
    std::cout << "\nLegenda:\n";

    // Cor de cada tarefa
    std::vector<int> ids_asc = ids;
    std::sort(ids_asc.begin(), ids_asc.end());
    for (int id : ids_asc) {
        std::string cor = "FFFFFF";
        for (const auto& tick : ticks) {
            for (const auto& e : tick.entradas) {
                if (e.tarefa_id == id) { cor = e.cor; break; }
            }
            break;
        }
        int r, g, b;
        parse_hex(cor, r, g, b);
        std::cout << "  T" << id << " = "
                  << ansi_bg(r, g, b) << FG_BRANCO << " CPU1 " << RESET
                  << " bloco colorido indica qual processador esta executando a tarefa\n";
    }

    std::cout << "   .   = aguardando CPU (fila de prontos)\n";
    std::cout << "   >>  = tarefa chegou no sistema neste tick\n";
    std::cout << "   !   = tarefa termina neste tick (ultimo ciclo)\n";
    std::cout << "  " << ansi_bg(0, 0, 0) << FG_BRANCO << " SUS " << RESET
              << " = tarefa suspensa (bloqueada)\n";
    std::cout << "   fim = tarefa ja finalizada\n";
    std::cout << "       = tarefa ainda nao chegou\n";
}

// ============================================================
// exportar_svg — PUBLICO (Requisito 2.4)
// Gera um arquivo SVG com o grafico de Gantt completo.
// Abre em qualquer navegador sem necessidade de bibliotecas.
// ============================================================
void GanttChart::exportar_svg(const GanttLog& log, const std::string& filename) {
    const auto& ticks = log.get_ticks();
    if (ticks.empty()) {
        std::cout << "Sem dados para exportar." << std::endl;
        return;
    }

    // IDs em ordem decrescente (maior no topo — Requisito 2.5)
    std::vector<int> ids;
    for (const auto& e : ticks[0].entradas) ids.push_back(e.tarefa_id);
    std::sort(ids.begin(), ids.end(), std::greater<int>());

    int n_ticks = (int)ticks.size();
    int n_tasks = (int)ids.size();

    // Dimensoes em pixels
    const int MARGIN   = 15;
    const int LABEL_W  = 55;
    const int CELL_W   = 52;
    const int CELL_H   = 36;
    const int HDR_H    = 26;
    const int TITLE_H  = 38;
    const int LEG_ITEM = 22;

    int grid_left   = MARGIN + LABEL_W;
    int grid_top    = TITLE_H + HDR_H;
    int grid_w      = n_ticks * CELL_W;
    int grid_h      = n_tasks * CELL_H;
    int legend_top  = grid_top + grid_h + HDR_H + 12;
    int legend_rows = n_tasks + 6; // cores das tarefas + 5 itens fixos + titulo
    int svg_w       = MARGIN + LABEL_W + grid_w + MARGIN;
    int svg_h       = legend_top + legend_rows * LEG_ITEM + MARGIN;

    std::ofstream f(filename);
    if (!f.is_open()) {
        std::cout << "Erro: nao foi possivel criar o arquivo " << filename << std::endl;
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
        int cy = TITLE_H + HDR_H - 5;
        f << "<text x=\"" << cx << "\" y=\"" << cy << "\" "
          << "font-family=\"monospace\" font-size=\"11\" text-anchor=\"middle\" fill=\"#555\">"
          << ticks[i].tick << "</text>\n";
    }

    // ----- linhas de separacao verticais (colunas) -----
    for (int i = 0; i <= n_ticks; i++) {
        int x = grid_left + i * CELL_W;
        f << "<line x1=\"" << x << "\" y1=\"" << grid_top << "\" "
          << "x2=\"" << x << "\" y2=\"" << (grid_top + grid_h) << "\" "
          << "stroke=\"#bbb\" stroke-width=\"1\"/>\n";
    }

    // ----- linhas de separacao horizontais (linhas de tarefa) -----
    for (int r = 0; r <= n_tasks; r++) {
        int y = grid_top + r * CELL_H;
        f << "<line x1=\"" << grid_left << "\" y1=\"" << y << "\" "
          << "x2=\"" << (grid_left + grid_w) << "\" y2=\"" << y << "\" "
          << "stroke=\"#bbb\" stroke-width=\"1\"/>\n";
    }

    // ----- celulas e labels de tarefa -----
    for (int r = 0; r < n_tasks; r++) {
        int id  = ids[r];
        int row_y = grid_top + r * CELL_H;

        // label da tarefa
        f << "<text x=\"" << (grid_left - 5) << "\" y=\"" << (row_y + CELL_H / 2 + 5) << "\" "
          << "font-family=\"monospace\" font-size=\"13\" text-anchor=\"end\" fill=\"black\">"
          << "T" << id << "</text>\n";

        for (int i = 0; i < n_ticks; i++) {
            int cx = grid_left + i * CELL_W;
            int cy = row_y;

            const EntradaGantt* e = nullptr;
            for (const auto& en : ticks[i].entradas) {
                if (en.tarefa_id == id) { e = &en; break; }
            }

            std::string fill = "white";
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
                        fill = "#f5f5f5";
                        text_col = "#666";
                        label = e->evento_chegada ? ">>" : ".";
                        break;
                    case TipoGantt::SUSPENSA:
                        fill = "#111111";
                        text_col = "white";
                        label = "SUS";
                        break;
                    case TipoGantt::FINALIZADA:
                        fill = "#ebebeb";
                        text_col = "#aaa";
                        label = "fim";
                        break;
                    default:
                        break;
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
        }
    }

    // ----- numeros de tick (rodape) -----
    int footer_y = grid_top + grid_h + HDR_H - 5;
    for (int i = 0; i < n_ticks; i++) {
        int cx = grid_left + i * CELL_W + CELL_W / 2;
        f << "<text x=\"" << cx << "\" y=\"" << footer_y << "\" "
          << "font-family=\"monospace\" font-size=\"11\" text-anchor=\"middle\" fill=\"#555\">"
          << ticks[i].tick << "</text>\n";
    }

    // ----- legenda -----
    int ly = legend_top;
    f << "<text x=\"" << MARGIN << "\" y=\"" << ly << "\" "
      << "font-family=\"monospace\" font-size=\"13\" font-weight=\"bold\" fill=\"black\">Legenda:</text>\n";
    ly += LEG_ITEM;

    // cores das tarefas (ordem crescente)
    std::vector<int> ids_asc = ids;
    std::sort(ids_asc.begin(), ids_asc.end());
    for (int id : ids_asc) {
        std::string cor = "CCCCCC";
        bool achou = false;
        for (const auto& tick : ticks) {
            if (achou) break;
            for (const auto& e : tick.entradas) {
                if (e.tarefa_id == id) { cor = e.cor; achou = true; break; }
            }
        }
        f << "<rect x=\"" << MARGIN << "\" y=\"" << (ly - 14) << "\" "
          << "width=\"32\" height=\"17\" fill=\"#" << cor << "\" stroke=\"#999\"/>\n";
        f << "<text x=\"" << (MARGIN + 38) << "\" y=\"" << ly << "\" "
          << "font-family=\"monospace\" font-size=\"12\" fill=\"black\">"
          << "T" << id << " = bloco colorido indica qual processador executa a tarefa</text>\n";
        ly += LEG_ITEM;
    }

    // itens fixos
    auto leg = [&](const std::string& fill, const std::string& stroke,
                    const std::string& inner, const std::string& desc) {
        f << "<rect x=\"" << MARGIN << "\" y=\"" << (ly - 14) << "\" "
          << "width=\"32\" height=\"17\" fill=\"" << fill << "\" stroke=\"" << stroke << "\"/>\n";
        if (!inner.empty()) {
            f << "<text x=\"" << (MARGIN + 16) << "\" y=\"" << ly << "\" "
              << "font-family=\"monospace\" font-size=\"10\" text-anchor=\"middle\" fill=\"#666\">"
              << inner << "</text>\n";
        }
        f << "<text x=\"" << (MARGIN + 38) << "\" y=\"" << ly << "\" "
          << "font-family=\"monospace\" font-size=\"12\" fill=\"black\">" << desc << "</text>\n";
        ly += LEG_ITEM;
    };

    leg("#f5f5f5", "#bbb", ".",   "aguardando CPU (fila de prontos)");
    leg("#f5f5f5", "#bbb", ">>",  "tarefa chegou neste tick");
    leg("white",   "#bbb", "!",   "tarefa termina neste tick (ultimo ciclo)");
    leg("#111",    "#111", "SUS", "tarefa suspensa (bloqueada)");
    leg("#ebebeb", "#bbb", "fim", "tarefa ja finalizada");

    f << "</svg>\n";
    f.close();

    std::cout << "Grafico exportado para: " << filename
              << "  (abra no navegador)" << std::endl;
}
