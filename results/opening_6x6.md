# Openingen van 6×6-dammen

Beginstand, wit aan zet: **remise**.

Methode: de volledige openings-DAG (transposities samengevoegd), uitgebreid zolang een stelling meer dan 10 stukken heeft en hoogstens één kant dammen. Bladeren: databasestellingen (exact), stellingen zonder zetten (verloren) en stellingen met meer stukken en dammen aan beide kanten (onbekend). Waarden zijn als intervallen teruggerekend, vanuit wit gezien.

DAG: 193708 inwendige stellingen, bladeren: database 175882, unknown 5101, unknown, decided in one ply 3848, no moves 56.
Onbekende bladeren die de beginstand nog kunnen beïnvloeden: 0.

## Boom

```
beginstand  remise
    wit 13-10  remise
        zwart 4-7  wit wint
            wit 14-11  zwart wint
            wit 14-12  wit wint
            wit 15-12  zwart wint
            wit 16-13  remise
            wit 17-13  remise
        zwart 5-7  remise
            wit 10x5  remise
        zwart 5-8  remise
            wit 10-7  zwart wint
            wit 14-11  zwart wint
            wit 14-12  remise
            wit 15-12  zwart wint
            wit 16-13  remise
            wit 17-13  zwart wint
        zwart 6-8  remise
            wit 10-7  zwart wint
            wit 14-11  zwart wint
            wit 14-12  zwart wint
            wit 15-12  zwart wint
            wit 16-13  remise
            wit 17-13  remise
        zwart 6-9  remise
            wit 10-7  zwart wint
            wit 14-11  remise
            wit 14-12  zwart wint
            wit 15-12  zwart wint
            wit 16-13  remise
            wit 17-13  remise
    wit 13-11  remise
        zwart 4-7  wit wint
            wit 11x4  wit wint
        zwart 5-7  remise
            wit 11-8  zwart wint
            wit 14-12  zwart wint
            wit 15-12  remise
            wit 16-13  remise
            wit 17-13  zwart wint
        zwart 5-8  remise
            wit 11-7  zwart wint
            wit 14-12  remise
            wit 15-12  zwart wint
            wit 16-13  remise
            wit 17-13  remise
        zwart 6-8  remise
            wit 11x6  remise
        zwart 6-9  wit wint
            wit 11-7  zwart wint
            wit 11-8  wit wint
            wit 14-12  zwart wint
            wit 15-12  remise
            wit 16-13  remise
            wit 17-13  remise
    wit 14-11  remise
        zwart 4-7  wit wint
            wit 11x4  wit wint
        zwart 5-7  remise
            wit 11-8  remise
            wit 13-10  zwart wint
            wit 15-12  zwart wint
            wit 17-14  zwart wint
            wit 18-14  zwart wint
        zwart 5-8  remise
            wit 11-7  zwart wint
            wit 13-10  zwart wint
            wit 15-12  zwart wint
            wit 17-14  remise
            wit 18-14  zwart wint
        zwart 6-8  remise
            wit 11x6  remise
        zwart 6-9  remise
            wit 11-7  zwart wint
            wit 11-8  remise
            wit 13-10  remise
            wit 15-12  zwart wint
            wit 17-14  remise
            wit 18-14  zwart wint
    wit 14-12  remise
        zwart 4-7  wit wint
            wit 12-8  wit wint
            wit 12-9  zwart wint
            wit 13-10  wit wint
            wit 13-11  zwart wint
            wit 17-14  zwart wint
            wit 18-14  zwart wint
        zwart 5-7  remise
            wit 12-8  remise
            wit 12-9  remise
            wit 13-10  remise
            wit 13-11  zwart wint
            wit 17-14  zwart wint
            wit 18-14  zwart wint
        zwart 5-8  remise
            wit 12x5  remise
        zwart 6-8  remise
            wit 12-9  remise
            wit 13-10  zwart wint
            wit 13-11  remise
            wit 17-14  zwart wint
            wit 18-14  remise
        zwart 6-9  remise
            wit 12-8  remise
            wit 13-10  zwart wint
            wit 13-11  zwart wint
            wit 17-14  zwart wint
            wit 18-14  zwart wint
    wit 15-12  remise
        zwart 4-7  remise
            wit 12-8  remise
            wit 12-9  remise
            wit 13-10  zwart wint
            wit 13-11  zwart wint
            wit 14-11  zwart wint
            wit 18-15  zwart wint
        zwart 5-7  wit wint
            wit 12-8  remise
            wit 12-9  wit wint
            wit 13-10  zwart wint
            wit 13-11  remise
            wit 14-11  zwart wint
            wit 18-15  zwart wint
        zwart 5-8  wit wint
            wit 12x5  wit wint
        zwart 6-8  remise
            wit 12-9  remise
            wit 13-10  zwart wint
            wit 13-11  zwart wint
            wit 14-11  zwart wint
            wit 18-15  remise
        zwart 6-9  remise
            wit 12-8  zwart wint
            wit 13-10  zwart wint
            wit 13-11  remise
            wit 14-11  zwart wint
            wit 18-15  zwart wint
```

## Alle openingen van 3 ply

106 openingen: zwart wint: 58, wit wint: 8, remise: 40.

| # | opening | resultaat |
|---|---|---|
| 1 | 13-10 4-7 14-11 | zwart wint |
| 2 | 13-10 4-7 14-12 | wit wint |
| 3 | 13-10 4-7 15-12 | zwart wint |
| 4 | 13-10 4-7 16-13 | remise |
| 5 | 13-10 4-7 17-13 | remise |
| 6 | 13-10 5-7 10x5 | remise |
| 7 | 13-10 5-8 10-7 | zwart wint |
| 8 | 13-10 5-8 14-11 | zwart wint |
| 9 | 13-10 5-8 14-12 | remise |
| 10 | 13-10 5-8 15-12 | zwart wint |
| 11 | 13-10 5-8 16-13 | remise |
| 12 | 13-10 5-8 17-13 | zwart wint |
| 13 | 13-10 6-8 10-7 | zwart wint |
| 14 | 13-10 6-8 14-11 | zwart wint |
| 15 | 13-10 6-8 14-12 | zwart wint |
| 16 | 13-10 6-8 15-12 | zwart wint |
| 17 | 13-10 6-8 16-13 | remise |
| 18 | 13-10 6-8 17-13 | remise |
| 19 | 13-10 6-9 10-7 | zwart wint |
| 20 | 13-10 6-9 14-11 | remise |
| 21 | 13-10 6-9 14-12 | zwart wint |
| 22 | 13-10 6-9 15-12 | zwart wint |
| 23 | 13-10 6-9 16-13 | remise |
| 24 | 13-10 6-9 17-13 | remise |
| 25 | 13-11 4-7 11x4 | wit wint |
| 26 | 13-11 5-7 11-8 | zwart wint |
| 27 | 13-11 5-7 14-12 | zwart wint |
| 28 | 13-11 5-7 15-12 | remise |
| 29 | 13-11 5-7 16-13 | remise |
| 30 | 13-11 5-7 17-13 | zwart wint |
| 31 | 13-11 5-8 11-7 | zwart wint |
| 32 | 13-11 5-8 14-12 | remise |
| 33 | 13-11 5-8 15-12 | zwart wint |
| 34 | 13-11 5-8 16-13 | remise |
| 35 | 13-11 5-8 17-13 | remise |
| 36 | 13-11 6-8 11x6 | remise |
| 37 | 13-11 6-9 11-7 | zwart wint |
| 38 | 13-11 6-9 11-8 | wit wint |
| 39 | 13-11 6-9 14-12 | zwart wint |
| 40 | 13-11 6-9 15-12 | remise |
| 41 | 13-11 6-9 16-13 | remise |
| 42 | 13-11 6-9 17-13 | remise |
| 43 | 14-11 4-7 11x4 | wit wint |
| 44 | 14-11 5-7 11-8 | remise |
| 45 | 14-11 5-7 13-10 | zwart wint |
| 46 | 14-11 5-7 15-12 | zwart wint |
| 47 | 14-11 5-7 17-14 | zwart wint |
| 48 | 14-11 5-7 18-14 | zwart wint |
| 49 | 14-11 5-8 11-7 | zwart wint |
| 50 | 14-11 5-8 13-10 | zwart wint |
| 51 | 14-11 5-8 15-12 | zwart wint |
| 52 | 14-11 5-8 17-14 | remise |
| 53 | 14-11 5-8 18-14 | zwart wint |
| 54 | 14-11 6-8 11x6 | remise |
| 55 | 14-11 6-9 11-7 | zwart wint |
| 56 | 14-11 6-9 11-8 | remise |
| 57 | 14-11 6-9 13-10 | remise |
| 58 | 14-11 6-9 15-12 | zwart wint |
| 59 | 14-11 6-9 17-14 | remise |
| 60 | 14-11 6-9 18-14 | zwart wint |
| 61 | 14-12 4-7 12-8 | wit wint |
| 62 | 14-12 4-7 12-9 | zwart wint |
| 63 | 14-12 4-7 13-10 | wit wint |
| 64 | 14-12 4-7 13-11 | zwart wint |
| 65 | 14-12 4-7 17-14 | zwart wint |
| 66 | 14-12 4-7 18-14 | zwart wint |
| 67 | 14-12 5-7 12-8 | remise |
| 68 | 14-12 5-7 12-9 | remise |
| 69 | 14-12 5-7 13-10 | remise |
| 70 | 14-12 5-7 13-11 | zwart wint |
| 71 | 14-12 5-7 17-14 | zwart wint |
| 72 | 14-12 5-7 18-14 | zwart wint |
| 73 | 14-12 5-8 12x5 | remise |
| 74 | 14-12 6-8 12-9 | remise |
| 75 | 14-12 6-8 13-10 | zwart wint |
| 76 | 14-12 6-8 13-11 | remise |
| 77 | 14-12 6-8 17-14 | zwart wint |
| 78 | 14-12 6-8 18-14 | remise |
| 79 | 14-12 6-9 12-8 | remise |
| 80 | 14-12 6-9 13-10 | zwart wint |
| 81 | 14-12 6-9 13-11 | zwart wint |
| 82 | 14-12 6-9 17-14 | zwart wint |
| 83 | 14-12 6-9 18-14 | zwart wint |
| 84 | 15-12 4-7 12-8 | remise |
| 85 | 15-12 4-7 12-9 | remise |
| 86 | 15-12 4-7 13-10 | zwart wint |
| 87 | 15-12 4-7 13-11 | zwart wint |
| 88 | 15-12 4-7 14-11 | zwart wint |
| 89 | 15-12 4-7 18-15 | zwart wint |
| 90 | 15-12 5-7 12-8 | remise |
| 91 | 15-12 5-7 12-9 | wit wint |
| 92 | 15-12 5-7 13-10 | zwart wint |
| 93 | 15-12 5-7 13-11 | remise |
| 94 | 15-12 5-7 14-11 | zwart wint |
| 95 | 15-12 5-7 18-15 | zwart wint |
| 96 | 15-12 5-8 12x5 | wit wint |
| 97 | 15-12 6-8 12-9 | remise |
| 98 | 15-12 6-8 13-10 | zwart wint |
| 99 | 15-12 6-8 13-11 | zwart wint |
| 100 | 15-12 6-8 14-11 | zwart wint |
| 101 | 15-12 6-8 18-15 | remise |
| 102 | 15-12 6-9 12-8 | zwart wint |
| 103 | 15-12 6-9 13-10 | zwart wint |
| 104 | 15-12 6-9 13-11 | remise |
| 105 | 15-12 6-9 14-11 | zwart wint |
| 106 | 15-12 6-9 18-15 | zwart wint |
