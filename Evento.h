#pragma once
#include <string>
#include <vector>

// Tipos de evento que podem ocorrer durante a execucao de uma tarefa (Projeto B).
enum class TipoEvento {
    MUTEX_LOCK,   // MLxx:tt   — solicita o mutex xx no instante relativo tt (Req. 2)
    MUTEX_UNLOCK, // MUxx:tt   — libera   o mutex xx no instante relativo tt (Req. 2)
    IO_INICIO     // IO:tt-dd  — inicia operacao de E/S no instante relativo tt com duracao dd (Req. 3)
};

// Evento — uma acao agendada para disparar quando a tarefa atingir
// `tempo_relativo` ticks de CPU executados desde o inicio da sua execucao.
// Instantes sempre relativos ao inicio da tarefa (Req. 2.4 e 3.3).
struct Evento {
    TipoEvento tipo;
    int tempo_relativo; // valor de ticks_executados em que o evento dispara
    int mutex_id;       // numero do mutex (ML e MU); 0 para IO_INICIO
    int io_duracao;     // duracao da operacao de E/S em ticks (Req. 3.2); 0 para ML/MU
};

// parse_eventos — converte a string lista_eventos do arquivo de configuracao
// em um vetor de Evento ordenado por tempo_relativo (ordem de arquivo mantida
// para eventos no mesmo instante — Req. 2.5 e 3.5).
// Formatos aceitos por token (separados por virgula ou espaco):
//   MLxx:tt   (mutex lock)
//   MUxx:tt   (mutex unlock)
//   IO:tt-dd  (E/S: instante relativo tt, duracao dd)
// Comparacao case-insensitive (Req. 3.3.2).
std::vector<Evento> parse_eventos(const std::string& lista);
