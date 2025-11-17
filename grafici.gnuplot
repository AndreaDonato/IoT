set title "Risultati QL"
set xlabel "step"
set ylabel "n"


plot   "outputStatic.txt" using 1:($2+$3) with lines title "Static N"#, "outputStatic.txt" using 1:2 with lines title "Static n1", "outputStatic.txt" using 1:3 with lines title "Static n2"

replot "outputVI.txt" using 1:($2+$3) with lines title "VI N"#, "outputVI.txt" using 1:2 with lines title "VI n1", "outputVI.txt" using 1:3 with lines title "VI n2"

replot "outputQL.txt" using 1:($2+$3) with lines title "QL N"#, "outputQL.txt" using 1:2 with lines title "QL n1", "outputQL.txt" using 1:3 with lines title "QL n2"

replot "outputGeniusQL.txt" using 1:($2+$3) with lines title "GQL N"#, "outputGeniusQL.txt" using 1:2 with lines title "GQL n1", "outputGeniusQL.txt" using 1:3 with lines title "GQL n2"

#replot "test_outputGeniusQL.txt" using 1:($2+$3) with lines title "test GQL N"#, "test_outputGeniusQL.txt" using 1:2 with lines title "GQL n1", "test_outputGeniusQL.txt" using 1:3 with lines title "GQL n2"

pause -1 "Premi invio per uscire"


#"outputQL.txt" using 1:2 with lines title "QL n1", 
  #"outputQL.txt" using 1:3 with lines title "QL n2", \
  "outputQL.txt" using 1:($2+$3) with lines title "QL N", 
  #"outputVI.txt" using 1:2 with lines title "VI n1", \
  #"outputVI.txt" using 1:3 with lines title "VI n2", \
  "outputVI.txt" using 1:($2+$3) with lines title "VI N"