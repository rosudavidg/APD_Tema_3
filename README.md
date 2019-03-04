```
=========================================================================================
Nume:    Rosu
Prenume: Gabriel - David
Grupa:   331CC
Data:    19.12.2018
Materie: APD, anul 3, semestrul 1
=========================================================================================
                                         Tema 3
                                 Image processing with MPI
=========================================================================================
    CUPRINS
-----------------------------------------------------------------------------------------
    [1] INTRODUCERE
    [2] CUM? DE CE?
    [3] FEEDBACK
=========================================================================================
[1] INTRODUCERE
-----------------------------------------------------------------------------------------
    Tema consta in aplicarea mai multor filtre unor imagini in format PGM sau PNM. Este
realizata in limbajul C, intr-o maniera distribuita, folosind MPI.
=========================================================================================
[2] CUM? DE CE?
=========================================================================================
-----------------------------------------------------------------------------------------
    Am utilizat structurile si mai multe functii din prima tema, cum ar fi:
read_next_int, readInput, write_int etc.
    Aplicarea unui filtru este aceeasi, indiferent de filtru. Am folosit MACRO-uri pentru
salvarea matricelor asociate filtrelor.
    Procesul ROOT (definit ca 0) se ocupa de citirea si scrierea pozelor, insa si de
prelucrarea unei bucati din aceasta (pentru eficientizare si pentru cazul cand exista
un singur proces). Celelalte procese se ocupa doar de prelucrare.
    
    1. Procesul ROOT citest imaginea initiala.
    2. Se trimit si primesc prin broadcast datele legate de poza (dimensiuni, numar de
culori etc).
    3. Fiecare proces isi aloca memorie pentru poza finala.
    4. Pentru fiecare filtru:
    4.1 ROOT trimite ('in') poza broadcast, iar toate procesle o primesc in imaginea 'in'.
    4.2 Fiecare proces aplica filtrul curent pe liniile indicate de [start, stop).
    4.3 Fiecare proces trimite portiunea de poza prelucrata (inclusiv ROOT) catre ROOT.
    4.4 ROOT formeaza in 'in' poza cu filtrul aplicat (formata din mesajele primite).
    4.5 Se continua cu pasul 4.1, pana cand se termina filtrele.
    5. ROOT scrie imaginea din 'in' in fisierul de output.

    Algoritmul este suficient de eficient, scaleaza, iar rezultatele acestuia sunt corecte.

    Exemplu pentru 3 procese:

    FISIER INTRARE:
    1234
    5678
    9012

    CITIRE:
    ROOT     P1       P2
    in   out in   out in   out
    1234  
    5678
    9012

    TRIMITERE DATE CATRE PROCESE:
    ROOT           P1        P2
    in   out       in   out  in   out 
    1234           5678      9012
    5678
    9012

    PRELUCRARE DATE:
    ROOT           P1        P2
    in   out       in   out  in   out 
    1234 ABCD      5678 EFGH 9012 IJKL
    5678
    9012

    PRIMIRE DATE CATRE PROCESE:
    ROOT           P1        P2
    in   out       in   out  in   out 
    ABCD ABCD      5678 EFGH 9012 IJKL
    EFGH
    IJKL

    FISIER IESIRE:
    ABCD
    EFGH
    IJKL

=========================================================================================
[3] FEEDBACK
-----------------------------------------------------------------------------------------
[+]     Tema isi indeplineste cu succes scopul, acela de a pune in practica cunostintele
acumulate de-alungul cursurilor, laboratoarelor, dar si studiului individual.
=========================================================================================
                                        SFARSIT
=========================================================================================
```