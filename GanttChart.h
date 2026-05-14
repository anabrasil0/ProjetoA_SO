#pragma once
#include "GanttLog.h"
#include <string>

// ============================================================
// GanttChart — renderiza o GanttLog no terminal e em arquivo SVG
// ============================================================
class GanttChart {
public:
    // Exibe o grafico no terminal para todos os ticks registrados ate o momento
    static void exibir_terminal(const GanttLog& log);

    // Exporta o grafico completo como arquivo SVG (Requisito 2.4)
    // Abre em qualquer navegador sem bibliotecas externas.
    static void exportar_svg(const GanttLog& log, const std::string& filename = "gantt.svg");
};
