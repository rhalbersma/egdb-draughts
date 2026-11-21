# Openingen van 6×6-dammen

Beginstand, wit aan zet: **D**.

Notatie: D = remise, Wn = winst, Ln = verlies, met n het aantal ply tot het einde van de partij bij perfect spel. Bij de beginstand geldt de waarde voor wit (aan zet). Bij een zet geldt de waarde voor wie de zet doet, en telt n vanaf de stelling vóór de zet, de zet meegeteld. Een winst heeft dus altijd een oneven n, een verlies een even n. In de tabel geldt het label voor de laatste (witte) zet van de opening.

Methode: elke stelling met hoogstens 12 stukken (hooguit 6 per kant) staat in de database, dus ook de beginstand: het spel is opgelost en alle waarden zijn exact, met afstand. De boom is uitgebreid tot 4 ply (transposities samengevoegd); elk blad heeft zijn waarde uit de database, de waarden zijn met minimax teruggerekend, en elk van de 112 inwendige stellingen is gecontroleerd tegen zijn eigen waarde in de database.

## Diagrammen

w/b = witte/zwarte schijf, W/B = dam.

Beginstand, wit aan zet: **D**

```text
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

```text
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

```text
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

```text
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

```text
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

```text
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

Per stelling staan de zetten op volgorde van waarde voor wie ze doet: winst (snelste eerst), remise, verlies (taaiste eerst); daarbinnen numeriek.

```text
beginstand  D
    wit 13-10  D
        zwart 5-7  D
            wit 10x5  D
                zwart 1x8  D
                zwart 2x7  D
        zwart 5-8  D
            wit 14-12  D
                zwart 6-9  D
                zwart 1-5  L42
                zwart 8-11  L24
                zwart 2-5  L16
                zwart 4-7  L14
            wit 16-13  D
                zwart 1-5  D
                zwart 2-5  D
                zwart 6-9  D
                zwart 8-11  D
                zwart 8-12  D
                zwart 4-7  L16
            wit 14-11  L42
                zwart 8x13  W41
            wit 15-12  L28
                zwart 8x15  W27
            wit 17-13  L26
                zwart 8-11  W25
                zwart 1-5  D
                zwart 2-5  D
                zwart 6-9  L28
                zwart 8-12  L20
                zwart 4-7  L18
            wit 10-7  L22
                zwart 4x11  W21
        zwart 6-8  D
            wit 16-13  D
                zwart 2-6  D
                zwart 3-6  D
                zwart 4-7  D
                zwart 8-12  D
                zwart 5-7  L18
                zwart 8-11  L18
            wit 17-13  D
                zwart 2-6  D
                zwart 3-6  D
                zwart 8-11  L28
                zwart 8-12  L24
                zwart 4-7  L22
                zwart 5-7  L20
            wit 14-12  L32
                zwart 3-6  W31
                zwart 8-11  D
                zwart 4-7  L58
                zwart 2-6  L16
                zwart 5-7  L16
            wit 14-11  L26
                zwart 8x13  W25
            wit 10-7  L22
                zwart 4x11  W21
                zwart 5x10  W23
            wit 15-12  L22
                zwart 8x15  W21
        zwart 6-9  D
            wit 14-11  D
                zwart 2-6  D
                zwart 3-6  D
                zwart 5-7  D
                zwart 5-8  D
                zwart 9-12  D
                zwart 4-7  L28
            wit 16-13  D
                zwart 2-6  D
                zwart 3-6  D
                zwart 5-8  D
                zwart 9-12  D
                zwart 5-7  L24
                zwart 4-7  L22
            wit 17-13  D
                zwart 2-6  D
                zwart 3-6  D
                zwart 4-7  D
                zwart 5-7  L44
                zwart 5-8  L28
                zwart 9-12  L24
            wit 14-12  L30
                zwart 9x14  W29
            wit 15-12  L26
                zwart 5-7  W25
                zwart 3-6  W27
                zwart 2-6  D
                zwart 4-7  D
                zwart 5-8  L28
            wit 10-7  L22
                zwart 4x11  W21
                zwart 5x10  W25
        zwart 4-7  L60
            wit 14-12  W59
                zwart 1-4  L58
                zwart 6-8  L58
                zwart 7-11  L28
                zwart 6-9  L22
                zwart 5-8  L14
            wit 16-13  D
                zwart 1-4  D
                zwart 6-8  D
                zwart 7-11  L24
                zwart 6-9  L22
                zwart 5-8  L16
            wit 17-13  D
                zwart 6-9  D
                zwart 1-4  L58
                zwart 7-11  L26
                zwart 6-8  L22
                zwart 5-8  L18
            wit 14-11  L44
                zwart 7x14  W43
            wit 15-12  L32
                zwart 6-8  W31
                zwart 6-9  D
                zwart 7-11  L24
                zwart 1-4  L20
                zwart 5-8  L14
    wit 13-11  D
        zwart 5-7  D
            wit 15-12  D
                zwart 6-9  D
                zwart 7-10  L28
                zwart 1-5  L24
                zwart 6-8  L18
                zwart 2-5  L16
            wit 16-13  D
                zwart 1-5  D
                zwart 2-5  D
                zwart 6-9  D
                zwart 7-10  D
                zwart 6-8  L24
            wit 17-13  L24
                zwart 7-10  W23
                zwart 1-5  D
                zwart 2-5  D
                zwart 6-8  D
                zwart 6-9  D
            wit 11-8  L22
                zwart 6x11  W21
            wit 14-12  L18
                zwart 7x9  W17
        zwart 5-8  D
            wit 14-12  D
                zwart 8x13  D
            wit 16-13  D
                zwart 1-5  D
                zwart 2-5  D
                zwart 4-7  L22
                zwart 8-12  L22
                zwart 6-9  L20
            wit 17-13  D
                zwart 1-5  D
                zwart 2-5  D
                zwart 6-9  D
                zwart 4-7  L26
                zwart 8-12  L22
            wit 15-12  L28
                zwart 8x15  W27
                zwart 8x13  D
            wit 11-7  L22
                zwart 4x11  W21
        zwart 6-8  D
            wit 11x6  D
                zwart 3x8  D
                zwart 2x9  L26
        zwart 6-9  L26
            wit 11-8  W25
                zwart 5x12  L24
            wit 15-12  D
                zwart 3-6  D
                zwart 5-7  D
                zwart 5-8  L24
                zwart 4-7  L20
                zwart 2-6  L16
            wit 16-13  D
                zwart 2-6  D
                zwart 3-6  D
                zwart 5-7  D
                zwart 4-7  L20
                zwart 5-8  L20
                zwart 9-12  L20
            wit 17-13  D
                zwart 2-6  D
                zwart 3-6  D
                zwart 5-7  D
                zwart 5-8  D
                zwart 9-12  L24
                zwart 4-7  L22
            wit 11-7  L22
                zwart 4x11  W21
                zwart 5x10  W25
            wit 14-12  L18
                zwart 9x7  W17
        zwart 4-7  L24
            wit 11x4  W23
                zwart 5-8  L22
                zwart 6-8  L22
                zwart 6-9  L20
                zwart 5-7  L18
    wit 14-11  D
        zwart 5-7  D
            wit 11-8  D
                zwart 6x11  D
            wit 13-10  L26
                zwart 7x14  W25
            wit 17-14  L24
                zwart 7-10  W23
                zwart 1-5  D
                zwart 2-5  D
                zwart 6-8  D
                zwart 6-9  D
            wit 18-14  L24
                zwart 7-10  W23
                zwart 1-5  D
                zwart 2-5  D
                zwart 6-8  D
                zwart 6-9  D
            wit 15-12  L18
                zwart 7x9  W17
        zwart 5-8  D
            wit 17-14  D
                zwart 1-5  D
                zwart 2-5  D
                zwart 6-9  D
                zwart 4-7  L26
                zwart 8-12  L22
            wit 13-10  L42
                zwart 8x13  W41
            wit 18-14  L26
                zwart 1-5  W25
                zwart 2-5  D
                zwart 6-9  D
                zwart 4-7  L22
                zwart 8-12  L20
            wit 11-7  L22
                zwart 4x11  W21
            wit 15-12  L22
                zwart 8x15  W21
        zwart 6-8  D
            wit 11x6  D
                zwart 2x9  D
                zwart 3x8  D
        zwart 6-9  D
            wit 11-8  D
                zwart 5x12  D
            wit 13-10  D
                zwart 2-6  D
                zwart 3-6  D
                zwart 5-7  D
                zwart 5-8  D
                zwart 9-12  D
                zwart 4-7  L28
            wit 17-14  D
                zwart 2-6  D
                zwart 3-6  D
                zwart 5-7  D
                zwart 5-8  D
                zwart 9-12  L24
                zwart 4-7  L22
            wit 11-7  L26
                zwart 5x10  W25
                zwart 4x11  L24
            wit 18-14  L24
                zwart 2-6  W23
                zwart 3-6  W33
                zwart 5-7  D
                zwart 5-8  D
                zwart 4-7  L26
                zwart 9-12  L22
            wit 15-12  L18
                zwart 9x7  W17
        zwart 4-7  L24
            wit 11x4  W23
                zwart 5-8  L22
                zwart 6-8  L22
                zwart 6-9  L22
                zwart 5-7  L18
    wit 14-12  D
        zwart 5-7  D
            wit 12-8  D
                zwart 6x11  D
            wit 12-9  D
                zwart 6-8  D
                zwart 7-10  D
                zwart 1-5  L58
                zwart 7-11  L24
                zwart 2-5  L22
            wit 13-10  D
                zwart 6-9  D
                zwart 1-5  L58
                zwart 7-11  L24
                zwart 2-5  L22
                zwart 6-8  L16
            wit 18-14  L28
                zwart 7-11  W27
                zwart 6-9  W31
                zwart 1-5  D
                zwart 7-10  L28
                zwart 2-5  L22
                zwart 6-8  L18
            wit 17-14  L22
                zwart 7-11  W21
                zwart 2-5  W25
                zwart 6-9  W25
                zwart 7-10  W25
                zwart 1-5  D
                zwart 6-8  L16
            wit 13-11  L18
                zwart 7x9  W17
        zwart 5-8  D
            wit 12x5  D
                zwart 1x8  D
                zwart 2x7  L24
        zwart 6-8  D
            wit 12-9  D
                zwart 3-6  D
                zwart 5-7  D
                zwart 4-7  L58
                zwart 8-11  L22
                zwart 8-12  L22
                zwart 2-6  L14
            wit 13-11  D
                zwart 8x13  D
            wit 18-14  D
                zwart 2-6  D
                zwart 3-6  D
                zwart 8-11  L34
                zwart 4-7  L22
                zwart 5-7  L18
            wit 13-10  L32
                zwart 3-6  W31
                zwart 8-11  D
                zwart 4-7  L58
                zwart 2-6  L16
                zwart 5-7  L16
            wit 17-14  L30
                zwart 8-11  W29
                zwart 2-6  D
                zwart 3-6  D
                zwart 4-7  L26
                zwart 5-7  L16
        zwart 6-9  D
            wit 12-8  D
                zwart 5x12  D
            wit 18-14  L32
                zwart 5-7  W31
                zwart 2-6  W33
                zwart 4-7  W37
                zwart 3-6  D
                zwart 5-8  D
            wit 13-10  L30
                zwart 9x14  W29
            wit 17-14  L26
                zwart 5-7  W25
                zwart 2-6  D
                zwart 3-6  D
                zwart 4-7  D
                zwart 5-8  L44
            wit 13-11  L18
                zwart 9x7  W17
        zwart 4-7  L26
            wit 12-8  W25
                zwart 5x12  L24
                zwart 6x11  L24
            wit 13-10  W59
                zwart 1-4  L58
                zwart 6-8  L58
                zwart 7-11  L28
                zwart 6-9  L22
                zwart 5-8  L14
            wit 12-9  L26
                zwart 7-10  W25
                zwart 1-4  L58
                zwart 6-8  L58
                zwart 7-11  L24
                zwart 5-8  L22
            wit 18-14  L26
                zwart 7-10  W25
                zwart 7-11  W27
                zwart 6-9  W37
                zwart 1-4  D
                zwart 6-8  L22
                zwart 5-8  L18
            wit 17-14  L22
                zwart 7-10  W21
                zwart 1-4  D
                zwart 6-9  D
                zwart 7-11  D
                zwart 6-8  L26
                zwart 5-8  L18
            wit 13-11  L18
                zwart 7x9  W17
    wit 15-12  D
        zwart 4-7  D
            wit 12-8  D
                zwart 5x12  D
                zwart 6x11  D
            wit 12-9  D
                zwart 7-10  D
                zwart 7-11  D
                zwart 1-4  L24
                zwart 5-8  L20
                zwart 6-8  L20
            wit 13-10  L32
                zwart 6-8  W31
                zwart 6-9  D
                zwart 7-11  L24
                zwart 1-4  L20
                zwart 5-8  L14
            wit 13-11  L32
                zwart 6-8  W31
                zwart 7-10  D
                zwart 1-4  L24
                zwart 6-9  L20
                zwart 5-8  L16
            wit 18-15  L26
                zwart 7-10  W25
                zwart 7-11  W27
                zwart 6-9  W37
                zwart 1-4  D
                zwart 6-8  L22
                zwart 5-8  L18
            wit 14-11  L18
                zwart 7x9  W17
        zwart 6-8  D
            wit 12-9  D
                zwart 8-12  D
                zwart 3-6  L28
                zwart 8-11  L28
                zwart 5-7  L24
                zwart 4-7  L20
                zwart 2-6  L16
            wit 18-15  D
                zwart 2-6  D
                zwart 3-6  D
                zwart 8-11  L34
                zwart 4-7  L22
                zwart 5-7  L18
            wit 13-10  L22
                zwart 8x15  W21
            wit 13-11  L22
                zwart 8x15  W21
                zwart 8x13  D
            wit 14-11  L22
                zwart 8x15  W21
        zwart 6-9  D
            wit 13-11  D
                zwart 3-6  D
                zwart 5-7  D
                zwart 5-8  L24
                zwart 4-7  L20
                zwart 2-6  L16
            wit 12-8  L32
                zwart 5x12  W31
            wit 18-15  L32
                zwart 5-7  W31
                zwart 2-6  W33
                zwart 4-7  W37
                zwart 3-6  D
                zwart 5-8  D
            wit 13-10  L26
                zwart 5-7  W25
                zwart 3-6  W27
                zwart 2-6  D
                zwart 4-7  D
                zwart 5-8  L28
            wit 14-11  L18
                zwart 9x7  W17
        zwart 5-8  L34
            wit 12x5  W33
                zwart 1x8  L32
                zwart 2x7  L24
        zwart 5-7  L30
            wit 12-9  W29
                zwart 7-10  L28
                zwart 1-5  L24
                zwart 6-8  L24
                zwart 7-11  L24
                zwart 2-5  L20
            wit 12-8  D
                zwart 6x11  D
            wit 13-11  D
                zwart 6-9  D
                zwart 7-10  L28
                zwart 1-5  L24
                zwart 6-8  L18
                zwart 2-5  L16
            wit 18-15  L28
                zwart 7-11  W27
                zwart 6-9  W31
                zwart 1-5  D
                zwart 7-10  L28
                zwart 2-5  L22
                zwart 6-8  L18
            wit 13-10  L26
                zwart 6-9  W25
                zwart 7-11  D
                zwart 1-5  L20
                zwart 2-5  L20
                zwart 6-8  L16
            wit 14-11  L18
                zwart 7x9  W17
```

## Alle openingen van 3 ply

Numeriek gesorteerd.

106 openingen: W 8, D 40, L 58.

| # | opening | resultaat |
|---|---|---|
| 1 | 13-10 4-7 14-11 | L44 |
| 2 | 13-10 4-7 14-12 | W59 |
| 3 | 13-10 4-7 15-12 | L32 |
| 4 | 13-10 4-7 16-13 | D |
| 5 | 13-10 4-7 17-13 | D |
| 6 | 13-10 5-7 10x5 | D |
| 7 | 13-10 5-8 10-7 | L22 |
| 8 | 13-10 5-8 14-11 | L42 |
| 9 | 13-10 5-8 14-12 | D |
| 10 | 13-10 5-8 15-12 | L28 |
| 11 | 13-10 5-8 16-13 | D |
| 12 | 13-10 5-8 17-13 | L26 |
| 13 | 13-10 6-8 10-7 | L22 |
| 14 | 13-10 6-8 14-11 | L26 |
| 15 | 13-10 6-8 14-12 | L32 |
| 16 | 13-10 6-8 15-12 | L22 |
| 17 | 13-10 6-8 16-13 | D |
| 18 | 13-10 6-8 17-13 | D |
| 19 | 13-10 6-9 10-7 | L22 |
| 20 | 13-10 6-9 14-11 | D |
| 21 | 13-10 6-9 14-12 | L30 |
| 22 | 13-10 6-9 15-12 | L26 |
| 23 | 13-10 6-9 16-13 | D |
| 24 | 13-10 6-9 17-13 | D |
| 25 | 13-11 4-7 11x4 | W23 |
| 26 | 13-11 5-7 11-8 | L22 |
| 27 | 13-11 5-7 14-12 | L18 |
| 28 | 13-11 5-7 15-12 | D |
| 29 | 13-11 5-7 16-13 | D |
| 30 | 13-11 5-7 17-13 | L24 |
| 31 | 13-11 5-8 11-7 | L22 |
| 32 | 13-11 5-8 14-12 | D |
| 33 | 13-11 5-8 15-12 | L28 |
| 34 | 13-11 5-8 16-13 | D |
| 35 | 13-11 5-8 17-13 | D |
| 36 | 13-11 6-8 11x6 | D |
| 37 | 13-11 6-9 11-7 | L22 |
| 38 | 13-11 6-9 11-8 | W25 |
| 39 | 13-11 6-9 14-12 | L18 |
| 40 | 13-11 6-9 15-12 | D |
| 41 | 13-11 6-9 16-13 | D |
| 42 | 13-11 6-9 17-13 | D |
| 43 | 14-11 4-7 11x4 | W23 |
| 44 | 14-11 5-7 11-8 | D |
| 45 | 14-11 5-7 13-10 | L26 |
| 46 | 14-11 5-7 15-12 | L18 |
| 47 | 14-11 5-7 17-14 | L24 |
| 48 | 14-11 5-7 18-14 | L24 |
| 49 | 14-11 5-8 11-7 | L22 |
| 50 | 14-11 5-8 13-10 | L42 |
| 51 | 14-11 5-8 15-12 | L22 |
| 52 | 14-11 5-8 17-14 | D |
| 53 | 14-11 5-8 18-14 | L26 |
| 54 | 14-11 6-8 11x6 | D |
| 55 | 14-11 6-9 11-7 | L26 |
| 56 | 14-11 6-9 11-8 | D |
| 57 | 14-11 6-9 13-10 | D |
| 58 | 14-11 6-9 15-12 | L18 |
| 59 | 14-11 6-9 17-14 | D |
| 60 | 14-11 6-9 18-14 | L24 |
| 61 | 14-12 4-7 12-8 | W25 |
| 62 | 14-12 4-7 12-9 | L26 |
| 63 | 14-12 4-7 13-10 | W59 |
| 64 | 14-12 4-7 13-11 | L18 |
| 65 | 14-12 4-7 17-14 | L22 |
| 66 | 14-12 4-7 18-14 | L26 |
| 67 | 14-12 5-7 12-8 | D |
| 68 | 14-12 5-7 12-9 | D |
| 69 | 14-12 5-7 13-10 | D |
| 70 | 14-12 5-7 13-11 | L18 |
| 71 | 14-12 5-7 17-14 | L22 |
| 72 | 14-12 5-7 18-14 | L28 |
| 73 | 14-12 5-8 12x5 | D |
| 74 | 14-12 6-8 12-9 | D |
| 75 | 14-12 6-8 13-10 | L32 |
| 76 | 14-12 6-8 13-11 | D |
| 77 | 14-12 6-8 17-14 | L30 |
| 78 | 14-12 6-8 18-14 | D |
| 79 | 14-12 6-9 12-8 | D |
| 80 | 14-12 6-9 13-10 | L30 |
| 81 | 14-12 6-9 13-11 | L18 |
| 82 | 14-12 6-9 17-14 | L26 |
| 83 | 14-12 6-9 18-14 | L32 |
| 84 | 15-12 4-7 12-8 | D |
| 85 | 15-12 4-7 12-9 | D |
| 86 | 15-12 4-7 13-10 | L32 |
| 87 | 15-12 4-7 13-11 | L32 |
| 88 | 15-12 4-7 14-11 | L18 |
| 89 | 15-12 4-7 18-15 | L26 |
| 90 | 15-12 5-7 12-8 | D |
| 91 | 15-12 5-7 12-9 | W29 |
| 92 | 15-12 5-7 13-10 | L26 |
| 93 | 15-12 5-7 13-11 | D |
| 94 | 15-12 5-7 14-11 | L18 |
| 95 | 15-12 5-7 18-15 | L28 |
| 96 | 15-12 5-8 12x5 | W33 |
| 97 | 15-12 6-8 12-9 | D |
| 98 | 15-12 6-8 13-10 | L22 |
| 99 | 15-12 6-8 13-11 | L22 |
| 100 | 15-12 6-8 14-11 | L22 |
| 101 | 15-12 6-8 18-15 | D |
| 102 | 15-12 6-9 12-8 | L32 |
| 103 | 15-12 6-9 13-10 | L26 |
| 104 | 15-12 6-9 13-11 | D |
| 105 | 15-12 6-9 14-11 | L18 |
| 106 | 15-12 6-9 18-15 | L32 |
