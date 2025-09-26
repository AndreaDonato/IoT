#include <stdio.h>
#include <stdlib.h>
#include "traffic_lights.h"

int main(int argc, char const *argv[])
{

	// Parametri

	MDPParams P = {
        .max_r1=40, .max_r2=25,
        .add_r1_max=5, .add_r2_max=3,
        .low_th=15, .med_th=30,
        .gamma=0.95
    };
	int steps=50;							// numero di passi di simulazione
    unsigned int seed=12345;				// seed per il generatore di numeri pseudocasuali


	// Struttura dati CDM
	
	// Funzione Aggiornamento (Azione tra stati CdM)

	return 0;
}