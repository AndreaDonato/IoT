
GRAPH_NAME = "Compensation"


set terminal postscript eps enhanced color font "Helvetica,12" 

# Output: file .eps
set output GRAPH_NAME."_ConfrontoN_DiversiMetodi.eps"

set title "Confronto N Diversi Metodi"
set xlabel "step"
set ylabel "n"
set key box
set key bottom right

plot  "outputStatic.txt" using 1:($2+$3) with lines linewidth 2 title "Static N", \
      "outputVI.txt" using 1:($2+$3) with lines linewidth 2 title "VI N", \
      "outputQL.txt" using 1:($2+$3) with lines linewidth 2 title "QL N", \
      "outputGeniusQL.txt" using 1:($2+$3) with lines linewidth 2 title "GQL N"

unset output




## DettaglioVI
set output GRAPH_NAME."_DettaglioVI.eps"
set title "DettaglioVI"
set xlabel "step"
set ylabel "n"

plot  "outputVI.txt" using 1:($2+$3) with lines title "VI N", "outputVI.txt" using 1:2 with lines title "VI n1", "outputVI.txt" using 1:3 with lines title "VI n2"

unset output




## Confronto Static QL
set output GRAPH_NAME."_Confronto_Static_QL.eps"
set title "Confronto Static QL"
set xlabel "step"
set ylabel "n"

plot  "outputStatic.txt" using 1:($2+$3) with lines title "Static N", "outputStatic.txt" using 1:2 with lines title "Static n1", "outputStatic.txt" using 1:3 with lines title "Static n2",\
      "outputQL.txt" using 1:($2+$3) with lines title "QL N", "outputQL.txt" using 1:2 with lines title "QL n1", "outputQL.txt" using 1:3 with lines title "QL n2"

unset output




## Confronto QL GQL
set output GRAPH_NAME."_ConfrontoN_QL_GQL.eps"
set title "Confronto QL GQL"
set xlabel "step"
set ylabel "n"

plot  "outputQL.txt" using 1:($2+$3) with lines title "QL N", "outputQL.txt" using 1:2 with lines title "QL n1", "outputQL.txt" using 1:3 with lines title "QL n2",\
      "outputGeniusQL.txt" using 1:($2+$3) with lines title "GQL N", "outputGeniusQL.txt" using 1:2 with lines title "GQL n1", "outputGeniusQL.txt" using 1:3 with lines title "GQL n2"

unset output