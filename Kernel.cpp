#include "Kernel.h"
#include "SRTFScheduler.h"
#include "PrioPScheduler.h"
#include <iostream>

#include <algorithm> // Necessário para o std::transform
#include <cctype>    // Necessário para o std::toupper

// Método Construtor: Inicializa o sistema com base no arquivo de configuração
Kernel::Kernel(Config config) {
    

    // Normalização do Algoritmo (Requisito 3.3.2)

	// Obtém o algoritmo do arquivo de configuração
    std::string algoritmo = config.getAlgoritmo();

    // Transforma toda a string para MAIÚSCULAS para comparação segura
    std::transform(algoritmo.begin(), algoritmo.end(), algoritmo.begin(),
        [](unsigned char c) { return std::toupper(c); });

    // 2. Decisão do Escalonador com a string normalizada
    if (algoritmo == "SRTF") {
        escalonador = new SRTFScheduler();
    }
    else if (algoritmo == "PRIOP") {
        escalonador = new PrioPScheduler();
    }
    else {
        // Caso venha algo inesperado, podemos definir um padrão ou lançar erro
        std::cout << "Aviso: Algoritmo desconhecido. Usando PRIOP por padrao." << std::endl;
        escalonador = new PrioPScheduler();
    }


    // Inicialização básica de variáveis de controle
    relogioGlobal = 0;
    quantum = config.getQuantum(); 
    qtde_cpus = config.getCPUs(); 

   // Inicialização da variável contadora que será utilizada no método
    int i;

    // Criação das CPUs (Requisito 1.2)
    for (i = 0; i < config.getQtdeCPUs(); i++) {
        // Adiciona uma nova CPU ao vetor (ID da CPU começa em 0)
        cpus.push_back(CPU(i)); // i é o ID da CPU
    }

    // Transforma dados brutos em objetos Task (Requisito 1.3)
    // Criação do TCB (Task Control Block) para cada tarefa
    for (i = 0; i < config.getTasksData().size(); i++) {
        const auto& tData = config.getTasksData()[i];

        // requisito 3.3
        Task* novaTask = new Task(
            tData.id,
            tData.cor,
            tData.ingresso,
            tData.duracao,
            tData.prioridade
        );

        // Adição à lista mestre do Kernel
        all_tasks.push_back(novaTask);
    }

    std::cout << "Kernel inicializado com " << qtde_cpus
        << " CPUs e algoritmo " << config.getAlgoritmo() << std::endl;

}

// Método Destrutor: Limpa a memória (deleta Tasks e Escalonador)
Kernel::~Kernel() {

    // Deleta todas as tarefas armazenadas no TCB 
    for (int i = 0; i < all_tasks.size(); i++) {
        Task* t = all_tasks[i];
        delete t;                    
    }
    all_tasks.clear();
    //  Deleta o escalonador 
    if (escalonador != nullptr) {
        delete escalonador;
    }

    std::cout << "Memória do Kernel limpa com sucesso." << std::endl;
}



// Executa um passo de tempo, (tick) do relógio global
void Kernel::proximoTick() {
   
    // Salva como as coisas estão AGORA 
    // antes de começarmos a mudar os valores para o próximo segundo.
    salvarEstado();

	// Verificação da chegada de novas tarefas no tick atual
    verificarChegadaTarefas();

    // 2. Processamento nas CPUs
    for (i = 0; i < cpus.size(); i++) {
        if (cpus[i].estaOcupada()) {
            Task* task_atual = cpus[i].getTask();

            // Registra o evento no histórico da tarefa
            task_atual->adicionarEvento(relogioGlobal, "Executando", i);

            // Reduz o tempo restante
            task_atual->tempo_restante--;

            // 3. Verificando se terminou
            if (task_atual->tempo_restante == 0) {
                task_atual->estado = "Finalizada";
                cpus[i].liberar();
            }
        }
    }

    // 4. Escalonamento (A tomada de decisão)
    // O escalonador olha a fila e decide quem vai para as CPUs vazias
    escalonador->agendar(prontos, cpus, relogioGlobal);

    // Avanço do tempo
    relogioGlobal++;
}


void Kernel::retroceder() {
    // Verificação de segurança: Só podemos voltar se houver 'passado'
    if (historico.empty()) {
        std::cout << "Aviso: Nao ha mais estados para retroceder. Inicio atingido." << std::endl;
        return;
    }

    // Recupera o último estado salvo (o topo da pilha do histórico)
    Snapshot ultimo_estado = historico.back();
    historico.pop_back(); // Remove esse estado do histórico

    // Restauração das variáveis globais do Kernel
    relogio_global = ultimoEstado.relogio;
    cpus = ultimoEstado.estadoCPUs;

    // Restauração do estado interno de cada tarefa (TCB)
    for (size_t i = 0; i < ultimoEstado.estadoTarefas.size(); i++) {
        const auto& sTask = ultimoEstado.estadoTarefas[i];

        // Busca da tarefa real no nosso vetor all_tasks pelo ID
        for (Task* t : all_tasks) {
            if (t->id == sTask.id) {
                t->tempoRestante = sTask.tempoRestante;
                t->estado = sTask.estado;

                // IMPORTANTE: Limpeza do Gráfico de Gantt
                // Se voltamos no tempo, o evento que aconteceu naquele segundo 
                // "futuro" deve ser apagado para não sujar o gráfico.
                if (!t->lista_eventos.empty()) {
                    // Remove o último evento se o tempo dele for maior ou igual ao novo relógio
                    if (t->lista_eventos.back().tempo >= relogio_global) {
                        t->lista_eventos.pop_back();
                    }
                }
                break; // Achou a tarefa, pula para a próxima do snapshot
            }
        }
    }

    //Reconstrução da Fila de Prontos:

    prontos.clear();
    for (Task* t : all_tasks) {
        if (t->estado == "Pronta") {
            prontos.push_back(t);
        }
    }

    std::cout << "Sistema retrocedido para o tempo: " << relogio_global << std::endl;
}

// Move tarefas do vetor mestre para a fila de prontos ao atingirem o tempo de ingresso
void Kernel::verificarChegadaTarefas() {
    for (size_t i = 0; i < all_tasks.size(); i++) {
        // Se a tarefa chega exatamente agora e ainda não foi admitida
        if (all_tasks[i]->ingresso == relogio_global && all_tasks[i]->estado == "Criada") {

            // Muda o estado para que o escalonador saiba que ela pode rodar
            all_tasks[i]->estado = "Pronta";

            // Adiciona o ponteiro da tarefa na fila de prontos
            prontos.push_back(all_tasks[i]);

            // Log para depuração
            std::cout << "[Tempo " << relogio_global << "] Tarefa " << all_tasks[i]->id << " entrou na fila." << std::endl;
        }
    }
}

// Captura um snapshot completo do sistema antes de cada alteração para permitir a função de desfazer
void Kernel::salvarEstado() {
    Snapshot snap;

    // Salva o relógio atual (antes de incrementar)
    snap.relogio = relogioGlobal;

    // Salva o estado físico das CPUs 
    snap.estado_cpus = cpus;

    // Salva o estado dinâmico de cada tarefa (TCB)
    for (size_t i = 0; i < all_tasks.size(); i++) {
        Snapshot::TaskState tState;
        tState.id = all_tasks[i]->id;
        tState.tempoRestante = all_tasks[i]->tempoRestante;
        tState.estado = all_tasks[i]->estado;

        snap.estadoTarefas.push_back(tState);
    }

    // Adiciona o snapshot ao histórico (pilha)
    historico.push_back(snap);
}


// Verifica se todas as tarefas cadastradas atingiram o estado final de execução
bool Kernel::simulacaoConcluida() const {
    // Se ainda existem tarefas na fila de prontos, a simulação não acabou
    if (!prontos.empty()) {
        return false;
    }

    // Verifica se alguma CPU ainda está processando algo
    for (const auto& cpu : cpus) {
        if (cpu.estaOcupada()) {
            return false;
        }
    }

    // Verifica no TCB se todas as tarefas já chegaram ao estado "Finalizada"
    // Isso garante que tarefas que ainda nem ingressaram no sistema sejam consideradas
    for (const Task* t : all_tasks) {
        if (t->estado != "Finalizada") {
            return false;
        }
    }

    // Se passou por todos os testes, tudo foi concluído
    return true;
}

// Exibe o estado atual da simulação no console para acompanhamento
void Kernel::imprimirStatus() {
    std::cout << "\n========================================" << std::endl;
    std::cout << " TEMPO ATUAL: " << relogio_global << " | ALGORITMO: " << (escalonador ? "ATIVO" : "N/A") << std::endl;
    std::cout << "========================================" << std::endl;

    // 1. Status das CPUs
    std::cout << " [CPUs]" << std::endl;
    for (size_t i = 0; i < cpus.size(); i++) {
        std::cout << "  CPU " << i << ": ";
        if (cpus[i].estaOcupada()) {
            Task* t = cpus[i].getTarefa();
            std::cout << "[ID: " << t->id << "] - " << t->tempoRestante << "s restantes" << std::endl;
        }
        else {
            std::cout << "IDLE (Ociosa)" << std::endl;
        }
    }

    // Status da Fila de Prontos
    std::cout << "\n [FILA DE PRONTOS]" << std::endl;
    if (prontos.empty()) {
        std::cout << "  (Vazia)" << std::endl;
    }
    else {
        std::cout << "  ";
        for (Task* t : prontos) {
            std::cout << "T" << t->id << " ";
        }
        std::cout << std::endl;
    }

    // Resumo Geral de Tarefas
    int concluido = 0;
    for (Task* t : all_tasks) {
        if (t->estado == "Finalizada") concluido++;
    }
    std::cout << "\n Progresso: " << concluido << "/" << all_tasks.size() << " tarefas concluidas." << std::endl;
    std::cout << "========================================\n" << std::endl;
}