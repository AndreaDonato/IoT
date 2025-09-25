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