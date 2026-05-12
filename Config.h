#pragma once
#include <string>
#include <vector>

// Estrutura com os dados brutos de uma tarefa lidos do arquivo de configuração
struct TaskData {
    int         id;
    std::string cor;
    int         ingresso;
    int         duracao;
    int         prioridade;
    std::string lista_eventos; // reservado para o Projeto B
};

// Classe Config — lê e armazena os parâmetros da simulação a partir de um
// arquivo de texto no formato definido pelo Requisito 3.3
class Config {
private:
    std::string          algoritmo; // algoritmo de escalonamento ("SRTF" ou "PRIOP")
    int                  quantum;   // período máximo de execução por tarefa
    int                  qtde_cpus; // quantidade de processadores
    std::vector<TaskData> tarefas;  // dados brutos de cada tarefa

    // Lê e interpreta o arquivo linha a linha
    void parse(const std::string& caminho_arquivo);

    // Divide uma string em partes usando um delimitador (usado para ler os campos separados por ';')
    std::vector<std::string> split(const std::string& linha, char delimitador) const;

    // Converte uma string para maiúsculas (Requisito 3.3.2 — comparação case-insensitive)
    std::string para_maiusculas(std::string s) const;

public:
    // Construtor: recebe o caminho do arquivo e já dispara o parse
    Config(const std::string& caminho_arquivo);

    // Getters
    std::string                   get_algoritmo() const;
    int                           get_quantum()   const;
    int                           get_cpus()      const;
    const std::vector<TaskData>&  get_tarefas()   const;
};
