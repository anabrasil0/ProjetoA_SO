#include "Simulador.h"
#include "SRTFScheduler.h"
#include "PRIOPScheduler.h"
#include <iostream>
#include <algorithm>

// ============================================================
// Construtor
// Recebe a configuracao ja carregada e montada pelo Config,
// instancia o escalonador correto e cria os objetos de CPU e Task.
// Toda a memoria dinamica alocada aqui e liberada no destrutor.
// ============================================================
Simulador::Simulador(Config& config) {

    // Config ja normaliza o algoritmo para maiusculas no parse (Requisito 3.3.2)
    std::string algoritmo = config.get_algoritmo();

    // Instancia o escalonador correto via polimorfismo (Requisito 4.2).
    // O quantum e passado ao scheduler para que ele mesmo controle a preempcao
    // por tempo. O Simulador nao tem logica de preempcao — isso e politica do scheduler.
    int q = config.get_quantum();
    if (algoritmo == "SRTF") {
        escalonador = new SRTFScheduler(q);  // Shortest Remaining Time First
    }
    else if (algoritmo == "PRIOP") {
        escalonador = new PRIOPScheduler(q); // Preemptivo por Prioridades
    }
    else {
        // Algoritmo desconhecido no arquivo: usa PRIOP como fallback seguro
        std::cout << "Aviso: Algoritmo desconhecido. Usando PRIOP por padrao." << std::endl;
        escalonador = new PRIOPScheduler(q);
    }

    relogio_global = 0;              // simulacao começa no tick zero
    quantum        = config.get_quantum();
    qtde_cpus      = config.get_cpus();

    // Cria os nucleos de processamento (Requisito 1.2)
    for (int i = 0; i < qtde_cpus; i++) {
        cpus.push_back(CPU()); // cada CPU nasce ligada e sem tarefa
    }

    // Cria os TCBs a partir dos dados brutos lidos do arquivo (Requisito 1.3)
    for (const auto& t_data : config.get_tarefas()) {
        Task* nova_task = new Task(
            t_data.id,
            t_data.ingresso,
            t_data.duracao,
            t_data.prioridade,
            t_data.cor       // cor hex para identificacao no Gantt
        );
        all_tasks.push_back(nova_task); // adiciona ao vetor de todas as tarefas do sistema
    }

    std::cout << "Simulador inicializado com " << qtde_cpus
              << " CPUs e algoritmo " << config.get_algoritmo() << std::endl;
}

// ============================================================
// Destrutor
// Libera a memoria de todos os TCBs e do escalonador alocados
// no construtor. O vetor cpus usa objetos por valor, entao nao
// precisa de delete manual.
// ============================================================
Simulador::~Simulador() {
    // Deleta cada TCB alocado dinamicamente
    for (Task* t : all_tasks) {
        delete t;
    }
    all_tasks.clear(); // garante que o vetor fique vazio apos a limpeza

    // Deleta o escalonador alocado dinamicamente
    if (escalonador != nullptr) {
        delete escalonador;
    }

    std::cout << "Memoria do Simulador limpa com sucesso." << std::endl;
}

// ============================================================
// do_tick — PRIVADO
// Logica pura de um tick: chegada → escalonamento → registro → execucao → avanco.
// Nao salva snapshot; isso e responsabilidade de step_forward.
// Separar aqui permite que run_complete() chame do_tick() diretamente,
// sem o custo de salvar estado a cada tick.
// ============================================================
void Simulador::do_tick() {

    // Passo 1: admite na fila de prontos todas as tarefas cujo ingresso == tick atual
    verificar_chegada_tarefas();

    // Passo 2: escalonador decide qual tarefa vai para cada CPU (Requisito 4)
    // O scheduler pode preemptar a tarefa atual, atribuir novas ou desligar CPUs.
    escalonador->escalonar(prontos, cpus, relogio_global);

    // Passo 3: registra o estado atual no Gantt APOS o escalonamento e ANTES de executar.
    // Dessa forma o Gantt mostra a tarefa que esta "prestes a rodar" neste tick,
    // nao a que estava rodando antes do escalonamento.
    {
        TickGantt tick_data;
        tick_data.tick = relogio_global; // numero do tick atual

        // Para cada tarefa do sistema, gera uma entrada no snapshot do Gantt
        for (Task* t : all_tasks) {
            EntradaGantt e;
            e.tarefa_id = t->get_id();
            e.cor       = t->get_cor(); // cor para colorir o bloco no grafico
            e.cpu_id    = -1;           // -1 = nao esta em nenhuma CPU

            // Verifica em qual CPU (se alguma) esta tarefa esta executando
            for (size_t i = 0; i < cpus.size(); i++) {
                if (cpus[i].esta_ocupada() && cpus[i].get_tarefa_atual() == t) {
                    e.cpu_id = static_cast<int>(i); // indice da CPU que a executa
                    break;
                }
            }

            // Define o tipo de entrada conforme o estado/posicao da tarefa
            if (e.cpu_id >= 0) {
                e.tipo = TipoGantt::EXECUTANDO;
                // Detecta termino: se tempo_restante == 1 agora, chegara a 0 apos processar_ciclo
                e.evento_termino = (t->get_tempo_restante() == 1);
            } else if (t->get_estado() == FINALIZADA) {
                e.tipo = TipoGantt::FINALIZADA;
            } else if (t->get_estado() == PRONTA) {
                e.tipo = TipoGantt::PRONTA;    // aguardando na fila de prontos
            } else if (t->get_estado() == CRIADA) {
                e.tipo = TipoGantt::NAO_CHEGOU; // ainda nao entrou no sistema
            } else {
                e.tipo = TipoGantt::SUSPENSA;   // bloqueada (reservado para Projeto B)
            }

            // Detecta chegada: tarefa entrou neste tick exato
            // O estado NAO_CHEGOU e excluido pois significa que o ingresso ainda nao ocorreu
            e.evento_chegada = (t->get_tempo_ingresso() == relogio_global &&
                                e.tipo != TipoGantt::NAO_CHEGOU);

            tick_data.entradas.push_back(e); // adiciona a entrada da tarefa ao snapshot
        }

        // Marca as tarefas escolhidas por sorteio neste tick (Requisito 4.3).
        // O scheduler registra os IDs vencedores em sorteio_ids durante escalonar();
        // aqui propagamos esse evento para a entrada correspondente no Gantt,
        // para que GanttChart possa exibir o elemento grafico do sorteio.
        for (int id : escalonador->get_sorteio_ids()) {
            for (auto& e : tick_data.entradas) {
                if (e.tarefa_id == id) { e.evento_sorteio = true; break; }
            }
        }

        // Registra o estado de cada CPU neste tick (Requisito 1.2).
        // Capturado apos escalonar() e antes de processar_ciclo(), refletindo
        // qual tarefa esta atribuida a cada CPU neste momento.
        for (size_t i = 0; i < cpus.size(); i++) {
            EntradaCPU ec;
            ec.cpu_id = static_cast<int>(i);
            if (!cpus[i].esta_ligada()) {
                ec.estado = EstadoCPU::DESLIGADA;
            } else if (cpus[i].esta_ocupada()) {
                ec.estado    = EstadoCPU::EXECUTANDO;
                ec.tarefa_id = cpus[i].get_tarefa_atual()->get_id();
                ec.cor       = cpus[i].get_tarefa_atual()->get_cor();
            } else {
                ec.estado = EstadoCPU::OCIOSA;
            }
            tick_data.cpus.push_back(ec);
        }

        gantt_log.registrar(tick_data); // armazena o snapshot no historico do Gantt
    }

    // Passo 4: notifica cada CPU para avançar seu estado interno em um ciclo.
    // processar_ciclo() decrementa o tempo restante das tarefas em execucao e
    // detecta finalizacoes. O relogio ja foi "disparado" pelo Simulador; a CPU
    // apenas responde a ele.
    for (size_t i = 0; i < cpus.size(); i++) {
        cpus[i].processar_ciclo();
    }

    // Passo 5: remove da fila de prontos as tarefas que finalizaram neste ciclo.
    // Uma tarefa pode ter sido finalizada dentro de processar_ciclo() (tempo esgotado).
    // Manter ela na fila causaria decisoes erradas do scheduler no proximo tick.
    prontos.erase(
        std::remove_if(prontos.begin(), prontos.end(),
            [](Task* t) { return t->get_estado() == FINALIZADA; }),
        prontos.end()
    );

    // Passo 6: avanca o relogio global para o proximo tick (Requisito 1.1)
    relogio_global++;
}

// ============================================================
// step_forward — PUBLICO (Requisito 1.5 opcao a)
// Salva o estado ANTES de avançar para permitir o retrocesso.
// E esse metodo que a interface chama no modo passo-a-passo.
// ============================================================
void Simulador::step_forward() {
    // Nao avanca se a simulacao ja terminou; evita estado inconsistente
    if (simulacao_concluida()) {
        std::cout << "Simulacao ja concluida." << std::endl;
        return;
    }

    // Snapshot tirado ANTES da mudanca de estado, para que step_backward
    // consiga restaurar o tick anterior com fidelidade.
    salvar_estado();

    do_tick(); // executa a logica do tick apos garantir que o estado anterior foi salvo
}

// ============================================================
// step_backward — PUBLICO (Requisito 1.5.2)
// Restaura o estado do tick anterior a partir do historico de snapshots.
// Alem dos TCBs e do relogio, o Gantt tambem e truncado para remover
// os ticks que deixaram de existir apos o retrocesso.
// ============================================================
void Simulador::step_backward() {
    // Sem historico salvo nao ha para onde voltar
    if (historico.empty()) {
        std::cout << "Aviso: Nao ha estados para retroceder. Inicio atingido." << std::endl;
        return;
    }

    // Recupera e remove o snapshot mais recente da pilha de historico
    Snapshot ultimo_estado = historico.back();
    historico.pop_back();

    // Restaura o relogio e o estado interno de cada CPU (ligada, tarefa, quantum)
    relogio_global = ultimo_estado.relogio;
    cpus           = ultimo_estado.estado_cpus; // copia por valor restaura todos os campos

    // Restaura o estado interno de cada tarefa (tempo_restante, estado) pelo ID
    for (const auto& s_task : ultimo_estado.estado_tarefas) {
        for (Task* t : all_tasks) {
            if (t->get_id() == s_task.id) {
                t->set_tempo_restante(s_task.tempo_restante); // progresso desfeito
                t->set_estado(s_task.estado);                 // estado desfeito
                break;
            }
        }
    }

    // Reconstroi a fila de prontos a partir do estado restaurado.
    // Apenas tarefas com estado PRONTA pertencem a fila; tarefas EXECUTANDO
    // ja estao vinculadas a uma CPU pelo snapshot restaurado acima.
    prontos.clear();
    for (Task* t : all_tasks) {
        if (t->get_estado() == PRONTA) {
            prontos.push_back(t);
        }
    }

    // Remove do historico do Gantt os ticks que pertenciam ao futuro (Requisito 2.3)
    // Garante que o grafico encolha corretamente ao retroceder
    gantt_log.truncar_apos(relogio_global);

    std::cout << "Sistema retrocedido para o tempo: " << relogio_global << std::endl;
}

// ============================================================
// run_complete — PUBLICO (Requisito 1.5 opcao b)
// Executa a simulacao ate o fim sem intervencao humana.
// Chama do_tick() diretamente, sem salvar snapshots, pois o modo
// completo nao precisa de retrocesso (Requisito 1.5.3).
// ============================================================
void Simulador::run_complete() {
    // Continua executando ticks enquanto houver trabalho pendente
    while (!simulacao_concluida()) {
        do_tick();
    }
    std::cout << "Simulacao completa finalizada no tick " << relogio_global << std::endl;
}

// ============================================================
// verificar_chegada_tarefas — PRIVADO
// Admite na fila de prontos toda tarefa cujo ingresso == tick atual.
// Chamado no inicio de cada do_tick(), antes do escalonamento,
// para que as tarefas recem-chegadas ja possam ser escalonadas
// neste mesmo tick.
// ============================================================
void Simulador::verificar_chegada_tarefas() {
    for (Task* t : all_tasks) {
        // Filtra: so tarefas CRIADAS (nao chegadas ainda) cujo ingresso e agora
        if (t->get_tempo_ingresso() == relogio_global && t->get_estado() == CRIADA) {
            t->set_estado(PRONTA);       // transicao de estado: CRIADA → PRONTA
            prontos.push_back(t);        // entra na fila de prontos para ser escalonada
            std::cout << "[Tick " << relogio_global << "] Tarefa " << t->get_id()
                      << " chegou e entrou na fila de prontos." << std::endl;
        }
    }
}

// ============================================================
// salvar_estado — PRIVADO
// Tira um snapshot completo do sistema (relogio + CPUs + TCBs).
// Chamado por step_forward antes de do_tick para que step_backward
// possa restaurar o estado exato deste ponto no tempo.
// ============================================================
void Simulador::salvar_estado() {
    Snapshot snap;
    snap.relogio     = relogio_global;
    snap.estado_cpus = cpus; // copia por valor captura: tarefa_atual, ligada, quantum, ociosa

    // Captura o estado relevante de cada TCB para poder restaura-lo depois
    for (Task* t : all_tasks) {
        Snapshot::TaskState t_state;
        t_state.id             = t->get_id();
        t_state.tempo_restante = t->get_tempo_restante(); // progresso atual da tarefa
        t_state.estado         = t->get_estado();         // posicao no ciclo de vida
        snap.estado_tarefas.push_back(t_state);
    }

    historico.push_back(snap); // empilha o snapshot para uso futuro em step_backward
}

// ============================================================
// simulacao_concluida — PUBLICO
// Retorna true somente quando todas as condicoes de termino sao
// satisfeitas simultaneamente:
//   1. Fila de prontos vazia (sem tarefas esperando CPU)
//   2. Nenhuma CPU ocupada (nenhuma tarefa em execucao)
//   3. Todos os TCBs em estado FINALIZADA
// Se qualquer condicao falhar, a simulacao ainda tem trabalho a fazer.
// ============================================================
bool Simulador::simulacao_concluida() const {
    // Condicao 1: ainda ha tarefas aguardando execucao
    if (!prontos.empty()) return false;

    // Condicao 2: alguma CPU ainda esta executando uma tarefa
    for (const CPU& cpu : cpus) {
        if (cpu.esta_ocupada()) return false;
    }

    // Condicao 3: alguma tarefa ainda nao chegou ou esta em progresso
    for (const Task* t : all_tasks) {
        if (t->get_estado() != FINALIZADA) return false;
    }

    return true; // todas as condicoes satisfeitas: simulacao encerrada
}

// ============================================================
// modificar_estado_tarefa — PUBLICO (Requisito 3.4)
// Permite que o usuario altere o estado de qualquer tarefa durante
// a simulacao. Alem de atualizar o TCB, mantém a consistencia interna:
//   - PRONTA:     insere na fila de prontos se ainda nao estiver;
//                 libera a CPU caso a tarefa estivesse sendo executada.
//   - FINALIZADA: remove da fila de prontos e desvincula de qualquer CPU.
// ============================================================
void Simulador::modificar_estado_tarefa(int id, Estado novo_estado) {
    // Busca a tarefa pelo ID no vetor de todas as tarefas
    Task* alvo = nullptr;
    for (Task* t : all_tasks) {
        if (t->get_id() == id) { alvo = t; break; }
    }

    // ID invalido: informa o usuario e encerra sem alterar nada
    if (alvo == nullptr) {
        std::cout << "Tarefa " << id << " nao encontrada." << std::endl;
        return;
    }

    alvo->set_estado(novo_estado); // aplica o novo estado no TCB

    if (novo_estado == PRONTA) {
        // Garante que a tarefa esteja na fila de prontos (sem duplicatas)
        bool ja_esta = false;
        for (Task* t : prontos) {
            if (t->get_id() == id) { ja_esta = true; break; }
        }
        if (!ja_esta) prontos.push_back(alvo); // insere apenas se ainda nao estiver

        // Se a tarefa estava em alguma CPU, libera essa CPU para receber outra tarefa
        for (CPU& cpu : cpus) {
            if (cpu.esta_ocupada() && cpu.get_tarefa_atual()->get_id() == id) {
                cpu.set_tarefa_atual(nullptr); // CPU fica ociosa ate o proximo escalonamento
            }
        }
    }
    else if (novo_estado == FINALIZADA) {
        // Remove a tarefa da fila de prontos usando o padrao erase-remove
        prontos.erase(
            std::remove_if(prontos.begin(), prontos.end(),
                [id](Task* t) { return t->get_id() == id; }),
            prontos.end()
        );

        // Libera qualquer CPU que esteja executando essa tarefa
        for (CPU& cpu : cpus) {
            if (cpu.esta_ocupada() && cpu.get_tarefa_atual()->get_id() == id) {
                cpu.set_tarefa_atual(nullptr); // CPU fica ociosa ate o proximo escalonamento
            }
        }
    }

    std::cout << "Tarefa " << id << " alterada para "
              << (novo_estado == PRONTA ? "PRONTA" : "FINALIZADA") << "." << std::endl;
}

// ============================================================
// imprimir_status — PUBLICO
// Exibe um resumo formatado do estado atual do sistema: relogio,
// estado de cada CPU, fila de prontos e progresso geral das tarefas.
// ============================================================
void Simulador::imprimir_status() {
    std::cout << "\n========================================" << std::endl;
    std::cout << " TEMPO ATUAL: " << relogio_global
              << " | ALGORITMO: " << (escalonador ? "ATIVO" : "N/A") << std::endl;
    std::cout << "========================================" << std::endl;

    // Estado de cada CPU individualmente
    std::cout << " [CPUs]" << std::endl;
    for (size_t i = 0; i < cpus.size(); i++) {
        std::cout << "  CPU " << i << ": ";
        if (!cpus[i].esta_ligada()) {
            // CPU desligada por ausencia de tarefas (Requisito 1.2)
            std::cout << "DESLIGADA (total desligada: "
                      << cpus[i].get_tempo_desligada() << " ticks)" << std::endl;
        }
        else if (cpus[i].esta_ocupada()) {
            // CPU executando: mostra qual tarefa e o progresso do quantum
            Task* t = cpus[i].get_tarefa_atual();
            std::cout << "Executando T" << t->get_id()
                      << " | restante: " << t->get_tempo_restante() << " ticks"
                      << " | quantum usado: " << cpus[i].get_ticks_no_quantum() << std::endl;
        }
        else {
            // CPU ligada mas sem tarefa atribuida
            std::cout << "OCIOSA (total ocioso: "
                      << cpus[i].get_tempo_ocioso() << " ticks)" << std::endl;
        }
    }

    // Tarefas aguardando CPU
    std::cout << "\n [FILA DE PRONTOS]" << std::endl;
    if (prontos.empty()) {
        std::cout << "  (Vazia)" << std::endl;
    }
    else {
        std::cout << "  ";
        for (Task* t : prontos) {
            std::cout << "T" << t->get_id() << " "; // lista os IDs das tarefas prontas
        }
        std::cout << std::endl;
    }

    // Contagem de tarefas concluidas sobre o total
    int concluido = 0;
    for (Task* t : all_tasks) {
        if (t->get_estado() == FINALIZADA) concluido++;
    }
    std::cout << "\n Progresso: " << concluido << "/" << all_tasks.size()
              << " tarefas concluidas." << std::endl;
    std::cout << "========================================\n" << std::endl;
}
