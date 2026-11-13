# Openingen van 6×6-dammen

Beginstand, wit aan zet: **D**.

Notatie vanuit wit: D = remise, Wn = wit wint, Ln = wit verliest, met n het aantal ply tot het einde van de partij bij perfect spel. Bij de beginstand telt n vanaf de beginstand (W oneven, L even); bij een zet telt n vanaf de stelling vóór de zet, de zet meegeteld: na een witte zet W oneven en L even, na een zwarte zet W even en L oneven. In de tabel geldt het label voor de laatste zet van de opening. Waar n ontbreekt, is het resultaat exact maar de afstand onbekend.

Methode: de volledige openings-DAG (transposities samengevoegd), uitgebreid zolang een stelling meer dan 10 stukken heeft en hoogstens één kant dammen. Bladeren: databasestellingen (exact), stellingen zonder zetten (verloren) en stellingen met meer stukken en dammen aan beide kanten (onbekend). Waarden zijn als intervallen teruggerekend, vanuit wit gezien.

DAG: 193708 inwendige stellingen, bladeren: database 175882, unknown 5101, unknown, decided in one ply 3848, no moves 56.
Onbekende bladeren die de beginstand nog kunnen beïnvloeden: 0.

## Diagrammen

w/b = witte/zwarte schijf, W/B = dam.

Beginstand, wit aan zet: **D**

```
    +-------------+     +-------------------+
  6 |   b   b   b |     |     1     2     3 |
  5 | b   b   b   |     |  4     5     6    |
  4 |   .   .   . |     |     7     8     9 |
  3 | .   .   .   |     | 10    11    12    |
  2 |   w   w   w |     |    13    14    15 |
  1 | w   w   w   |     | 16    17    18    |
    +-------------+     +-------------------+
      a b c d e f
```

1. 13-10 (zwart aan zet): **D**

```
    +-------------+     +-------------------+
  6 |   b   b   b |     |     1     2     3 |
  5 | b   b   b   |     |  4     5     6    |
  4 |   .   .   . |     |     7     8     9 |
  3 | w   .   .   |     | 10    11    12    |
  2 |   .   w   w |     |    13    14    15 |
  1 | w   w   w   |     | 16    17    18    |
    +-------------+     +-------------------+
      a b c d e f
```

1. 13-11 (zwart aan zet): **D**

```
    +-------------+     +-------------------+
  6 |   b   b   b |     |     1     2     3 |
  5 | b   b   b   |     |  4     5     6    |
  4 |   .   .   . |     |     7     8     9 |
  3 | .   w   .   |     | 10    11    12    |
  2 |   .   w   w |     |    13    14    15 |
  1 | w   w   w   |     | 16    17    18    |
    +-------------+     +-------------------+
      a b c d e f
```

1. 14-11 (zwart aan zet): **D**

```
    +-------------+     +-------------------+
  6 |   b   b   b |     |     1     2     3 |
  5 | b   b   b   |     |  4     5     6    |
  4 |   .   .   . |     |     7     8     9 |
  3 | .   w   .   |     | 10    11    12    |
  2 |   w   .   w |     |    13    14    15 |
  1 | w   w   w   |     | 16    17    18    |
    +-------------+     +-------------------+
      a b c d e f
```

1. 14-12 (zwart aan zet): **D**

```
    +-------------+     +-------------------+
  6 |   b   b   b |     |     1     2     3 |
  5 | b   b   b   |     |  4     5     6    |
  4 |   .   .   . |     |     7     8     9 |
  3 | .   .   w   |     | 10    11    12    |
  2 |   w   .   w |     |    13    14    15 |
  1 | w   w   w   |     | 16    17    18    |
    +-------------+     +-------------------+
      a b c d e f
```

1. 15-12 (zwart aan zet): **D**

```
    +-------------+     +-------------------+
  6 |   b   b   b |     |     1     2     3 |
  5 | b   b   b   |     |  4     5     6    |
  4 |   .   .   . |     |     7     8     9 |
  3 | .   .   w   |     | 10    11    12    |
  2 |   w   w   . |     |    13    14    15 |
  1 | w   w   w   |     | 16    17    18    |
    +-------------+     +-------------------+
      a b c d e f
```

## Boom

```
beginstand  D
    wit 13-10  D
        zwart 4-7  W
            wit 14-11  L44
            wit 14-12  W
            wit 15-12  L
            wit 16-13  D
            wit 17-13  D
        zwart 5-7  D
            wit 10x5  D
        zwart 5-8  D
            wit 10-7  L22
            wit 14-11  L42
            wit 14-12  D
            wit 15-12  L
            wit 16-13  D
            wit 17-13  L
        zwart 6-8  D
            wit 10-7  L22
            wit 14-11  L26
            wit 14-12  L
            wit 15-12  L
            wit 16-13  D
            wit 17-13  D
        zwart 6-9  D
            wit 10-7  L22
            wit 14-11  D
            wit 14-12  L30
            wit 15-12  L
            wit 16-13  D
            wit 17-13  D
    wit 13-11  D
        zwart 4-7  W
            wit 11x4  W
        zwart 5-7  D
            wit 11-8  L
            wit 14-12  L18
            wit 15-12  D
            wit 16-13  D
            wit 17-13  L
        zwart 5-8  D
            wit 11-7  L22
            wit 14-12  D
            wit 15-12  L28
            wit 16-13  D
            wit 17-13  D
        zwart 6-8  D
            wit 11x6  D
        zwart 6-9  W
            wit 11-7  L22
            wit 11-8  W25
            wit 14-12  L
            wit 15-12  D
            wit 16-13  D
            wit 17-13  D
    wit 14-11  D
        zwart 4-7  W
            wit 11x4  W
        zwart 5-7  D
            wit 11-8  D
            wit 13-10  L26
            wit 15-12  L18
            wit 17-14  L
            wit 18-14  L
        zwart 5-8  D
            wit 11-7  L
            wit 13-10  L42
            wit 15-12  L
            wit 17-14  D
            wit 18-14  L
        zwart 6-8  D
            wit 11x6  D
        zwart 6-9  D
            wit 11-7  L
            wit 11-8  D
            wit 13-10  D
            wit 15-12  L18
            wit 17-14  D
            wit 18-14  L
    wit 14-12  D
        zwart 4-7  W
            wit 12-8  W25
            wit 12-9  L
            wit 13-10  W
            wit 13-11  L18
            wit 17-14  L
            wit 18-14  L
        zwart 5-7  D
            wit 12-8  D
            wit 12-9  D
            wit 13-10  D
            wit 13-11  L18
            wit 17-14  L
            wit 18-14  L
        zwart 5-8  D
            wit 12x5  D
        zwart 6-8  D
            wit 12-9  D
            wit 13-10  L
            wit 13-11  D
            wit 17-14  L
            wit 18-14  D
        zwart 6-9  D
            wit 12-8  D
            wit 13-10  L30
            wit 13-11  L
            wit 17-14  L
            wit 18-14  L
    wit 15-12  D
        zwart 4-7  D
            wit 12-8  D
            wit 12-9  D
            wit 13-10  L
            wit 13-11  L
            wit 14-11  L
            wit 18-15  L
        zwart 5-7  W
            wit 12-8  D
            wit 12-9  W
            wit 13-10  L
            wit 13-11  D
            wit 14-11  L18
            wit 18-15  L
        zwart 5-8  W34
            wit 12x5  W33
        zwart 6-8  D
            wit 12-9  D
            wit 13-10  L
            wit 13-11  L
            wit 14-11  L
            wit 18-15  D
        zwart 6-9  D
            wit 12-8  L
            wit 13-10  L
            wit 13-11  D
            wit 14-11  L18
            wit 18-15  L
```

## Alle openingen van 3 ply

106 openingen: W 8, D 40, L 58.

| # | opening | resultaat |
|---|---|---|
| 1 | 13-10 4-7 14-11 | L44 |
| 2 | 13-10 4-7 14-12 | W |
| 3 | 13-10 4-7 15-12 | L |
| 4 | 13-10 4-7 16-13 | D |
| 5 | 13-10 4-7 17-13 | D |
| 6 | 13-10 5-7 10x5 | D |
| 7 | 13-10 5-8 10-7 | L22 |
| 8 | 13-10 5-8 14-11 | L42 |
| 9 | 13-10 5-8 14-12 | D |
| 10 | 13-10 5-8 15-12 | L |
| 11 | 13-10 5-8 16-13 | D |
| 12 | 13-10 5-8 17-13 | L |
| 13 | 13-10 6-8 10-7 | L22 |
| 14 | 13-10 6-8 14-11 | L26 |
| 15 | 13-10 6-8 14-12 | L |
| 16 | 13-10 6-8 15-12 | L |
| 17 | 13-10 6-8 16-13 | D |
| 18 | 13-10 6-8 17-13 | D |
| 19 | 13-10 6-9 10-7 | L22 |
| 20 | 13-10 6-9 14-11 | D |
| 21 | 13-10 6-9 14-12 | L30 |
| 22 | 13-10 6-9 15-12 | L |
| 23 | 13-10 6-9 16-13 | D |
| 24 | 13-10 6-9 17-13 | D |
| 25 | 13-11 4-7 11x4 | W |
| 26 | 13-11 5-7 11-8 | L |
| 27 | 13-11 5-7 14-12 | L18 |
| 28 | 13-11 5-7 15-12 | D |
| 29 | 13-11 5-7 16-13 | D |
| 30 | 13-11 5-7 17-13 | L |
| 31 | 13-11 5-8 11-7 | L22 |
| 32 | 13-11 5-8 14-12 | D |
| 33 | 13-11 5-8 15-12 | L28 |
| 34 | 13-11 5-8 16-13 | D |
| 35 | 13-11 5-8 17-13 | D |
| 36 | 13-11 6-8 11x6 | D |
| 37 | 13-11 6-9 11-7 | L22 |
| 38 | 13-11 6-9 11-8 | W25 |
| 39 | 13-11 6-9 14-12 | L |
| 40 | 13-11 6-9 15-12 | D |
| 41 | 13-11 6-9 16-13 | D |
| 42 | 13-11 6-9 17-13 | D |
| 43 | 14-11 4-7 11x4 | W |
| 44 | 14-11 5-7 11-8 | D |
| 45 | 14-11 5-7 13-10 | L26 |
| 46 | 14-11 5-7 15-12 | L18 |
| 47 | 14-11 5-7 17-14 | L |
| 48 | 14-11 5-7 18-14 | L |
| 49 | 14-11 5-8 11-7 | L |
| 50 | 14-11 5-8 13-10 | L42 |
| 51 | 14-11 5-8 15-12 | L |
| 52 | 14-11 5-8 17-14 | D |
| 53 | 14-11 5-8 18-14 | L |
| 54 | 14-11 6-8 11x6 | D |
| 55 | 14-11 6-9 11-7 | L |
| 56 | 14-11 6-9 11-8 | D |
| 57 | 14-11 6-9 13-10 | D |
| 58 | 14-11 6-9 15-12 | L18 |
| 59 | 14-11 6-9 17-14 | D |
| 60 | 14-11 6-9 18-14 | L |
| 61 | 14-12 4-7 12-8 | W25 |
| 62 | 14-12 4-7 12-9 | L |
| 63 | 14-12 4-7 13-10 | W |
| 64 | 14-12 4-7 13-11 | L18 |
| 65 | 14-12 4-7 17-14 | L |
| 66 | 14-12 4-7 18-14 | L |
| 67 | 14-12 5-7 12-8 | D |
| 68 | 14-12 5-7 12-9 | D |
| 69 | 14-12 5-7 13-10 | D |
| 70 | 14-12 5-7 13-11 | L18 |
| 71 | 14-12 5-7 17-14 | L |
| 72 | 14-12 5-7 18-14 | L |
| 73 | 14-12 5-8 12x5 | D |
| 74 | 14-12 6-8 12-9 | D |
| 75 | 14-12 6-8 13-10 | L |
| 76 | 14-12 6-8 13-11 | D |
| 77 | 14-12 6-8 17-14 | L |
| 78 | 14-12 6-8 18-14 | D |
| 79 | 14-12 6-9 12-8 | D |
| 80 | 14-12 6-9 13-10 | L30 |
| 81 | 14-12 6-9 13-11 | L |
| 82 | 14-12 6-9 17-14 | L |
| 83 | 14-12 6-9 18-14 | L |
| 84 | 15-12 4-7 12-8 | D |
| 85 | 15-12 4-7 12-9 | D |
| 86 | 15-12 4-7 13-10 | L |
| 87 | 15-12 4-7 13-11 | L |
| 88 | 15-12 4-7 14-11 | L |
| 89 | 15-12 4-7 18-15 | L |
| 90 | 15-12 5-7 12-8 | D |
| 91 | 15-12 5-7 12-9 | W |
| 92 | 15-12 5-7 13-10 | L |
| 93 | 15-12 5-7 13-11 | D |
| 94 | 15-12 5-7 14-11 | L18 |
| 95 | 15-12 5-7 18-15 | L |
| 96 | 15-12 5-8 12x5 | W33 |
| 97 | 15-12 6-8 12-9 | D |
| 98 | 15-12 6-8 13-10 | L |
| 99 | 15-12 6-8 13-11 | L |
| 100 | 15-12 6-8 14-11 | L |
| 101 | 15-12 6-8 18-15 | D |
| 102 | 15-12 6-9 12-8 | L |
| 103 | 15-12 6-9 13-10 | L |
| 104 | 15-12 6-9 13-11 | D |
| 105 | 15-12 6-9 14-11 | L18 |
| 106 | 15-12 6-9 18-15 | L |
