#include <stdio.h>
#include <stdlib.h>
//#include "traffic_lights.h"

int main(int argc, char const *argv[])
{
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