#include "Evento.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>

// ============================================================
// parse_eventos — PUBLICO (Req. 2.1 e 3.1)
// Converte a string "lista_eventos" lida do arquivo de configuracao
// em um vetor de Evento ordenado por tempo_relativo.
//
// Formatos aceitos por token (separados por virgula ou espaco):
//   MLxx:tt   — mutex lock:   solicitar mutex xx no instante relativo tt
//   MUxx:tt   — mutex unlock: liberar   mutex xx no instante relativo tt
//   IO:tt-dd  — E/S: iniciar operacao no instante relativo tt com duracao dd ticks
//
// Comparacao case-insensitive (Req. 3.3.2).
// ============================================================
std::vector<Evento> parse_eventos(const std::string& lista) {
    std::vector<Evento> resultado;
    if (lista.empty()) return resultado;

    // Normaliza: troca espacos por virgulas para usar um unico separador
    std::string norm = lista;
    std::replace(norm.begin(), norm.end(), ' ', ',');

    std::stringstream ss(norm);
    std::string token;

    while (std::getline(ss, token, ',')) {
        if (token.empty()) continue;

        // Converte para maiusculas para comparacao case-insensitive
        std::string up = token;
        std::transform(up.begin(), up.end(), up.begin(),
            [](unsigned char c) { return std::toupper(c); });

        // ---- Token IO:tt-dd (Req. 3) ----
        if (up.size() >= 2 && up.substr(0, 2) == "IO") {
            size_t sep = up.find(':');
            if (sep == std::string::npos) {
                std::cout << "[Config] Aviso: evento IO sem ':' ignorado: " << token << std::endl;
                continue;
            }
            std::string resto = up.substr(sep + 1); // "tt-dd"
            size_t dash = resto.find('-');
            if (dash == std::string::npos) {
                std::cout << "[Config] Aviso: evento IO sem '-' na duracao ignorado: " << token << std::endl;
                continue;
            }
            try {
                int tempo   = std::stoi(resto.substr(0, dash));
                int duracao = std::stoi(resto.substr(dash + 1));
                resultado.push_back({TipoEvento::IO_INICIO, tempo, 0, duracao});
            } catch (...) {
                std::cout << "[Config] Aviso: evento IO com campos invalidos ignorado: "
                          << token << std::endl;
            }
            continue;
        }

        // ---- Tokens MLxx:tt e MUxx:tt (Req. 2) ----
        TipoEvento tipo;
        if      (up.size() >= 2 && up.substr(0, 2) == "ML") tipo = TipoEvento::MUTEX_LOCK;
        else if (up.size() >= 2 && up.substr(0, 2) == "MU") tipo = TipoEvento::MUTEX_UNLOCK;
        else {
            std::cout << "[Config] Aviso: tipo de evento desconhecido ignorado: "
                      << token << std::endl;
            continue;
        }

        // Localiza o ':' que separa mutex_id de tempo_relativo
        size_t sep = up.find(':');
        if (sep == std::string::npos || sep < 2) {
            std::cout << "[Config] Aviso: evento sem ':' ignorado: " << token << std::endl;
            continue;
        }

        try {
            int mutex_id = std::stoi(up.substr(2, sep - 2));
            int tempo    = std::stoi(up.substr(sep + 1));
            resultado.push_back({tipo, tempo, mutex_id, 0});
        } catch (...) {
            std::cout << "[Config] Aviso: evento com campos invalidos ignorado: "
                      << token << std::endl;
        }
    }

    // Ordena por tempo_relativo para que o simulador processe em ordem crescente.
    // Eventos com mesmo tempo_relativo mantem a ordem de insercao (estavel) — Req. 2.5.
    std::stable_sort(resultado.begin(), resultado.end(),
        [](const Evento& a, const Evento& b) {
            return a.tempo_relativo < b.tempo_relativo;
        });

    return resultado;
}
