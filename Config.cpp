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
static const int         ALPHA_PADRAO     = 1; // taxa de envelhecimento padrao para PRIOPEnv

// ============================================================
// remover_prefixo_letras
// Remove qualquer letra no inicio do campo de ID da tarefa, permitindo que
// o arquivo de configuracao use tanto "1" quanto "t1"/"T1" para identificar
// a mesma tarefa. Se nao sobrar nenhum digito, retorna a string original
// (o std::stoi subsequente lanca excecao e a linha e ignorada normalmente).
// ============================================================
static std::string remover_prefixo_letras(const std::string& campo) {
    size_t i = 0;
    while (i < campo.size() && std::isalpha((unsigned char)campo[i])) i++;
    return (i < campo.size()) ? campo.substr(i) : campo;
}

// ============================================================
// Construtor
// ============================================================
Config::Config(const std::string& caminho_arquivo) {
    // Inicializa com valores padrão antes de ler o arquivo (Requisito 3.2)
    algoritmo = ALGORITMO_PADRAO;
    quantum   = QUANTUM_PADRAO;
    qtde_cpus = CPUS_PADRAO;
    alpha     = ALPHA_PADRAO;

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
            // Formato base: algoritmo;quantum;qtde_cpus
            // Formato PRIOPEnv: PRIOPEnv;quantum;qtde_cpus;alpha  (Projeto B Req. 1.1)
            if (campos.size() >= 1) algoritmo = para_maiusculas(campos[0]);
            if (campos.size() >= 2) quantum   = std::stoi(campos[1]);
            if (campos.size() >= 3) qtde_cpus = std::stoi(campos[2]);
            if (campos.size() >= 4) alpha     = std::stoi(campos[3]); // so presente no PRIOPEnv

            // Requisito 1.2: o sistema exige no minimo 2 processadores.
            // Valores invalidos no arquivo de configuracao sao corrigidos para o minimo,
            // em vez de deixar o simulador rodar com uma configuracao fora do especificado.
            if (qtde_cpus < 2) {
                std::cout << "Aviso: qtde_cpus=" << qtde_cpus
                          << " e invalido (minimo permitido = 2). Ajustando para 2." << std::endl;
                qtde_cpus = 2;
            }

            primeira_linha = false;
        } 
        else {
            // Demais linhas: uma tarefa por linha
            if (campos.size() < 5) {
                std::cout << "Aviso: linha de tarefa ignorada por ter menos de 5 campos: "
                          << linha << std::endl;
                continue;
            }

            // std::stoi lanca excecao se o campo nao comecar com um numero
            // (ex.: "t1" em vez de "1"). Sem este try/catch, uma linha mal
            // formatada derrubaria o programa inteiro em vez de apenas
            // avisar e ignorar a tarefa invalida.
            try {
                TaskData td;
                // Aceita tanto "1" quanto "t1"/"T1" (prefixo de letras) como o
                // mesmo ID: remove qualquer letra no inicio do campo antes de
                // converter, entao so o valor numerico e considerado.
                td.id = std::stoi(remover_prefixo_letras(campos[0]));
                td.cor = campos[1];
                td.ingresso = std::stoi(campos[2]);
                td.duracao = std::stoi(campos[3]);
                td.prioridade = std::stoi(campos[4]);

                // lista_eventos é opcional (Projeto B)
                if (campos.size() >= 6) td.lista_eventos = campos[5];

                tarefas.push_back(td);
            } catch (const std::exception&) {
                std::cout << "Aviso: linha de tarefa ignorada por conter valor numerico invalido: "
                          << linha << std::endl;
                continue;
            }
        }
    }

    arquivo.close();

    std::cout << "Configuracao carregada: algoritmo=" << algoritmo
              << " quantum=" << quantum
              << " cpus=" << qtde_cpus
              << " tarefas=" << tarefas.size();
    if (algoritmo == "PRIOPENV") std::cout << " alpha=" << alpha;
    std::cout << std::endl;
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
int Config::get_alpha()     const { return alpha; }

const std::vector<TaskData>& Config::get_tarefas() const { return tarefas; }

// ============================================================
// Setters (Requisito 3.1 — configuracao interativa pre-simulacao)
// ============================================================
void Config::set_algoritmo(const std::string& alg) { algoritmo = para_maiusculas(alg); }
void Config::set_quantum(int q)                    { quantum   = q; }
void Config::set_cpus(int n)                       { qtde_cpus = (n < 2) ? 2 : n; } // Requisito 1.2: minimo de 2 CPUs
void Config::set_alpha(int a)                      { alpha     = a; }

void Config::adicionar_tarefa(const TaskData& td) {
    tarefas.push_back(td);
}

bool Config::remover_tarefa(int id) {
    for (auto it = tarefas.begin(); it != tarefas.end(); ++it) {
        if (it->id == id) { tarefas.erase(it); return true; }
    }
    return false;
}

bool Config::editar_tarefa(int id, const TaskData& td) {
    for (auto& t : tarefas) {
        if (t.id == id) { t = td; return true; }
    }
    return false;
}
