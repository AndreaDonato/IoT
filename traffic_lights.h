
// Stato: n1 auto su r1, n2 su r2, g1=1 se TL1 verde (quindi TL2 rosso), 0 altrimenti.
typedef struct { int n1, n2, g1; } State;


typedef struct {
    int max_r1, max_r2;           // cap strade (default 40,25)
    int add_r1_max, add_r2_max;   // nuove auto massime (default 5,3)
    int out_r1_max, out_r2_max;   // auto che passano se verde (default 3,2)
    int low_th, med_th;           // soglie N: low<N<med, high>=med (default 15,30)
    double gamma;                 // sconto (default 0.95)
} MDPParams;


// Calcola la dimensione dello spazio di stato
// Al variare del numero di parametri cambia il numero di stati della CdM
int mdp_num_states(const MDPParams *p);


// Encoding/decoding dello stato in/da un indice compatto [0..S-1]
// Evita inefficienze di codice come cicli annidati
int state_encode(const MDPParams *p, State s);
State state_decode(const MDPParams *p, int idx);


// Reward della transizione verso uno stato successivo s' (dipende solo da N' = n1'+n2')
int mdp_reward(const MDPParams *p, State sp);


// Elenca le possibile transizioni da (s, a) con probabilità uniformi (24 esiti: 6×4) e ritorna il numero di next states riempiti in out_idx[], out_p[].
int mdp_transitions(const MDPParams *p, State s, int action, int out_idx[], double out_p[]);

// Value Iteration: calcola V e policy ottima (0 => set TL1 verde; 1 => set TL2 verde). max_iter e tol governano lo stop; ritorna iterazioni eseguite.
int mdp_value_iteration(const MDPParams *p, int max_iter, double tol, double *V, unsigned char *policy);

// Simulazione seguendo la policy per T passi; ritorna reward cumulato.
int mdp_simulate(const MDPParams *p, State s0, const unsigned char *policy, int steps, unsigned int *rng_state, int *TotAuto);