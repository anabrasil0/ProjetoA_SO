#include "Simulador.h"
#include "SRTFScheduler.h"
#include "PRIOPScheduler.h"
#include "PRIOPEnvScheduler.h"
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
    else if (algoritmo == "PRIOPENV") {
        // Preemptivo por Prioridades com Envelhecimento (Projeto B Req. 1)
        // O alpha e lido do config e controla quanto a prioridade efetiva cresce por tick de espera
        escalonador = new PRIOPEnvScheduler(q, config.get_alpha());
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
            t_data.cor,          // cor hex para identificacao no Gantt
            t_data.lista_eventos // string bruta; parseada em Evento[] no construtor de Task
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

    // Passo 0: acorda tarefas que concluiram operacao de E/S neste tick (IRQ simulado, Req. 3).
    // Executado ANTES do escalonamento para que tarefas acordadas sejam imediatamente candidatas.
    verificar_conclusao_io();

    // Passo 1: admite na fila de prontos todas as tarefas cujo ingresso == tick atual
    verificar_chegada_tarefas();

    // Passo 2: escalonador decide qual tarefa vai para cada CPU (Requisito 4)
    escalonador->escalonar(prontos, cpus, relogio_global);

    // Passo 3: processa eventos (mutex e E/S) das tarefas em execucao neste tick (Req. 2 e 3).
    // Os eventos disparam quando ticks_executados da tarefa == evento.tempo_relativo.
    // Eventos sao verificados APOS o escalonamento e ANTES de processar_ciclo(),
    // garantindo a ordenacao: "primeiro (re)iniciar a tarefa, depois a acao" (Req. 2.6).
    // Se uma tarefa bloqueia (ML falhou ou IO iniciou), ela e retirada da CPU aqui mesmo.
    std::vector<EventoTick> eventos_tick;
    processar_eventos(eventos_tick);

    // Passo 4: registra o estado atual no Gantt apos escalonamento e eventos, antes de executar.
    {
        TickGantt tick_data;
        tick_data.tick = relogio_global;

        for (Task* t : all_tasks) {
            EntradaGantt e;
            e.tarefa_id = t->get_id();
            e.cor       = t->get_cor();
            e.cpu_id    = -1;

            for (size_t i = 0; i < cpus.size(); i++) {
                if (cpus[i].esta_ocupada() && cpus[i].get_tarefa_atual() == t) {
                    e.cpu_id = static_cast<int>(i);
                    break;
                }
            }

            Estado est = t->get_estado();
            if (e.cpu_id >= 0) {
                e.tipo = TipoGantt::EXECUTANDO;
                e.evento_termino = (t->get_tempo_restante() == 1);
            } else if (est == FINALIZADA) {
                e.tipo = TipoGantt::FINALIZADA;
            } else if (est == PRONTA) {
                e.tipo = TipoGantt::PRONTA;
            } else if (est == CRIADA) {
                e.tipo = TipoGantt::NAO_CHEGOU;
            } else if (est == SUSPENSA_MUTEX) {
                e.tipo = TipoGantt::SUSPENSA_MUTEX; // bloco visual distinto (Req. 2.9)
            } else if (est == SUSPENSA_IO) {
                e.tipo = TipoGantt::SUSPENSA_IO;    // bloco visual distinto (Req. 2.9)
            } else {
                e.tipo = TipoGantt::PRONTA;
            }

            e.evento_chegada = (t->get_tempo_ingresso() == relogio_global &&
                                e.tipo != TipoGantt::NAO_CHEGOU);

            // Propaga eventos deste tick para o Gantt (Req. 2.8 e 3)
            for (const auto& et : eventos_tick) {
                if (et.tarefa_id == t->get_id()) {
                    if      (et.tipo == TipoRegistroTick::MUTEX_LOCK)   e.evento_mutex_lock   = true;
                    else if (et.tipo == TipoRegistroTick::MUTEX_UNLOCK) e.evento_mutex_unlock = true;
                    else if (et.tipo == TipoRegistroTick::IO_INICIO)    e.evento_io           = true;
                }
            }

            tick_data.entradas.push_back(e);
        }

        for (int id : escalonador->get_sorteio_ids()) {
            for (auto& e : tick_data.entradas) {
                if (e.tarefa_id == id) { e.evento_sorteio = true; break; }
            }
        }

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

        gantt_log.registrar(tick_data);
    }

    // Passo 5: cada CPU executa um ciclo (decrementa tempo_restante, incrementa ticks_executados)
    for (size_t i = 0; i < cpus.size(); i++) {
        cpus[i].processar_ciclo();
    }

    // Passo 6: remove da fila de prontos tarefas finalizadas neste ciclo
    prontos.erase(
        std::remove_if(prontos.begin(), prontos.end(),
            [](Task* t) { return t->get_estado() == FINALIZADA; }),
        prontos.end()
    );

    // Passo 7: avanca o relogio global
    relogio_global++;
}

// ============================================================
// processar_eventos — PRIVADO (Req. 2 e 3)
// Verifica e executa eventos (mutex e E/S) para todas as tarefas em execucao.
// Chamado apos escalonar() e antes de processar_ciclo(), garantindo que:
//   - A tarefa ja foi (re)iniciada na CPU (por escalonar()), e
//   - Os eventos do instante atual (ticks_executados == tempo_relativo) disparam.
//
// Para cada CPU com tarefa: itera pelos eventos a partir de indice_proximo_evento.
// Para o mesmo ticks_executados, executa todos os eventos em ordem (Req. 2.5).
// Se uma tarefa bloqueia (ML falhou ou IO iniciou), para de processar (saiu da CPU).
// ============================================================
void Simulador::processar_eventos(std::vector<EventoTick>& out) {
    for (CPU& cpu : cpus) {
        if (!cpu.esta_ocupada()) continue;
        Task* t = cpu.get_tarefa_atual();

        const auto& evs = t->get_eventos();
        int idx = t->get_indice_proximo_evento();

        while (idx < (int)evs.size()) {
            const Evento& ev = evs[idx];

            // So processa eventos cujo tempo_relativo == ticks_executados atual
            if (ev.tempo_relativo != t->get_ticks_executados()) break;

            // Avanca o indice antes de executar — garante avanco mesmo se houver
            // retorno antecipado (tarefa bloqueada) dentro do loop
            t->avancar_indice_evento();
            idx++;

            if (ev.tipo == TipoEvento::MUTEX_LOCK) {
                out.push_back({t->get_id(), TipoRegistroTick::MUTEX_LOCK});

                MutexSim* mx = encontrar_ou_criar_mutex(ev.mutex_id);
                bool adquiriu = mx->tentar_lock(t->get_id());

                if (!adquiriu) {
                    // Mutex ocupado: suspende a tarefa e libera a CPU (Req. 2.7)
                    t->set_estado(SUSPENSA_MUTEX);
                    t->set_mutex_aguardando_id(ev.mutex_id);
                    cpu.set_tarefa_atual(nullptr); // CPU fica ociosa; nao executara tick
                    break; // tarefa saiu da CPU: nao ha mais eventos a processar agora
                }
                // Adquiriu com sucesso: continua processando proximos eventos do mesmo tick

            } else if (ev.tipo == TipoEvento::MUTEX_UNLOCK) {
                out.push_back({t->get_id(), TipoRegistroTick::MUTEX_UNLOCK});

                MutexSim* mx = encontrar_mutex(ev.mutex_id);
                if (mx) {
                    int next_id = mx->unlock(t->get_id());
                    if (next_id != -1) {
                        Task* prox = encontrar_tarefa(next_id);
                        if (prox) {
                            // Tarefa despertada: volta a fila de prontos (Req. 2.7)
                            prox->set_estado(PRONTA);
                            prox->set_mutex_aguardando_id(-1);
                            prontos.push_back(prox);
                        }
                    }
                }
                // Continua processando proximos eventos do mesmo tick

            } else if (ev.tipo == TipoEvento::IO_INICIO) {
                out.push_back({t->get_id(), TipoRegistroTick::IO_INICIO});

                // Calcula o tick global em que a E/S conclui (Req. 3.2)
                // io_tick_conclusao = relogio_global + io_duracao
                // Duracao 1 significa suspenso este tick; acorda no proximo.
                t->set_io_tick_conclusao(relogio_global + ev.io_duracao);
                t->set_estado(SUSPENSA_IO);
                cpu.set_tarefa_atual(nullptr); // CPU fica ociosa durante a E/S
                std::cout << "[Tick " << relogio_global << "] Tarefa " << t->get_id()
                          << " iniciou E/S (duracao " << ev.io_duracao
                          << " ticks; conclui no tick " << (relogio_global + ev.io_duracao)
                          << ")." << std::endl;
                break; // tarefa saiu da CPU: nao ha mais eventos a processar agora
            }
        }
    }
}

// ============================================================
// verificar_conclusao_io — PRIVADO (Req. 3)
// Verifica se alguma tarefa em estado SUSPENSA_IO teve sua operacao
// de E/S concluida neste tick (io_tick_conclusao == relogio_global).
// A tarefa e acordada e inserida de volta na fila de prontos para
// ser escalonada imediatamente no mesmo tick (IRQ simulado).
// Chamado NO INICIO de do_tick(), antes de escalonar().
// ============================================================
void Simulador::verificar_conclusao_io() {
    for (Task* t : all_tasks) {
        if (t->get_estado() == SUSPENSA_IO &&
            t->get_io_tick_conclusao() == relogio_global) {
            t->set_estado(PRONTA);
            t->set_io_tick_conclusao(-1);
            prontos.push_back(t);
            std::cout << "[Tick " << relogio_global << "] Tarefa " << t->get_id()
                      << " concluiu E/S e voltou para a fila de prontos." << std::endl;
        }
    }
}

// ============================================================
// encontrar_ou_criar_mutex — PRIVADO
// Retorna ponteiro para o MutexSim com o ID dado, criando-o caso nao exista.
// Mutexes sao criados lentamente (lazy) na primeira vez que sao referenciados.
// ATENCAO: apos push_back o vetor pode realocar, invalidando ponteiros anteriores.
// Por isso chamamos encontrar_ou_criar_mutex uma vez por evento, sem guardar ponteiros.
// ============================================================
MutexSim* Simulador::encontrar_ou_criar_mutex(int mutex_id) {
    for (auto& m : mutexes) {
        if (m.get_id() == mutex_id) return &m;
    }
    mutexes.push_back(MutexSim(mutex_id));
    return &mutexes.back();
}

MutexSim* Simulador::encontrar_mutex(int mutex_id) {
    for (auto& m : mutexes) {
        if (m.get_id() == mutex_id) return &m;
    }
    return nullptr;
}

Task* Simulador::encontrar_tarefa(int tarefa_id) {
    for (Task* t : all_tasks) {
        if (t->get_id() == tarefa_id) return t;
    }
    return nullptr;
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

    // Restaura o estado interno de cada tarefa
    for (const auto& ts : ultimo_estado.estado_tarefas) {
        for (Task* t : all_tasks) {
            if (t->get_id() == ts.id) {
                t->set_tempo_restante(ts.tempo_restante);
                t->set_estado(ts.estado);
                t->set_idade(ts.idade);
                t->set_ticks_executados(ts.ticks_executados);
                t->set_indice_proximo_evento(ts.indice_proximo_evento);
                t->set_mutex_aguardando_id(ts.mutex_aguardando_id);
                t->set_io_tick_conclusao(ts.io_tick_conclusao);
                break;
            }
        }
    }

    // Restaura o estado dos mutexes (Req. 2 — step_backward)
    mutexes.clear();
    for (const auto& ms : ultimo_estado.estado_mutexes) {
        MutexSim m(ms.mutex_id);
        m.set_dono_id(ms.dono_id);
        m.set_fila(ms.fila_ids);
        mutexes.push_back(m);
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

    // Captura o estado relevante de cada TCB
    for (Task* t : all_tasks) {
        Snapshot::TaskState ts;
        ts.id                   = t->get_id();
        ts.tempo_restante       = t->get_tempo_restante();
        ts.estado               = t->get_estado();
        ts.idade                = t->get_idade();
        ts.ticks_executados     = t->get_ticks_executados();
        ts.indice_proximo_evento = t->get_indice_proximo_evento();
        ts.mutex_aguardando_id  = t->get_mutex_aguardando_id();
        ts.io_tick_conclusao    = t->get_io_tick_conclusao();
        snap.estado_tarefas.push_back(ts);
    }

    // Captura o estado de cada mutex (Req. 2 — step_backward)
    for (const auto& m : mutexes) {
        Snapshot::MutexState ms;
        ms.mutex_id  = m.get_id();
        ms.dono_id   = m.get_dono_id();
        ms.fila_ids  = m.get_fila();
        snap.estado_mutexes.push_back(ms);
    }

    historico.push_back(snap);
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

    // Condicao 3: alguma tarefa ainda nao chegou, esta em progresso ou suspensa
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

    // Se a tarefa estava suspensa por mutex, remove-a da fila de espera do mutex
    if (alvo->get_estado() == SUSPENSA_MUTEX) {
        int mid = alvo->get_mutex_aguardando_id();
        MutexSim* mx = encontrar_mutex(mid);
        if (mx) {
            // Reconstroi a fila sem este task_id
            std::vector<int> nova_fila;
            for (int fid : mx->get_fila()) {
                if (fid != id) nova_fila.push_back(fid);
            }
            mx->set_fila(nova_fila);
        }
        alvo->set_mutex_aguardando_id(-1);
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
