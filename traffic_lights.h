

// Stato: n1 auto su r1, n2 su r2, g1=1 se TL1 verde (quindi TL2 rosso), 0 altrimenti.
typedef struct { int n1, n2, g1; } State;

typedef struct {
    int max_r1, max_r2;           // cap strade (default 40,25)
    int add_r1_max, add_r2_max;   // nuove auto massime (default 5,3)
    int low_th, med_th;           // soglie N: low<N<med, high>=med (default 15,30)
    double gamma;                 // sconto (default 0.95)
} MDPParams;

// Dimensione spazio di stato
int mdp_num_states(const MDPParams *p);

// Encoding/decoding stato <-> indice compatto [0..S-1]
int mdp_encode(const MDPParams *p, State s);
State mdp_decode(const MDPParams *p, int idx);
