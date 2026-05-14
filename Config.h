#pragma once
#include <string>
#include <vector>

// Estrutura com os dados brutos de uma tarefa lidos do arquivo de configuração.
// É usada pelo Config para armazenar os valores antes de criar os objetos Task.
struct TaskData {
    int         id;            // identificador único da tarefa
    std::string cor;           // cor RGB hex para o Gantt (ex: "FF0000")
    int         ingresso;      // tick de chegada ao sistema
    int         duracao;       // tempo total de execução em ticks
    int         prioridade;    // prioridade estática (usada pelo PRIOP)
    std::string lista_eventos; // reservado para eventos de I/O do Projeto B
};

// Classe Config — lê e armazena os parâmetros da simulação a partir de um
// arquivo de texto no formato definido pelo Requisito 3.3
class Config {
private:
    std::string algoritmo; // algoritmo de escalonamento ("SRTF" ou "PRIOP")
    int quantum; // período máximo de execução por tarefa
    int qtde_cpus; // quantidade de processadores
    std::vector<TaskData> tarefas; // dados brutos de cada tarefa

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
    std::string get_algoritmo() const;
    int get_quantum() const;
    int get_cpus() const;
    const std::vector<TaskData>& get_tarefas() const;

    // Setters para configuracao interativa pre-simulacao (Requisito 3.1)
    void set_algoritmo(const std::string& alg);
    void set_quantum(int q);
    void set_cpus(int n);
    void adicionar_tarefa(const TaskData& td);
    bool remover_tarefa(int id);
    bool editar_tarefa(int id, const TaskData& td);
};
