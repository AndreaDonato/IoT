#include <stdlib.h>
#include <stdio.h>
#include <stdint.h> 
#include "traffic_lights.h"

static inline int clamp(int x, int a, int b){ return x<a?a: (x>b?b:x); }    // assicura che x sia in [a,b]

int mdp_num_states(const MDPParams *p){                                     // conta il numero di stati
    return (p->max_r1+1)*(p->max_r2+1)*2;
}

int state_encode(const MDPParams *p, State s){                              // appiattisce l'array 3D dello stato in un solo indice. Efficiente per usare array invece di tabelle multidimensionali o hashmap.
    int W = p->max_r2 + 1;
    int base = s.n1 * W + s.n2;                                             // indice per la coppia (n1,n2)
    int idx  = base * 2 + (s.g1 ? 1 : 0);                                   // moltiplica per 2 e aggiungi g1
    return idx;
}

State state_decode(const MDPParams *p, int idx){                            // ricostruisce lo stato a partire dallo stato appiattito. Efficiente per calcolare deflussi, arrivi, reward.
    State s;
    s.g1 = idx % 2;                                                         // ultimo “slot” dice quale semaforo è verde
    idx /= 2;                                                               // rimuovi quel bit
    int W = p->max_r2 + 1;
    s.n1 = idx / W;
    s.n2 = idx % W;
    return s;
}

int mdp_reward(const MDPParams *p, State sp){                               // funzione di calcolo del reward
    int N = sp.n1 + sp.n2;
    if (N < p->low_th) return +1;
    if (N < p->med_th) return 0;
    return -1;
}

static inline unsigned int xorshift32(unsigned int *st){                    // generatore di numeri pseudocasuali deterministico su qualsiasi piattaforma
    unsigned int x=*st; 
    x ^= x<<13; 
    x ^= x>>17; 
    x ^= x<<5; 
    return *st=x;
}


int mdp_transitions(const MDPParams *p, State s, int action, int out_idx[], double out_p[]){    // costruisce l'elenco di tutti gli stati successivi possibili e la loro probabilità a partire dallo stato s e dall'azione action. Più efficiente di costruire l'intera matrice di transizione.
    // action: 0 => TL1 green; 1 => TL2 green
    int k=0;
    for(int i=0; i<=p->add_r1_max; i++){                                      // cicla su tutte le possibili nuove auto che arrivano
        for(int j=0; j<=p->add_r2_max; j++){
            State sp;
            if(action==0){
                int pass = s.n1<p->out_r1_max? s.n1:p->out_r1_max;                                  // massimo 3 auto passano se TL1 è verde
                sp.n1 = clamp(s.n1 - pass + i, 0, p->max_r1);
                sp.n2 = clamp(s.n2 + j, 0, p->max_r2);
                sp.g1 = 1;                                                  // segna nello stato l'informazione TL1 verde
            }else{
                int pass = s.n2<p->out_r2_max? s.n2:p->out_r2_max;                                  // massimo 2 auto passano se TL2 è verde
                sp.n1 = clamp(s.n1 + i, 0, p->max_r1);
                sp.n2 = clamp(s.n2 - pass + j, 0, p->max_r2);
                sp.g1 = 0;                                                  // segna nello stato l'informazione TL2 verde
            }
            out_idx[k] = state_encode(p, sp);                                 // codifica lo stato succesivo calcolato in un indice intero
            out_p[k]   = 1.0/((p->add_r1_max+1)*(p->add_r2_max+1));         // probabilità uniforme su tutte le combinazioni di nuove auto
            k++;
        }
    }
    return k;   // ritorna il numero di stati successivi calcolati
}


int mdp_value_iteration(const MDPParams *p, int max_iter, double tol, double *V, unsigned char *policy){    // calcola il valore atteso della ricompensa (V) e la policy ottima per ciascuno stato. Ritorna il numero di iterazioni eseguite. Il parametro max_iter indica il numero massimo di iterazioni mentre tol la soglia di convergenza; entrambi governano lo stop. 
    int S = mdp_num_states(p);                                              // numero di stati
    for(int i=0; i<S; i++){ 
        V[i]=0.0;                                                           // inizializza V
        policy[i]=0;                                                        // inizializza policy                        
    }
    int reachable_states=(p->add_r1_max+1)*(p->add_r2_max+1);               // numero di stati raggiungibili da uno stato qualsiasi (per default 6*4=24)
    int *trans_idx=malloc(reachable_states*sizeof(int));                    // Lista degli stati successivi possibili (encoded).
    double *trans_p=malloc(reachable_states*sizeof(double));                // Lista delle probabilità associate agli stati successivi possibili. 
    int it=0;                                                               // contatore delle iterazioni
    for(; it<max_iter; ++it){                                               // ripete gli Update di Bellman finché non converge
        double delta=0.0;                                                   // massima variazione in questa iterazione
        for(int s=0; s<S; s++){                                             // Cicla per ogni stato   
            double best = -1e100;                                           // valore migliore trovato
            unsigned char best_a=0;                                         // azione che produce il valore migliore
            for(int a=0; a<2; a++){                                         // Loop per ciascuna delle due azioni possibili
                State scur = state_decode(p, s);                            // decodifica lo stato corrente
                int n = mdp_transitions(p, scur, a, trans_idx, trans_p);    // calcola gli stati successivi e le loro probabilità. n è il numero di stati successivi calcolati
                double q=0.0;                                               // valore atteso per azione a nello stato s
                for(int t=0; t<n; t++){                                     // cicla su tutti gli stati successivi possibili
                    State sp = state_decode(p, trans_idx[t]);               // decodifica lo stato successivo
                    int r = mdp_reward(p, sp);                              // calcola il reward per lo stato successivo
                    q += trans_p[t] * (r + p->gamma * V[ trans_idx[t] ]);   // somma il contributo di questo stato successivo al valore atteso
                }
                if(q>best){ 
                    best=q;                                                 // aggiorna il valore migliore trovato
                    best_a=(unsigned char)a;                                // aggiorna l'azione che produce il valore migliore
                }
            }
            double old = V[s];                                              // salva il vecchio valore per calcolare la variazione
            V[s]=best;                                                      // aggiorna il valore dello stato con il valore migliore trovato
            policy[s]=best_a;                                               // aggiorna la policy con l'azione che produce il valore migliore
            double d = old>best? old-best : best-old;                       // calcola la variazione (in valore assoluto)
            if(d>delta) delta=d;                                            // aggiorna la massima variazione in questa iterazione
        }
        if(delta<tol) break;                                                // se la massima variazione è sotto la soglia, esci dal ciclo
    }   
    free(trans_p); free(trans_idx);
    return it;                                                              // ritorna il numero di iterazioni eseguite
}

int mdp_simulate(const MDPParams *p, State s, const unsigned char *policy, int steps, unsigned int *rng_state, int *snapshotAuto, int *snaphotReward){     // simula l'evoluzione della CdM per un certo numero di passi seguendo la policy data. Ritorna il reward cumulato.
    int R=0;                                                                // reward cumulato
    int ctr=0;                                                              // counter per snapshots
    for(int t=1; t<steps+1; t++){                                           // cicla per ciascun passo di simulazione
        int idx = state_encode(p,s);                                        // codifica lo stato corrente in un indice
        int a = policy[idx];                                                // seleziona l'azione secondo la policy
        unsigned int r1 = xorshift32(rng_state);                            // genera due numeri pseudocasuali per calcolare le nuove auto che arrivano
        unsigned int r2 = xorshift32(rng_state);                           
        int i = (int)(r1 % (p->add_r1_max+1));                              // nuove auto che arrivano su r1
        int j = (int)(r2 % (p->add_r2_max+1));                              // nuove auto che arrivano su r2
        State sp;                                                           // stato successivo
        if(a==0){
            int pass = s.n1<p->out_r1_max? s.n1:p->out_r1_max;              // massimo 3 auto passano se TL1 è verde (equivale a min(s.n1, 3))
            sp.n1 = clamp(s.n1 - pass + i, 0, p->max_r1);
            sp.n2 = clamp(s.n2 + j, 0, p->max_r2);
            sp.g1 = 1;                                                      // segna nello stato l'informazione TL1 verde per il prossimo passo
        }else{
            int pass = s.n2<p->out_r2_max? s.n2:p->out_r2_max;               // massimo 2 auto passano se TL2 è verde (equivale a min(s.n2, 2))
            sp.n1 = clamp(s.n1 + i, 0, p->max_r1);
            sp.n2 = clamp(s.n2 - pass + j, 0, p->max_r2);
            sp.g1 = 0;                                                      // segna nello stato l'informazione TL2 verde per il prossimo passo
        }
        R += mdp_reward(p, sp);                                             // calcola il reward per lo stato successivo e lo aggiunge al cumulato
        s = sp;                                                             // passa allo stato successivo
        if(t%(steps/100)==0){                                               // salva uno snapshot ogni 1% del totale dei passi  
            snapshotAuto[ctr]=s.n1 + s.n2;                                  // salva il numero totale di auto in questo snapshot
            snaphotReward[ctr]=R;                                           // salva il reward cumulato in questo snapshot
            ctr++;
        }                                          
    }
    printf("Totale auto residue: %d\n", s.n1 + s.n2);                       // stampa il numero totale di auto residue alla fine della simulazione
    return R;                                                               // ritorna il reward cumulato
}

// implementazione Q-learning

static inline int mdp_sample_arrivals(unsigned int *rng_state, int max_add){    // Ritorna i in {0..max_add} uniforme
    unsigned int r = xorshift32(rng_state);     
    return (int)(r % (max_add+1));      
}

static inline void mdp_env_step(const MDPParams *p, State s, int action, unsigned int *rng_state, State *sp_out, int *r_out){   // Una singola transizione ambiente per Q-learning: campiona arrivi, applica azione, calcola reward.
    int i = mdp_sample_arrivals(rng_state, p->add_r1_max);
    int j = mdp_sample_arrivals(rng_state, p->add_r2_max);

    State sp;
    if(action==0){
        int pass = s.n1 < p->out_r1_max ? s.n1 : p->out_r1_max;  // TL1 verde
        sp.n1 = clamp(s.n1 - pass + i, 0, p->max_r1);
        sp.n2 = clamp(s.n2 + j, 0, p->max_r2);
        sp.g1 = 1;
    }else{
        int pass = s.n2 < p->out_r2_max ? s.n2 : p->out_r2_max;  // TL2 verde
        sp.n1 = clamp(s.n1 + i, 0, p->max_r1);
        sp.n2 = clamp(s.n2 - pass + j, 0, p->max_r2);
        sp.g1 = 0;
    }

    *r_out = mdp_reward(p, sp);
    *sp_out = sp;
}

static inline int argmax2(double q0, double q1){    // tie-breaking deterministico: preferisce azione 0 in caso di parità
    return (q1 > q0) ? 1 : 0;                       // ritorna l'indice dell'argomento massimo
}

void mdp_policy_from_Q(const MDPParams *p, const double *Q, unsigned char *policy){  // Estrae la policy greedy dalla Q-table.
    int S = mdp_num_states(p);
    for(int s=0; s<S; ++s){
        int a = argmax2(Q[s*2+0], Q[s*2+1]);
        policy[s] = (unsigned char)a;
    }
}

double mdp_q_learning(const MDPParams *p, int episodes, int steps_per_ep, unsigned int seed, double alpha, double eps_start, double eps_end, double eps_decay, double *Q, unsigned char *policy){   // Esegue l'algoritmo di Q-learning per un certo numero di episodi e passi per episodio. Ritorna il valore medio del ritorno nell'ultimo episodio.  
    int S = mdp_num_states(p);
    for(int i=0; i<S*2; ++i) Q[i] = 0.0;            // Inizializza Q a zero
    unsigned int rng = seed;                        // RNG locale
    double eps = eps_start;
    double last_avg_return = 0.0;

    for(int ep=0; ep<episodes; ++ep){
        State s = (State){ .n1=0, .n2=0, .g1=1 };   // Stato iniziale
        int G = 0;                                  // ritorno cumulato dell'episodio (somma reward)
        for(int t=0; t<steps_per_ep; ++t){
            int s_idx = state_encode(p, s);
            // ε-greedy: con prob ε scegli azione random, altrimenti greedy su Q
            int a;
            if(((double)(xorshift32(&rng)) / (double)UINT32_MAX) < eps){
                a = (xorshift32(&rng) & 1u) ? 1 : 0; // random tra {0,1}
            }else{
                a = argmax2(Q[s_idx*2+0], Q[s_idx*2+1]);
            }

            // Ambiente: una transizione campionata
            State sp; int r;
            mdp_env_step(p, s, a, &rng, &sp, &r);
            int sp_idx = state_encode(p, sp);

            // Target di Q-learning: r + gamma * max_a' Q(sp,a')
            double maxQsp = (Q[sp_idx*2+0] > Q[sp_idx*2+1]) ? Q[sp_idx*2+0] : Q[sp_idx*2+1];
            double *Qsa = &Q[s_idx*2 + a];
            *Qsa = *Qsa + alpha * (r + p->gamma * maxQsp - *Qsa);
            G += r;
            s = sp;
        }

        // Aggiorna epsilon (decadimento moltiplicativo) e clamp tra [eps_end, eps_start]
        eps = eps * eps_decay;
        if(eps < eps_end) eps = eps_end;
        if(eps > eps_start) eps = eps_start;

        last_avg_return = 0.9*last_avg_return + 0.1*(double)G;        // Semplice media mobile del ritorno per log/diagnostica
        if ((ep + 1) % 100 == 0) {
            printf("[QL][train] ep=%d/%d  eps=%.3f  return=%d  avg=%.2f\n",
                   ep + 1, episodes, eps, G, last_avg_return);
        }
    }
    
    mdp_policy_from_Q(p, Q, policy);    // Estrai la policy greedy finale

    return last_avg_return;
}

