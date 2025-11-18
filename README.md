
COMPILAZIONE:   gcc main.c traffic_lights.c -o smart_traffic.exe    (Windows)
                gcc main.c traffic_lights.c -o smart_traffic.x      (Linux)


RUN         :   Da lina di comando possono essere passati i seguenti parametri, nella forma NOME(VALORI AMMESSI), in quest'ordine:

                    - hours (1, 2, 3, 4, 6) -   Definisce in quante fasce orarie dividere un giorno (24 ore) di simulazione.
                                                Diverse fasce orarie determinano diverse fluttuazioni di traffico.
                    
                    - max_r1, max_r2 (interi positivi) - Capienza massima delle code 1 e 2.
                    
                    - add_r1_max, add_r2_max (interi positivi) - Massimo numero di auto che si aggiungono alle code 1 e 2.
                    
                    - out_r1_max, out_r2_max (interi positivi) - Massimo numero di auto che possono superare l'incrocio in un singolo step provenendo dalle code 1 e 2.
                    
                    - low_th, med_th (interi positivi, il secondo maggiore stretto del primo) - Le due threshold che definiscono le zone di basso, medio e alto traffico.
                    
                    - seed (intero) - Riproducibilità della simulazione. Tutte le simulazioni che compaiono nel report usano seed=12312.
                    
                    - simulations (intero positivo) - Numero di simulazioni ripetute per ridurre il rumore nei grafici. Valore tipicamente usato: 10000.
                    
                    - steps (intero positivo) - Numero di step della simulazione.


OUTPUT      :   Un'esecuzione del programma produce file outputXX.txt della forma [[ TIMESTAMP n1 n2 R ]], dove XX=

                    - Static per il caso statico
                    - VI per la Value Iteration (MDP)
                    - QL per il Q-Learning
                    - GeniusQL per il Q-Learning nel caso di conducenti indisciplinati.

                A partire da tali file si possono ricavare i grafici.


GRAFICI     :   I grafici possono essere prodotti semplicemente eseguendo genera_grafici.sh.
                L'unico parametro modificabile (GRAPH_NAME) si trova in testa al file grafici.gnuplot, ed è il prefisso usato per distinguere lo scenario.
                Ad esempio, i grafici relativi ai parametri proposti nel pdf originale hanno come prefisso "Default".