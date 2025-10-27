#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "traffic_lights.h"

static void usage(const char* prog){
    fprintf(stderr,
      "Uso: %s [max_r1 max_r2 add_r1_max add_r2_max out_r1_max out_r2_max low_th med_th gamma steps seed simulations]\n"
      "Esempio (stato default): %s\n", prog, prog);
}

int main(int argc, char const *argv[])
{
    // Inizializzazione dei parametri MDP
    MDPParams P = {
        .max_r1=40, .max_r2=25,
        .add_r1_max=5, .add_r2_max=3,
        .out_r1_max=3, .out_r2_max=2,
        .low_th=15, .med_th=30,
        .gamma=0.95
    };
    int steps=1000000;                     // numero di passi di simulazione
    unsigned int seed=12345;            // seed per il generatore di numeri pseudocasuali
    int simulations=10;                 // numero di simulazioni per valutare la policy

    State state={ .n1=0, .n2=0, .g1=0 };  // stato iniziale

    FILE *foutVI  = fopen("outputVI.txt",  "w");
    FILE *foutQL  = fopen("outputQL.txt",  "w");

    // Leggi i parametri da linea di comando
    if(argc>=3){ P.max_r1=atoi(argv[1]); P.max_r2=atoi(argv[2]); }
    if(argc>=5){ P.add_r1_max=atoi(argv[3]); P.add_r2_max=atoi(argv[4]);}
    if(argc>=7){ P.out_r1_max=atoi(argv[5]); P.out_r2_max=atoi(argv[6]);}
    if(argc>=9){ P.low_th=atoi(argv[7]); P.med_th=atoi(argv[8]); }
    if(argc>=10){ P.gamma=atof(argv[9]); }
    if(argc>=11){ steps=atoi(argv[10]); }
    if(argc>=12){ seed=(unsigned int)strtoul(argv[11],NULL,10); }
    if(argc>=13){ simulations=atoi(argv[12]); }
    if(argc==2){ usage(argv[0]); return 1; }

    // VALUE ITERATION 

    int S = mdp_num_states(&P);                                         // numero di stati                      
    double *V = (double*)calloc(S, sizeof(double));                     // valore di ogni stato inizializzato a 0
    unsigned char *Pi_VI = (unsigned char*)malloc(S);                   // policy ottima (0 => TL1 verde; 1 => TL2 verde)
    int it = mdp_value_iteration(&P, 1000, 1e-6, V, Pi_VI);             // calcola il valore di ciascuno stato e la relativa policy ottima con Value Iteration
    printf("Value Iteration: %d iterazioni, S=%d stati\n", it, S);
    //fprintf(fout, "Parametri: max_r1=%d, max_r2=%d, add_r1_max=%d, add_r2_max=%d, out_r1_max=%d, out_r2_max=%d, low_th=%d, med_th=%d, gamma=%.2f, steps=%d, seed=%u, simulations=%d, iterations=%d, tot_states=%d\n", P.max_r1, P.max_r2, P.add_r1_max, P.add_r2_max, P.out_r1_max, P.out_r2_max, P.low_th, P.med_th, P.gamma, steps, seed, simulations, it, S);

    // Q-LEARNING

    // Iperparametri base (puoi cambiare liberamente):
    double alpha      = 0.1;      // learning rate
    double eps_start  = 0.9;     // esplorazione iniziale
    double eps_end    = 0.02;     // esplorazione minima
    double eps_decay  = 0.995;    // decadimento per episodio

    double *Q = (double*)calloc(S*2, sizeof(double));                    // Q-table (S stati, 2 azioni)
    unsigned char *Pi_ql = (unsigned char*)malloc(S);                    // policy greedy da Q-learning

    //double avg_ret = mdp_q_learning(&P, state, 0, steps_per_ep, 987654u, alpha, eps_start, eps_end, eps_decay, Q, Pi_ql); // allena e produce policy greedy
    //printf("Q-learning: episodes=%d, steps/ep=%d, alpha=%.3f, eps=%.2f->%.2f (x%.3f), avg_return~%.2f\n", episodes, steps_per_ep, alpha, eps_start, eps_end, eps_decay, avg_ret);
    //fprintf(fout, "Q-learning: episodes=%d, steps_per_ep=%d, alpha=%.3f, eps_start=%.2f, eps_end=%.2f, eps_decay=%.3f, avg_return~%.2f\n", episodes, steps_per_ep, alpha, eps_start, eps_end, eps_decay, avg_ret);

    // Valutazione/Confronto in simulazione
    int *N1=(int*)calloc(100, sizeof(int));         // array per snapshot numero auto r1
    int *N2=(int*)calloc(100, sizeof(int));         // array per snapshot numero auto r2
    int *R=(int*)calloc(100, sizeof(int));          // array per snapshot reward cumulato
    int *T=(int*)calloc(100, sizeof(int));          // array per snapshot tempo

    State s0 = (State){ .n1=0, .n2=0, .g1=1 };      // stato iniziale di simulazione (strade vuote, TL1 verde)
    // State s0 = (State){ .n1=P.max_r1/2, .n2=P.max_r2/2, .g1=1 };         // stato iniziale di simulazione (metà capacità su entrambe le strade, TL1 verde)

    // 1) Simula policy da Value Iteration (baseline)
    for(int i=0; i<simulations; i++){
        unsigned int s=seed+i;                      // copia il seed per non modificare l'originale
        int Rmax = mdp_simulate(&P, s0, Pi_VI, steps, &s, N1, N2, R, T);
        printf("[VI] Reward cumulato=%d\n", Rmax);
    }
    for(int j=0;j<100;j++){ 
        fprintf(foutVI, "%d, %.1f, %.1f, %.1f \n", T[j], (double)N1[j]/simulations, (double)N2[j]/simulations, (double)R[j]/simulations); 
    }
    /* reset snapshot arrays (they have 100 elements each) - sizeof(N1) would give size of pointer */
    memset(N1, 0, 100 * sizeof(int));
    memset(N2, 0, 100 * sizeof(int));
    memset(R,  0, 100 * sizeof(int));
    memset(T,  0, 100 * sizeof(int));
    // 2) Simula policy appresa con Q-learning
    for(int i=0; i<simulations; i++){
        unsigned int s=seed+1000+i;
        int Rmax = mdp_q_learning(&P, state, 1, steps, 987654u, alpha, eps_start, eps_end, eps_decay, Q, Pi_ql, N1, N2, R, T);
        printf("[QL] Reward cumulato=%d\n", Rmax);      
    }
    for(int j=0;j<100;j++){ 
        fprintf(foutQL, "%d, %.1f, %.1f, %.1f \n", T[j], (double)N1[j]/simulations, (double)N2[j]/simulations, (double)R[j]/simulations); 
    }
    free(V); free(Pi_VI);
    free(Q); free(Pi_ql);
    free(N1); free(N2);
    free(R); free(T);

    return 0;
}