set title "Risultati QL"
set xlabel "step"
set ylabel "n"
plot   "outputQL.txt" using 1:($2+$3) with lines title "QL N", "outputQL.txt" using 1:2 with lines title "QL n1", "outputQL.txt" using 1:3 with lines title "QL n2"
#replot "outputVI.txt" using 1:($2+$3) with lines title "VI N", "outputVI.txt" using 1:2 with lines title "VI n1", "outputVI.txt" using 1:3 with lines title "VI n2"

  
pause -1 "Premi invio per uscire"


#"outputQL.txt" using 1:2 with lines title "QL n1", 
  #"outputQL.txt" using 1:3 with lines title "QL n2", \
  "outputQL.txt" using 1:($2+$3) with lines title "QL N", 
  #"outputVI.txt" using 1:2 with lines title "VI n1", \
  #"outputVI.txt" using 1:3 with lines title "VI n2", \
  "outputVI.txt" using 1:($2+$3) with lines title "VI N"