// Stato: n1 auto su r1, n2 su r2, g1=1 se TL1 verde (quindi TL2 rosso), 0 altrimenti.
typedef struct { int n1, n2, g1; } State;

typedef struct {
    int max_r1, max_r2;           // cap strade (default 40,25)
    int add_r1_max, add_r2_max;   // nuove auto massime (default 5,3)
    int out_r1_max, out_r2_max;   // auto che passano se verde (default 3,2)
    int low_th, med_th;           // soglie N: low<N<med, high>=med (default 15,30)
    double gamma;                 // sconto (default 0.95)
} MDPParams;


// Stampa il progresso dei cicli
void print_progress(double progress);

// --- AGGIUNTA: prototipi Q-learning ---

// Allena una Q-table con Q-learning a orizzonte infinito scontato.
// Q ha dimensione mdp_num_states(p)*2 (due azioni).
// Restituisce il reward medio per episodio.
double mdp_q_learning(const MDPParams *p,
                      State *s,
                      int multiSim,          // flag per output dettagliato in training Q-learning
                      int steps_per_ep,      // passi per episodio
                      unsigned int seed,     // seed RNG
                      double alpha,          // learning rate
                      double eps_start,      // epsilon iniziale (ε-greedy)
                      double eps_end,        // epsilon finale
                      double eps_decay,      // moltiplicatore per eps a fine episodio (e.g. 0.995)
                      double *Q,             // out: tabella Q (S*2)
                      unsigned int *N,       // out: contatore visite (S*2)
                      int *snapshotAutoN1, int *snapshotAutoN2, int *snaphotReward, int *snapshopTime,
                      int *G_start,          // optional in/out cumulative reward (if non-NULL continue from *G_start and write back)
                      int h                  // fascia oraria                 
                    );

// Estrae la policy greedy dalla Q-table.
void mdp_policy_from_Q(const MDPParams *p, const double *Q, unsigned char *policy);

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
int mdp_transitions(const MDPParams *p, State s, int a, int out_idx[], double out_p[]);

// Value Iteration: calcola V e policy ottima (0 => set TL1 verde; 1 => set TL2 verde). max_iter e tol governano lo stop; ritorna iterazioni eseguite.
int mdp_value_iteration(const MDPParams *p, int max_iter, double tol, double *V, unsigned char *policy);

// Simulazione seguendo la policy per T passi; ritorna reward cumulato.
int mdp_simulate(const MDPParams *p, State s0, const unsigned char *policy, int steps, unsigned int *rng_state, int *snapshotAutoN1, int *snapshotAutoN2, int *snaphotReward, int *snapshopTime);

// Modifica i parametri MDP in base alla fascia oraria corrente (hours totale, j fascia corrente)
void adjust_params_for_hour(const MDPParams Input, MDPParams *P, int hours, int j);

int static_simulate(const MDPParams *p, State s, int staticPolicy, int steps, unsigned int *rng_state, int *snapshotAutoN1, int *snapshotAutoN2, int *snaphotReward, int *snapshopTime);

static inline int genius_mdp_env_step(const MDPParams *p, State s, int action, unsigned int *rng_state, State *sp_out, int *r_out, int block);

int geniusDriversBlock(int a, int consecutive, int blocked, const MDPParams *params, int blockedSteps);

double geniusDrivers_q_learning(const MDPParams *p, State *s, int multiSim, int steps, unsigned int seed, double alpha, double eps_start, double eps_end, double eps_decay, double *Q, unsigned int *N, int *snapshotAutoN1, int *snapshotAutoN2, int *snaphotReward, int *snapshopTime, int *G_start, int h);