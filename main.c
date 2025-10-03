#include <stdio.h>
#include <stdlib.h>
#include "traffic_lights.h"

static void usage(const char* prog){
    fprintf(stderr,
      "Uso: %s [max_r1 max_r2 add_r1_max add_r2_max out_r1_max out_r2_max low_th med_th gamma steps seed simulations]\n"
      "Esempio (stato default): %s\n", prog, prog);
}

int main(int argc, char const *argv[])
{

	// Inizializzazione dei parametri
	MDPParams P = {
        .max_r1=40, .max_r2=25,
        .add_r1_max=5, .add_r2_max=3,
        .out_r1_max=3, .out_r2_max=2,
        .low_th=15, .med_th=30,
        .gamma=0.95
    };
	int steps=1000000;							                                // numero di passi di simulazione
    unsigned int seed=12345;                                                    // seed per il generatore di numeri pseudocasuali
    int simulations=10;				                                            // seed per il generatore di numeri pseudocasuali

    FILE *foutN = fopen("outputN.txt", "w");
    FILE *foutR = fopen("outputR.txt", "w");
    FILE *fout = fopen("output.txt", "w");

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
   

    int S = mdp_num_states(&P);                                             // numero di stati
    double *V = (double*)calloc(S, sizeof(double));                         // valore di ogni stato inizializzato a 0
    unsigned char *Pi = (unsigned char*)malloc(S);                          // policy ottima (0 => TL1 verde; 1 => TL2 verde)
    int it = mdp_value_iteration(&P, 1000, 1e-6, V, Pi);                    // calcola il valore di ciascuno stato e la relativa policy ottima con Value Iteration
    printf("Value Iteration: %d iterazioni, S=%d stati\n", it, S);
    fprintf(fout, "max_r1: %d, max:r2: %d, add_r1_max: %d, add_r2_max: %d, out_r1_max: %d, out_r2_max: %d, low_th: %d, med_th: %d, gamma: %.2f, steps: %d, seed: %u, simulations: %d, iterations: %d, tot_states: %d\n", P.max_r1, P.max_r2, P.add_r1_max, P.add_r2_max, P.out_r1_max, P.out_r2_max, P.low_th, P.med_th, P.gamma, steps, seed, simulations, it, S);
    
    int *N=(int*)calloc(100, sizeof(int));                                  // array per salvare il numero totale di auto in ciascuno snapshot (100 snapshot totali)
    int *R=(int*)calloc(100, sizeof(int));

    State s0 = (State){ .n1=0, .n2=0, .g1=1 };                              // stato iniziale di simulazione (strade vuote, TL1 verde)
    // State s0 = (State){ .n1=P.max_r1/2, .n2=P.max_r2/2, .g1=1 };         // stato iniziale di simulazione (metà capacità su entrambe le strade, TL1 verde)
    for(int i=0; i<simulations; i++){
        unsigned int s=seed+i;                                              // copia il seed per non modificare l'originale
        int Rmax = mdp_simulate(&P, s0, Pi, steps, &s, N, R);
        printf("Numero totale di macchine per snapshot: ");                 // simula l'evoluzione della CdM per un certo numero di passi seguendo la policy calcolat
        for(int j=0; j<100; j++){                                           // stampa il numero totale di auto in ciascuno snapshot
            printf("%d, ", N[j]); 
            fprintf(foutN, "%d, ", N[j]);
            fprintf(foutR, "%d, ", R[j]);
        }
        fprintf(foutN, "\n");
        fprintf(foutR, "\n");                      
        printf("\nSimulazione da s0=(%d,%d,%s) per %d step: reward cumulato=%d\n", s0.n1, s0.n2, s0.g1?"TL1G":"TL2G", steps, Rmax);
        fprintf(fout, "\nSimulazione da s0=(%d,%d,%s) per %d step: reward cumulato=%d\n", s0.n1, s0.n2, s0.g1?"TL1G":"TL2G", steps, Rmax);
    }
    free(V); free(Pi); free(N); free(R);                                           // libera la memoria dinamica

	return 0;
}