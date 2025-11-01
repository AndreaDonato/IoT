#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "traffic_lights.h"




// Mostra l'ordine dei parametri da inserire manualmente. Il codice gira anche con una chiamata senza parametri, con valori standard.
static void usage(const char* prog){
    fprintf(stderr,
      "Uso: %s [hours max_r1 max_r2 add_r1_max add_r2_max out_r1_max out_r2_max low_th med_th gamma steps seed simulations]\n"
      "Esempio (stato default): %s\n", prog, prog);
}





int main(int argc, char const *argv[])
{

    /*************************************
    ***** Inizializzazione parametri *****
    *************************************/
    
    //
    // Inizializzazione di default in caso non vengano specificati valori da linea di comando
    //


    // Struttura dati con parametri di defult
    MDPParams P = {
        .max_r1=40, .max_r2=25,
        .add_r1_max=5, .add_r2_max=3,
        .out_r1_max=3, .out_r2_max=2,
        .low_th=15, .med_th=30,
        .gamma=0.95
    };

    State s_init = (State){ .n1=0, .n2=0, .g1=0 };                                              // Stato iniziale di simulazione (strade vuote, TL1 verde)


    //
    // Parametri delle simulazioni
    //
    int hours = 2;                                                                              // Parametro che decide il numero di fasce orarie             
    unsigned int seed=123456;                                                                   // Seed per il generatore di numeri pseudocasuali
    int simulations=100;                                                                        // Numero di simulazioni ripetute, per ridurre il rumore nei grafici


    //
    // Parametri da linea di comando
    //
    if(argc>=2) { hours = atoi(argv[1]); }                                                      // Parametro che decide il numero di fasce orarie
    if(hours<1 || hours == 5 || hours>6){ printf("\nNumeri di fasce orarie consentiti:\n\n\t1 - Nessuna fascia oraria;\n\t 2,3,4,6 - Fasce orarie da {12, 8, 6, 4} ore).\n\n"); return 1; }
    int steps = 9000/hours;                                                                     // Numero di passi di simulazione (ha sempre 9000 steps, eventualmente divisi in fasce orarie)
    
    // Eventuale override da linea di comando dei parametri di default...
    if(argc>=4) { P.max_r1 = atoi(argv[2]); P.max_r2 = atoi(argv[3]); }         
    if(argc>=6) { P.add_r1_max=atoi(argv[4]); P.add_r2_max=atoi(argv[5]);}
    if(argc>=8) { P.out_r1_max=atoi(argv[6]); P.out_r2_max=atoi(argv[7]);}
    if(argc>=10){ P.low_th=atoi(argv[8]); P.med_th=atoi(argv[9]); }
    if(argc>=11){ P.gamma=atof(argv[10]); }

    // ... e di quelli della simulazione
    if(argc>=12){ seed=(unsigned int)strtoul(argv[11],NULL,10); }
    if(argc>=13){ simulations=atoi(argv[12]); }
    if(argc>=14 && hours==1){ steps=atoi(argv[13]); }                                           // Se c'è una sola fascia oraria posso specificare il numero di step
    if(argc==0 || argc>14){ usage(argv[0]); return 1; }


    //
    // Parametri Q-LEARNING
    //
    double alpha      = 0.1;                                                                    // Learning rate
    double eps_start  = 0.9;                                                                    // Esplorazione iniziale
    double eps_end    = 0.02;                                                                   // Esplorazione minima
    
    // DA TOGLIERE?
    double eps_decay  = 0.995;                                                                  // Decadimento per episodio





    /********************************
    ******** Value Iteration ********
    ********************************/
    
    //
    // Ottimizzazione MDP (caso statico)
    //
    int S = mdp_num_states(&P);                                                                 // Numero di stati della CdM                     
    double *V = (double*)calloc(S, sizeof(double));                                             // Valore di ogni stato inizializzato a 0
    unsigned char *Pi_VI = (unsigned char*)malloc(S);                                           // Policy ottima (0 => TL1 verde; 1 => TL2 verde)
    int it = mdp_value_iteration(&P, 1000, 1e-6, V, Pi_VI);                                     // Calcola il valore di ciascuno stato e la relativa policy ottima con Value Iteration
    
    
    //printf("Value Iteration: %d iterazioni, S=%d stati\n", it, S);
    //fprintf(fout, "Parametri: max_r1=%d, max_r2=%d, add_r1_max=%d, add_r2_max=%d, out_r1_max=%d, out_r2_max=%d, low_th=%d, med_th=%d, gamma=%.2f, steps=%d, seed=%u, simulations=%d, iterations=%d, tot_states=%d\n", P.max_r1, P.max_r2, P.add_r1_max, P.add_r2_max, P.out_r1_max, P.out_r2_max, P.low_th, P.med_th, P.gamma, steps, seed, simulations, it, S);
 

    //
    // Array di supporto per gli snapshot (servono a fare i grafici)
    //
    int *N1=(int*)calloc(hours * 100, sizeof(int));                                             // array per snapshot numero auto r1
    int *N2=(int*)calloc(hours * 100, sizeof(int));                                             // array per snapshot numero auto r2
    int *R =(int*)calloc(hours * 100, sizeof(int));                                             // array per snapshot reward cumulato
    int *T =(int*)calloc(hours * 100, sizeof(int));                                             // array per snapshot tempo

    
    //
    // Simulazione del sistema data la policy ottimale trovata allo step precedente
    //
    State s0 = s_init;
    printf("\nValue Iteration simulation in progress...\n");
    for(int i=0; i<simulations; i++){
        print_progress((double)i / simulations);
        unsigned int s=seed+i;                                                                  // Copia il seed per non modificare l'originale - Ogni simulazione deve avere seed diverso
        int Rmax = mdp_simulate(&P, s0, Pi_VI, steps, &s, N1, N2, R, T);
        //printf("[VI] Reward cumulato=%d\n", Rmax);
    }
    printf("\n");


    //
    // Stampa gli andamenti di n1, n2 ed R in funzione del tempo
    //
    FILE *foutVI  = fopen("outputVI.txt",  "w");
    for(int j=0;j<100;j++){ 
        fprintf(foutVI, "%d, %.1f, %.1f, %.1f \n", T[j], (double)N1[j]/simulations, (double)N2[j]/simulations, (double)R[j]/simulations); 
    }





    /*********************************
    *********** Q-Learning ***********
    *********************************/


    unsigned int *N = (unsigned int*)calloc(S*2, sizeof(unsigned int));                         // Contatore visite (stato, azione) per adattare learning rate
    double *Q = (double*)calloc(S*2, sizeof(double));                                           // Q-table (S stati, 2 azioni)

    //double avg_ret = mdp_q_learning(&P, state, 0, steps_per_ep, 987654u, alpha, eps_start, eps_end, eps_decay, Q, Pi_ql); // allena e produce policy greedy
    //printf("Q-learning: episodes=%d, steps/ep=%d, alpha=%.3f, eps=%.2f->%.2f (x%.3f), avg_return~%.2f\n", episodes, steps_per_ep, alpha, eps_start, eps_end, eps_decay, avg_ret);
    //fprintf(fout, "Q-learning: episodes=%d, steps_per_ep=%d, alpha=%.3f, eps_start=%.2f, eps_end=%.2f, eps_decay=%.3f, avg_return~%.2f\n", episodes, steps_per_ep, alpha, eps_start, eps_end, eps_decay, avg_ret);


    //
    // Re-inizializzazione degli array di supporto per gli snapshot
    //
    memset(N1, 0, hours * 100 * sizeof(int));
    memset(N2, 0, hours * 100 * sizeof(int));
    memset(R,  0, hours * 100 * sizeof(int));
    memset(T,  0, hours * 100 * sizeof(int));

    printf("\nQ-Learning in progress...\n");


    // Nell'implementare le fasce orarie, i parametri di input variano in percentuale. Ci serve un riferimento statico.
    MDPParams Input_Parameters = P;


    //
    // Per ogni simulazione, il Q-Learning 
    //
    for(int i=0; i<simulations; i++){
        print_progress(i / simulations);

        //
        // Se non ci sono fasce orarie, re-inizializza lo stato iniziale e la Q-Table ad ogni simulazione.
        // Se invece ci sono, l'idea è simulare diversi giorni di fila, non lo stesso giorno ripetuto da zero.
        //
        if(hours==1){
            s0 = s_init;
            memset(Q, 0, S * 2 * sizeof(double));
        }
        print_progress((double) i / simulations);

        unsigned int s=seed+1000+i;
        int cumR = 0;                                                                           // Cumulative reward across hours for a single simulation
        for(int j=1; j<=hours; j++){
            adjust_params_for_hour(Input_Parameters, &P, hours, j);                                               // Aggiorna i parametri per la fascia oraria (se necessario)
            double avg = mdp_q_learning(&P, &s0, 1, steps, s, alpha, eps_start, eps_end, eps_decay, Q, N, N1, N2, R, T, &cumR, j-1);
            //printf("[QL] Reward cumulato=%d (avg=%.2f)\n", cumR, avg);
        }
    }
    printf("\n\n");


    //
    // Stampa gli andamenti di n1, n2 ed R in funzione del tempo
    //
    FILE *foutQL  = fopen("outputQL.txt",  "w");
    for(int j=0;j<hours*100;j++){ 
        fprintf(foutQL, "%d, %.1f, %.1f, %.1f \n", T[j], (double)N1[j]/simulations, (double)N2[j]/simulations, (double)R[j]/simulations);
    }




    /*********************************
    ************** Free **************
    *********************************/

    free(V); free(Pi_VI);
    free(Q);
    free(N1); free(N2);
    free(R ); free(T);
    free(N );
    fclose(foutVI); fclose(foutQL);
    
    return 0;
}