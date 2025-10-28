Cose da implementare:

- Per il RL: lei propone aggiornamento Q(s, a) ← Q(s, a) + α[r + γ max Q(s′, a) − Q(s, a)] (Q-Learning) e policy epsilon-greedy (quindi azione scelta con prob 1-epsilon come max di Q value su quello stato e casuale con probabilità epsilon).

    - Exploration vs Exploitation che decade come epsilon <- c/(c+k) con c=10 (o comunque una funzione L1)
    - anche learning rate che decade, suggerisce di farlo con un contatore N(s,a) di quante volte il sistema sceglie quell'azione e fare 1/N(s,a)
    - Suggerisce di simulare una situazione reale con fluttuazioni di traffico tra giorno e notte 

- Un programma che esegue solo il task richiesto e tirare fuori N in funzione di t (credo basti questo come metrica), magari gli si sovrappone il reward cumulato R;
- Un programma per giocare con i parametri (e.g. capienza massima delle code) e tirare fuori qualche grafico (tipo, fissato t=500 plottiamo N in funzione della capienza massima complessiva N_max);
- Un programmino per il caso non smart (switch di semafori fisso ogni TOT azioni), per avere un riferimento di quanto migliora la situazione

COMPILAZIONE: gcc main.c traffic_lights.c -o smart_traffic.exe

MODIFIICARE TIE-BREAK DETERMINISTICO (argmax2) e capiire perchè con 0 : 1 una coda non si svuota mai

Also: questa roba viene applicata solo al caso Q-Learning. Come risolve questi conflitti il caso statico??