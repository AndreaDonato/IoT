#include <stdio.h>
#include <stdlib.h>
#include "traffic_lights.h"

static void usage(const char* prog){
    fprintf(stderr,
      "Uso: %s [max_r1 max_r2 low_th med_th gamma steps seed]\n"
      "Esempio (stato default): %s\n", prog, prog);
}

int main(int argc, char const *argv[])
{

	// Inizializzazione dei parametri
	MDPParams P = {
        .max_r1=40, .max_r2=25,
        .add_r1_max=5, .add_r2_max=3,
        .low_th=15, .med_th=30,
        .gamma=0.95
    };
	int steps=50;							                                // numero di passi di simulazione
    unsigned int seed=12345;				                                // seed per il generatore di numeri pseudocasuali

    // Leggi i parametri da linea di comando
    if(argc>=3){ P.max_r1=atoi(argv[1]); P.max_r2=atoi(argv[2]); }
    if(argc>=5){ P.low_th=atoi(argv[3]); P.med_th=atoi(argv[4]); }
    if(argc>=6){ P.gamma=atof(argv[5]); }
    if(argc>=7){ steps=atoi(argv[6]); }
    if(argc>=8){ seed=(unsigned int)strtoul(argv[7],NULL,10); }
    if(argc==2){ usage(argv[0]); return 1; }

    int S = mdp_num_states(&P);                                             // numero di stati
    double *V = (double*)calloc(S, sizeof(double));                         // valore di ogni stato inizializzato a 0
    unsigned char *Pi = (unsigned char*)malloc(S);                          // policy ottima (0 => TL1 verde; 1 => TL2 verde)

    int it = mdp_value_iteration(&P, 1000, 1e-6, V, Pi);                    // calcola il valore di ciascuno stato e la relativa policy ottima con Value Iteration
    printf("Value Iteration: %d iterazioni, S=%d stati\n", it, S);

    State s0 = (State){ .n1=0, .n2=0, .g1=1 };                              // stato iniziale di simulazione (strade vuote, TL1 verde)
    // State s0 = (State){ .n1=P.max_r1/2, .n2=P.max_r2/2, .g1=1 };          // stato iniziale di simulazione (metà capacità su entrambe le strade, TL1 verde)

    int R = mdp_simulate(&P, s0, Pi, steps, &seed);                         // simula l'evoluzione della CdM per un certo numero di passi seguendo la policy calcolata
    printf("Simulazione da s0=(%d,%d,%s) per %d step: reward cumulato=%d\n", s0.n1, s0.n2, s0.g1?"TL1G":"TL2G", steps, R);

    free(V); free(Pi);                                                      // libera la memoria dinamica

	return 0;
}