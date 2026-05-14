#include "GanttLog.h"
#include <algorithm>

// ============================================================
// registrar — PUBLICO
// Armazena o snapshot de um tick no historico do Gantt.
// Chamado por Simulador::do_tick() apos o escalonamento e antes
// de processar_ciclo(), capturando o estado "em execucao" correto
// para aquele tick antes de qualquer alteracao pela CPU.
// ============================================================
void GanttLog::registrar(const TickGantt& t) {
    ticks.push_back(t); // adiciona o snapshot ao final do historico
}

// ============================================================
// truncar_apos — PUBLICO
// Remove do historico todos os ticks com numero >= ao valor dado.
// Chamado por Simulador::step_backward() para apagar os snapshots
// de ticks que "deixaram de existir" apos um retrocesso, mantendo
// o Gantt sincronizado com o estado atual da simulacao.
// ============================================================
void GanttLog::truncar_apos(int tick) {
    // Usa o padrao erase-remove: remove_if desloca os elementos indesejados
    // para o final do vetor; erase os deleta de fato.
    ticks.erase(
        std::remove_if(ticks.begin(), ticks.end(),
            // Predicado: tick >= valor dado deve ser removido
            [tick](const TickGantt& t) { return t.tick >= tick; }),
        ticks.end()
    );
}

// ============================================================
// get_ticks — PUBLICO
// Retorna o historico completo de snapshots por referencia constante,
// para que GanttChart possa ler sem copiar o vetor inteiro.
// ============================================================
const std::vector<TickGantt>& GanttLog::get_ticks() const {
    return ticks;
}
