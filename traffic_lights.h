
// Stato: n1 auto su r1, n2 su r2, g1=1 se TL1 verde (quindi TL2 rosso), 0 altrimenti.
typedef struct { int n1, n2, g1; } State;


typedef struct {
    int max_r1, max_r2;           // cap strade (default 40,25)
    int add_r1_max, add_r2_max;   // nuove auto massime (default 5,3)
    int low_th, med_th;           // soglie N: low<N<med, high>=med (default 15,30)
    double gamma;                 // sconto (default 0.95)
} MDPParams;


// Calcola la dimensione dello spazio di stato
// Al variare del numero di parametri cambia il numero di stati della CdM
int mdp_num_states(const MDPParams *p);


// Encoding/decoding dello stato in/da un indice compatto [0..S-1]
// Evita inefficienze di codice come cicli annidati
int mdp_encode(const MDPParams *p, State s);
State mdp_decode(const MDPParams *p, int idx);


// Reward della transizione verso uno stato successivo s' (dipende solo da N' = n1'+n2')
int mdp_reward(const MDPParams *p, State sp);


// Elenca le possibile transizioni da (s, a) con probabilità uniformi (24 esiti: 6×4) e ritorna il numero di next states riempiti in out_idx[], out_p[].
int mdp_transitions(const MDPParams *p, State s, int action, int out_idx[], double out_p[]);