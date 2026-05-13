#include "Config.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

// Valores padrão caso algum campo esteja ausente no arquivo (Requisito 3.2)
static const std::string ALGORITMO_PADRAO = "PRIOP";
static const int         QUANTUM_PADRAO   = 2;
static const int         CPUS_PADRAO      = 2;

// ============================================================
// Construtor
// ============================================================
Config::Config(const std::string& caminho_arquivo) {
    // Inicializa com valores padrão antes de ler o arquivo (Requisito 3.2)
    algoritmo = ALGORITMO_PADRAO;
    quantum   = QUANTUM_PADRAO;
    qtde_cpus = CPUS_PADRAO;

    parse(caminho_arquivo);
}

// ============================================================
// parse — PRIVADO
// Lê o arquivo linha a linha e preenche os atributos da classe.
// Formato esperado (Requisito 3.3):
//   Linha 1: algoritmo;quantum;qtde_cpus
//   Linha 2+: id;cor;ingresso;duracao;prioridade;lista_eventos
// ============================================================
void Config::parse(const std::string& caminho_arquivo) {
    std::ifstream arquivo(caminho_arquivo);

    if (!arquivo.is_open()) {
        std::cout << "Erro: nao foi possivel abrir o arquivo: " << caminho_arquivo << std::endl;
        std::cout << "Usando valores padrao." << std::endl;
        return;
    }

    std::string linha;
    bool primeira_linha = true;

    while (std::getline(arquivo, linha)) {
        // Remove BOM UTF-8 (0xEF 0xBB 0xBF) se presente no inicio da primeira linha.
        // Editores e o PowerShell frequentemente inserem o BOM em arquivos UTF-8,
        // o que corromperia a leitura do primeiro campo sem este tratamento.
        if (linha.size() >= 3 &&
            (unsigned char)linha[0] == 0xEF &&
            (unsigned char)linha[1] == 0xBB &&
            (unsigned char)linha[2] == 0xBF) {
            linha.erase(0, 3);
        }

        // Remove '\r' final (arquivos com quebra de linha Windows \r\n lidos em modo texto)
        if (!linha.empty() && linha.back() == '\r') linha.pop_back();

        // Ignora linhas vazias
        if (linha.empty()) continue;

        std::vector<std::string> campos = split(linha, ';');

        if (primeira_linha) {
            // Linha 1: parâmetros gerais do sistema
            if (campos.size() >= 1) algoritmo = para_maiusculas(campos[0]);
            if (campos.size() >= 2) quantum   = std::stoi(campos[1]);
            if (campos.size() >= 3) qtde_cpus = std::stoi(campos[2]);
            primeira_linha = false;
        } 
        else {
            // Demais linhas: uma tarefa por linha
            if (campos.size() < 5) {
                std::cout << "Aviso: linha de tarefa ignorada por ter menos de 5 campos: "
                          << linha << std::endl;
                continue;
            }

            TaskData td;
            td.id = std::stoi(campos[0]);
            td.cor = campos[1];
            td.ingresso = std::stoi(campos[2]);
            td.duracao = std::stoi(campos[3]);
            td.prioridade = std::stoi(campos[4]);

            // lista_eventos é opcional (Projeto B)
            if (campos.size() >= 6) td.lista_eventos = campos[5];

            tarefas.push_back(td);
        }
    }

    arquivo.close();

    std::cout << "Configuracao carregada: algoritmo=" << algoritmo
              << " quantum=" << quantum
              << " cpus=" << qtde_cpus
              << " tarefas=" << tarefas.size() << std::endl;
}

// ============================================================
// split — PRIVADO
// Divide uma string em partes usando o delimitador informado.
// ============================================================
std::vector<std::string> Config::split(const std::string& linha, char delimitador) const {
    std::vector<std::string> campos;
    std::stringstream ss(linha);
    std::string campo;

    while (std::getline(ss, campo, delimitador)) {
        campos.push_back(campo);
    }

    return campos;
}

// ============================================================
// para_maiusculas — PRIVADO
// Converte a string para maiúsculas para comparação case-insensitive (Requisito 3.3.2).
// ============================================================
std::string Config::para_maiusculas(std::string s) const {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return std::toupper(c); });
    return s;
}

// ============================================================
// Getters
// ============================================================
std::string Config::get_algoritmo() const { return algoritmo; }
int Config::get_quantum()   const { return quantum; }
int Config::get_cpus()      const { return qtde_cpus; }

const std::vector<TaskData>& Config::get_tarefas() const { return tarefas; }
