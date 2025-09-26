#include <stdlib.h>
#include "traffic_lights.h"

static inline int clamp(int x, int a, int b){ return x<a?a: (x>b?b:x); }    // assicura che x sia in [a,b]

int mdp_num_states(const MDPParams *p){     //conta il numero di stati
    return (p->max_r1+1)*(p->max_r2+1)*2;
}

int state_encode(const MDPParams *p, State s){   // appiattisce l'array 3D dello stato in un indice
    int W = p->max_r2 + 1;
    int base = s.n1 * W + s.n2;   // indice per la coppia (n1,n2)
    int idx  = base * 2 + (s.g1 ? 1 : 0); // moltiplica per 2 e aggiungi g1
    return idx;
}

State state_decode(const MDPParams *p, int idx){  // ricostruisce lo stato a partire dallo stato appiattito
    State s;
    s.g1 = idx % 2;        // ultimo “slot” dice quale semaforo è verde
    idx /= 2;              // rimuovi quel bit
    int W = p->max_r2 + 1;
    s.n1 = idx / W;
    s.n2 = idx % W;
    return s;
}

int mdp_reward(const MDPParams *p, State sp){       // funzione di calcolo del reward
    int N = sp.n1 + sp.n2;
    if (N < p->low_th) return +1;
    if (N < p->med_th) return 0;
    return -1;
}

static inline unsigned int xorshift32(unsigned int *st){    // generatore di numeri pseudocasuali deterministico su qualsiasi piattaforma
    unsigned int x=*st; 
    x ^= x<<13; 
    x ^= x>>17; 
    x ^= x<<5; 
    return *st=x;
}

int mdp_transitions(const MDPParams *p, State s, int action, int out_idx[], double out_p[]){    // costruisce l'elenco di tutti gli stati successivi possibili e la loro probabilità a partire dallo stato s e dall'azione action. Più efficiente di costruire l'intera matrice di transizione.
    // action: 0 => TL1 green; 1 => TL2 green
    int k=0;
    for(int i=0;i<=p->add_r1_max;i++){      // cicla su tutte le possibili nuove auto che arrivano
        for(int j=0;j<=p->add_r2_max;j++){
            State sp;
            if(action==0){
                int pass = s.n1<3? s.n1:3;      // massimo 3 auto passano se TL1 è verde
                sp.n1 = clamp(s.n1 - pass + i, 0, p->max_r1);
                sp.n2 = clamp(s.n2 + j, 0, p->max_r2);
                sp.g1 = 1;  // segna nello stato l'informazione TL1 verde
            }else{
                int pass = s.n2<2? s.n2:2;      // massimo 2 auto passano se TL2 è verde
                sp.n1 = clamp(s.n1 + i, 0, p->max_r1);
                sp.n2 = clamp(s.n2 - pass + j, 0, p->max_r2);
                sp.g1 = 0;  // segna nello stato l'informazione TL2 verde
            }
            out_idx[k] = mdp_encode(p, sp);     // codifica lo stato succesivo calcolato in un indice intero
            out_p[k]   = 1.0/((p->add_r1_max+1)*(p->add_r2_max+1));     // probabilità uniforme su tutte le combinazioni di nuove auto
            k++;
        }
    }
    return k;   // ritorna il numero di stati successivi calcolati
}