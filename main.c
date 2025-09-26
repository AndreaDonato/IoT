#include <stdio.h>
#include <stdlib.h>
#include "traffic_lights.h"

int main(int argc, char const *argv[])
{
	MDPParams P = {
        .max_r1=40, .max_r2=25,
        .add_r1_max=5, .add_r2_max=3,
        .low_th=15, .med_th=30,
        .gamma=0.95
    };
	int steps=50;	// numero di passi di simulazione
    unsigned int seed=12345;	// seed per il generatore di numeri pseudocasuali

	// Parametri
	// Gioca con capienza massima della coda, switch di semafori fisso ogni TOT azioni (caso non smart)
	
	int n1 = 0;
	n1 = &(*argv[1]);
	
	int n2 = 0;
	n2 = *argv[2];
	
	printf("%d\n%d\n", n1, n2);
	


	// Struttura dati CDM

	int markov_chain[41][26][2];

	
	// Funzione Aggiornamento (Azione tra stati CdM)

	return 0;
}