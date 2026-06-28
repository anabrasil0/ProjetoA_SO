#include "Task.h"

// ============================================================
// Construtor
// Inicializa o TCB com os dados do arquivo de configuracao.
// O tempo_restante parte igual a duracao e diminui a cada tick executado.
// A string lista_eventos e parseada em `eventos` pelo parse_eventos() — Req. 2.1.
// Estado inicial CRIADA: a transicao para PRONTA ocorre no tick de ingresso.
// ============================================================
Task::Task(int id, int ingresso, int duracao, int prioridade,
           const std::string& cor, const std::string& lista_eventos)
    : id(id),
      tempo_ingresso(ingresso),
      duracao(duracao),
      tempo_restante(duracao),
      prioridade(prioridade),
      estado(CRIADA),
      cor(cor),
      idade(0),
      eventos(parse_eventos(lista_eventos)), // converte string em vetor de Evento
      ticks_executados(0),                  // nenhum tick de CPU consumido ainda
      indice_proximo_evento(0),             // aponta para o primeiro evento
      mutex_aguardando_id(-1),              // -1 = nao aguardando nenhum mutex
      io_tick_conclusao(-1)                 // -1 = nao em operacao de E/S (Req. 3)
{}

// ============================================================
// Getters — retornam os atributos do TCB sem permitir alteracao
// externa direta. Todos sao const porque nao modificam o objeto.
// ============================================================

// Identificador unico da tarefa, definido no arquivo de config
int Task::get_id() const {
    return id;
}

// Tick em que a tarefa deve chegar ao sistema e entrar na fila de prontos
int Task::get_tempo_ingresso() const {
    return tempo_ingresso;
}

// Duracao total original da tarefa (nunca muda apos criacao)
int Task::get_duracao() const {
    return duracao;
}

// Tempo que ainda falta para a tarefa terminar (decrementado pela CPU a cada tick)
int Task::get_tempo_restante() const {
    return tempo_restante;
}

// Prioridade estatica: valor maior = maior prioridade no escalonador PRIOP
int Task::get_prioridade() const {
    return prioridade;
}

// Estado atual no ciclo de vida: CRIADA -> PRONTA -> EXECUTANDO -> FINALIZADA
Estado Task::get_estado() const {
    return estado;
}

// Cor em hexadecimal RGB (ex: "FF0000" = vermelho) usada para pintar o bloco no Gantt
std::string Task::get_cor() const {
    return cor;
}

// ============================================================
// Setters — usados em dois contextos:
//   1. set_tempo_restante: chamado por CPU::processar_ciclo a cada tick
//      para decrementar o progresso da tarefa.
//   2. set_estado: chamado pelo scheduler (PRONTA/EXECUTANDO), pela CPU
//      (FINALIZADA) e pelo Simulador em step_backward para restaurar
//      o estado de um tick anterior (Requisito 1.5.2).
// ============================================================

// Atualiza o tempo restante; normalmente chamado com (tempo_restante - 1) por processar_ciclo
void Task::set_tempo_restante(int t) {
    tempo_restante = t;
}

// Atualiza o estado no ciclo de vida da tarefa
void Task::set_estado(Estado e) {
    estado = e;
}

// Retorna a quantidade de ticks acumulados na fila de prontos sem executar
int Task::get_idade() const { return idade; }
void Task::set_idade(int i)  { idade = i; }

void Task::incrementar_idade(int alpha) { idade += alpha; }
void Task::resetar_idade()              { idade = 0; }

// ============================================================
// Getters dos campos de eventos e mutex (Req. 2)
// ============================================================
int Task::get_ticks_executados()       const { return ticks_executados; }
int Task::get_indice_proximo_evento()  const { return indice_proximo_evento; }
int Task::get_mutex_aguardando_id()    const { return mutex_aguardando_id; }
const std::vector<Evento>& Task::get_eventos() const { return eventos; }

// ============================================================
// Setters para step_backward
// ============================================================
void Task::set_ticks_executados(int t)      { ticks_executados     = t; }
void Task::set_indice_proximo_evento(int i) { indice_proximo_evento = i; }
void Task::set_mutex_aguardando_id(int id)  { mutex_aguardando_id  = id; }
int  Task::get_io_tick_conclusao()    const { return io_tick_conclusao; }
void Task::set_io_tick_conclusao(int t)     { io_tick_conclusao    = t; }

// ============================================================
// incrementar_ticks_executados — chamado por CPU::processar_ciclo()
// Contabiliza mais um tick de CPU consumido por esta tarefa.
// Serve de base para determinar quando disparar eventos (Req. 2.4):
// o evento com tempo_relativo == N dispara quando ticks_executados == N.
// ============================================================
void Task::incrementar_ticks_executados() {
    ticks_executados++;
}

// ============================================================
// avancar_indice_evento — marca o evento atual como processado
// Chamado pelo Simulador logo apos executar o evento apontado
// por indice_proximo_evento, avancando para o proximo.
// ============================================================
void Task::avancar_indice_evento() {
    indice_proximo_evento++;
}
