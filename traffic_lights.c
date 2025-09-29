#include <stdlib.h>
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
                int pass = s.n1<3? s.n1:3;                                  // massimo 3 auto passano se TL1 è verde
                sp.n1 = clamp(s.n1 - pass + i, 0, p->max_r1);
                sp.n2 = clamp(s.n2 + j, 0, p->max_r2);
                sp.g1 = 1;                                                  // segna nello stato l'informazione TL1 verde
            }else{
                int pass = s.n2<2? s.n2:2;                                  // massimo 2 auto passano se TL2 è verde
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
    int trans_idx[32];                                                      // Lista degli stati successivi possibili (encoded). 6*4=24 al default, ma allochiamo 32 per sicurezza.
    double trans_p[32];                                                     // Lista delle probabilità associate agli stati successivi possibili. 
    int it=0;                                                               // contatore delle iterazioni
    for(; it<max_iter; ++it){                                               // ripete gli Update di Bellman finché non converge
        double delta=0.0;                                                   // massima variazione in questa iterazione
        for(int s=0; s<S; s++){                                             // Cicla per ogni stato   
            double best = -1e100;                                           // valore migliore trovato
            unsigned char best_a=0;                                         // azione che produce il valore migliore
            for(int a=0; a<2; a++){                                         // Loop per ciascuna delle due azioni possibili
                State scur = state_decode(p, s);                              // decodifica lo stato corrente
                int n = mdp_transitions(p, scur, a, trans_idx, trans_p);    // calcola gli stati successivi e le loro probabilità. n è il numero di stati successivi calcolati
                double q=0.0;                                               // valore atteso per azione a nello stato s
                for(int t=0; t<n; t++){                                     // cicla su tutti gli stati successivi possibili
                    State sp = state_decode(p, trans_idx[t]);                 // decodifica lo stato successivo
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
    return it;                                                              // ritorna il numero di iterazioni eseguite
}

int mdp_simulate(const MDPParams *p, State s, const unsigned char *policy, int steps, unsigned int *rng_state){     // simula l'evoluzione della CdM per un certo numero di passi seguendo la policy data. Ritorna il reward cumulato.
    int R=0;                                                                // reward cumulato
    for(int t=0; t<steps; t++){                                             // cicla per ciascun passo di simulazione
        int idx = state_encode(p,s);                                          // codifica lo stato corrente in un indice
        int a = policy[idx];                                                // seleziona l'azione secondo la policy
        unsigned int r1 = xorshift32(rng_state);                            // genera due numeri pseudocasuali per calcolare le nuove auto che arrivano
        unsigned int r2 = xorshift32(rng_state);                           
        int i = (int)(r1 % (p->add_r1_max+1));                              // nuove auto che arrivano su r1
        int j = (int)(r2 % (p->add_r2_max+1));                              // nuove auto che arrivano su r2
        State sp;                                                           // stato successivo
        if(a==0){
            int pass = s.n1<3? s.n1:3;                                      // massimo 3 auto passano se TL1 è verde (equivale a min(s.n1, 3))
            sp.n1 = clamp(s.n1 - pass + i, 0, p->max_r1);
            sp.n2 = clamp(s.n2 + j, 0, p->max_r2);
            sp.g1 = 1;                                                      // segna nello stato l'informazione TL1 verde per il prossimo passo
        }else{
            int pass = s.n2<2? s.n2:2;                                      // massimo 2 auto passano se TL2 è verde (equivale a min(s.n2, 2))
            sp.n1 = clamp(s.n1 + i, 0, p->max_r1);
            sp.n2 = clamp(s.n2 - pass + j, 0, p->max_r2);
            sp.g1 = 0;                                                      // segna nello stato l'informazione TL2 verde per il prossimo passo
        }
        R += mdp_reward(p, sp);                                             // calcola il reward per lo stato successivo e lo aggiunge al cumulato
        s = sp;                                                             // passa allo stato successivo
    }
    return R;                                                               // ritorna il reward cumulato
}
