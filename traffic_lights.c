#include <stdlib.h>
#include <stdio.h>
#include <stdint.h> 
#include <math.h>
#include "traffic_lights.h"





/***************************************
****** Funzioni Utility Generiche ******
***************************************/


// Una volta aggiornata la lunghezza delle code, serve ad assicurarsi che il risultato non sia mai < 0 o > N_max
static inline int clamp(int x, int a, int b){ return x<a?a: (x>b?b:x); }


// Dati i parametri del problema, restituisce il numero di stati della CdM associata
int mdp_num_states(const MDPParams *p){
    return (p->max_r1+1)*(p->max_r2+1)*2;
}


// Funzioni di encode e decode per gestire in modo più efficiente la rappresentazione dello stato
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


// Dato uno stato e i parametri scelti dall'utente (i.e. le definizioni di cos'è low, medium e high traffic) restituisce il Reward associato
int mdp_reward(const MDPParams *p, State sp){
    int N = sp.n1 + sp.n2;
    if (N < p->low_th) return +1;
    if (N < p->med_th) return 0;
    return -1;
}


// Generatore di numeri pseudocasuali (deterministico) su qualsiasi piattaforma, dato un seed di input
static inline unsigned int xorshift32(unsigned int *st){
    unsigned int x=*st; 
    x ^= x<<13; 
    x ^= x>>17; 
    x ^= x<<5; 
    return *st=x;
}


// Stampa il progresso dei cicli
void print_progress(double progress) {
    int barWidth = 50;
    int pos = (int)(barWidth * progress);

    printf("\r[");  // \r in testa, così riscrive la riga intera
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) printf("=");
        else if (i == pos) printf(">");
        else printf(" ");
    }
    printf("] %3.0f%%", progress * 100.0);
    fflush(stdout);
}

/**********************************************
****** Time-Based Synchronization Policy ******
**********************************************/
//
// Questa sezione definisce una policy non basata sulle osservazioni: i semafori switchano ogni t steps
//
//
// Da scrivere per avere il benchmark "caso peggiore"
//


/*******************************************
****** Static Markov Decision Process ******
*******************************************/
//
// Questa sezione di codice calcola la soluzione ottimale del problema statico, dati i parametri
// Questo è il benchmark "caso migliore"
//

// Dati uno stato s e un'azione a costruisce l'elenco di tutti i possibili stati successivi con relative probabilità.
// Più efficiente di costruire l'intera matrice di transizione.
// Action: 0 => TL1 green; 1 => TL2 green
int mdp_transitions(const MDPParams *p, State s, int a, int out_idx[], double out_p[]){
    int k=0;
    for(int i=0; i<=p->add_r1_max; i++){                                      // cicla su tutte le possibili nuove auto che arrivano
        for(int j=0; j<=p->add_r2_max; j++){
            State sp;
            if(a==0){
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



// Risolve il MDP statico e trova la soluzione ottimale sottoforma di policy
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
                } else if (q == best) {
                    /* tie-break: prefer the action that gives green to the longer queue */
                    best_a = (scur.n1 >= scur.n2) ? 0 : 1;
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


//
// Date le condizioni iniziali e la policy ottimale del MDP statico, simula una situazione reale e costruisce l'andamento dei parametri nel tempo
//
int mdp_simulate(const MDPParams *p, State s, const unsigned char *policy, int steps, unsigned int *rng_state, int *snapshotAutoN1, int *snapshotAutoN2, int *snaphotReward, int *snapshopTime){     // simula l'evoluzione della CdM per un certo numero di passi seguendo la policy data. Ritorna il reward cumulato.
    int R=0;                                                                // reward cumulato
    int ctr=0;                                                              // counter per snapshots
    for(int t=0; t<steps+1; t++){                                           // cicla per ciascun passo di simulazione
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
            /*
             * Protezione: per alcuni valori di `steps` (ad es. steps=2250 quando hours=4)
             * l'espressione `t % (steps/100) == 0` può risultare true più di 100 volte,
             * causando scritture fuori dall'array di snapshot (overflow). Qui limitiamo
             * il numero di snapshot a 100 per chiamata (ctr < 100).
             */
            if(ctr < 100) {
                snapshotAutoN1[ctr]+=s.n1;                                   // salva il numero totale di auto in questo snapshot
                snapshotAutoN2[ctr]+=s.n2;                                   // salva il numero totale di auto in questo snapshot
                snaphotReward[ctr]+=R;                                       // salva il reward cumulato in questo snapshot
                snapshopTime[ctr]=t;                                        // salva il tempo (passi) in questo snapshot
                ctr++;
            }
        }
    }
    return R;                                                               // ritorna il reward cumulato finale
}





/***************************************
****** Q-learning & MDP Dinamico *******
***************************************/

//
// Questa sezione di codice implementa il Q-Learning su un singolo problema statico, consentendo però di generalizzare in modo "smooth" al caso dinamico
// Applicando in sequenza la funzione mdp_q_learning, le si può dare via via in input lo stato a partire dal quale l'algoritmo inizia a imparare dinamicamente le nuove azioni ottimali 
// Questa è una condizione realistica, che a livello di performance si piazza in mezzo ai due benchmark
//


// Estrae in modo uniforme un numero da 0 a max_add di nuove macchine
static inline int mdp_sample_arrivals(unsigned int *rng_state, int max_add){
    unsigned int r = xorshift32(rng_state);     
    return (int)(r % (max_add+1));      
}


// Un singolo step per il problema di Q-learning: estrae gli arrivi, applica l'azione e calcola il reward, restituendo lo stato successivo
static inline void mdp_env_step(const MDPParams *p, State s, int action, unsigned int *rng_state, State *sp_out, int *r_out){
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

    *r_out = mdp_reward(p, sp);     // calcola il reward
    *sp_out = sp;             // output stato successivo
}

// tie-breaking: preferisce l'azione che dà verde alla corsia più lunga in caso di parità
static inline int argmax2(double q0, double q1, State st){
    if(q1 > q0) return 1;
    if(q0 > q1) return 0;
    // tie: prefer the action that gives green to the longer queue
    // action 0 -> TL1 green (helps n1); action 1 -> TL2 green (helps n2)
    return (st.n1 >= st.n2) ? 0 : 1;
}

// ###############################################################################################################################
// Mi sa che questa funzione non la usaremo più
void mdp_policy_from_Q(const MDPParams *p, const double *Q, unsigned char *policy){  // Estrae la policy greedy dalla Q-table.
    int S = mdp_num_states(p);
    for(int s=0; s<S; ++s){
        State st = state_decode(p, s);
        int a = argmax2(Q[s*2+0], Q[s*2+1], st);
        policy[s] = (unsigned char)a;
    }
}
// ###############################################################################################################################



// Applica steps volte un singolo passo della simulazione, aggiornando via via tutti i parametri del problema di Q-Learning e costruendo l'andamento dei parametri nel tempo
double mdp_q_learning(const MDPParams *p, State *s, int multiSim, int steps, unsigned int seed, double alpha, double eps_start, double eps_end, double eps_decay, double *Q, unsigned int *N, int *snapshotAutoN1, int *snapshotAutoN2, int *snaphotReward, int *snapshopTime, int *G_start, int h){   // Esegue l'algoritmo di Q-learning per un certo numero di episodi e passi per episodio. Ritorna il valore medio del ritorno nell'ultimo episodio.  
   
    
    int S = mdp_num_states(p);
    unsigned int rng = seed;                        // RNG locale
    double eps = eps_start;
    double last_avg_return = 0.0;
    int ctr=0;                                      // counter per snapshots
    int G = (G_start != NULL) ? *G_start : 0;       // ritorno cumulato dell'episodio (somma reward), continua da G_start se fornito
    int c=10;                                       // Parametro per il decadimento armonico di epsilon

    FILE *fout  = fopen("QLoutput.txt",  "w");
    for(int t=0; t<steps; ++t){
        int s_idx = state_encode(p, *s);
        // ε-greedy: con prob ε scegli azione random, altrimenti greedy su Q
        int a;
        if(((double)(xorshift32(&rng)) / (double)UINT32_MAX) < eps){
            a = (xorshift32(&rng) & 1u) ? 1 : 0; // random tra {0,1}
        }else{
            a = argmax2(Q[s_idx*2+0], Q[s_idx*2+1], *s);
        }
        // Ambiente: una transizione campionata
        State sp; int r;
        mdp_env_step(p, *s, a, &rng, &sp, &r);
        int sp_idx = state_encode(p, sp);
        // Target di Q-learning: r + gamma * max_a' Q(sp,a')
        double maxQsp = (Q[sp_idx*2+0] > Q[sp_idx*2+1]) ? Q[sp_idx*2+0] : Q[sp_idx*2+1];
        int idx_sa = s_idx * 2 + a;
        N[idx_sa]++;
        double alpha_sa = 1.0 / pow((double)N[idx_sa], 0.6);
        double *Qsa = &Q[idx_sa];
        *Qsa = *Qsa + alpha_sa * (r + p->gamma * maxQsp - *Qsa);
        G += r;
        *s = sp;
        // Aggiorna epsilon
        //eps = fmax(eps_end, eps * eps_decay);
        eps = fmax(eps_end, (double)c/(c+t)); // decay più lento
        last_avg_return = 0.9*last_avg_return + 0.1*(double)G;        // Semplice media mobile del ritorno per log/diagnostica

        int index = 0, tmp = -1;

        if ((t) % 100 == 0 && multiSim==0) {
            printf("[QL][train] ep=%d/%d  eps=%.3f  return=%d  avg=%.2f\n", t + 1, steps, eps, G, last_avg_return);
            if(fout) fprintf(fout, "%d %d %d %d\n", t, s->n1, s->n2, G);
        }
        else if(t%(steps/100)==0){                                               // salva uno snapshot ogni 1% del totale dei passi
            int base = (h>=0) ? (100*h) : 0;
            /* Protezione: non scrivere più di 100 snapshot per fascia oraria */
            if(ctr < 100) {
                index = base+ctr;
                if(index == tmp) printf("Indice %d duplicato!\n", tmp);
                snapshotAutoN1[index]+= s->n1;                                   // salva il numero totale di auto in questo snapshot
                snapshotAutoN2[index]+= s->n2;                                   // salva il numero totale di auto in questo snapshot
                snaphotReward[index]+=G;                                          // salva il reward cumulato in questo snapshot
                snapshopTime[index]=t + steps * h;                                // salva il tempo (passi) in questo snapshot
                tmp = index;
                ctr++;
            }
        }
    }
    if(G_start != NULL) *G_start = G; // write back cumulative reward
    if(fout) fclose(fout);
    return last_avg_return;
}

/***********************************************************
****** Fasce Orarie, Macchine in mezzo all'incrocio  *******
***********************************************************/

//
// Questa sezione di codice aggiunge complicazioni e imprevisti alla simulazione base, implementando distribuzioni dinamiche e gente che passa quando non dovrebbe
//

// Cambia i parametri dell'MDP in base alla fascia oraria corrente.
// hours: numero totale di fasce in cui è divisa la giornata
// j: indice della fascia corrente (1-based)
// La funzione modifica in-place i campi rilevanti di P (arrivi) per simulare variazioni orarie.
void adjust_params_for_hour(const MDPParams Input, MDPParams *P, int hours, int j){
    if(hours <= 1) return;                                                                                              // Ignora le modifiche se non ci sono fasce multiple
    switch(hours){
        case 2:                                                                             
            if     (j==1){ P->add_r1_max = floor(Input.add_r1_max*1.5) ; P->add_r2_max = floor(Input.add_r2_max*1.5) ; } // Giorno (+50% traffico rispetto al valore impostato)
            else         { P->add_r1_max = ceil (Input.add_r1_max*0.5) ; P->add_r2_max = ceil (Input.add_r2_max*0.5) ; } // Notte (-50%)
            break;
        case 3:                                                                             
            if     (j==1){ P->add_r1_max = ceil (Input.add_r1_max*0.3) ; P->add_r2_max = ceil (Input.add_r1_max*0.3) ; } // 22-06 (-70%)
            else if(j==2){ P->add_r1_max = floor(Input.add_r1_max*1.4) ; P->add_r2_max = floor(Input.add_r1_max*1.4) ; } // 06-14 (+40%)
            else         { P->add_r1_max = floor(Input.add_r1_max*1.3) ; P->add_r2_max = floor(Input.add_r1_max*1.3) ; } // 14-22 (+30%)
            break;
         case 4:                                                                            
            if     (j==1){ P->add_r1_max = ceil (Input.add_r1_max*0.2) ; P->add_r2_max = ceil (Input.add_r1_max*0.2) ; } // 22-04 (-80%)
            else if(j==2){ P->add_r1_max = floor(Input.add_r1_max*1.4) ; P->add_r2_max = floor(Input.add_r1_max*1.4) ; } // 04-10 (+40%)
            else if(j==3){ P->add_r1_max =       Input.add_r1_max      ; P->add_r2_max =       Input.add_r1_max      ; } // 10-16 (+ 0%)
            else         { P->add_r1_max = floor(Input.add_r1_max*1.4) ; P->add_r2_max = floor(Input.add_r1_max*1.4) ; } // 16-22 (+40%)
            break;
        case 6:                                                                             
            if     (j==1){ P->add_r1_max = ceil (Input.add_r1_max*0.2) ; P->add_r2_max = ceil (Input.add_r1_max*0.2) ; } // 22-02 (-80%)
            else if(j==2){ P->add_r1_max = ceil (Input.add_r1_max*0.4) ; P->add_r2_max = ceil (Input.add_r1_max*0.4) ; } // 02-06 (-60%)
            else if(j==3){ P->add_r1_max = floor(Input.add_r1_max*1.6) ; P->add_r2_max = floor(Input.add_r1_max*1.6) ; } // 06-10 (+60%) 
            else if(j==4){ P->add_r1_max = floor(Input.add_r1_max*1.1) ; P->add_r2_max = floor(Input.add_r1_max*1.1) ; } // 10-14 (+10%)
            else if(j==5){ P->add_r1_max = floor(Input.add_r1_max*1.4) ; P->add_r2_max = floor(Input.add_r1_max*1.4) ; } // 14-18 (+30%)
            else         { P->add_r1_max = floor(Input.add_r1_max*1.4) ; P->add_r2_max = floor(Input.add_r1_max*1.4) ; } // 18-22 (+20%)
            break;
        default:
            printf("adjust_params_for_hour: unsupported number of hours (allowed values: 1, 2, 3, 4, 6): %d\n", hours);
            exit(1);
    }
}
