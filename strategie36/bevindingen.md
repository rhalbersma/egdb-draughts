# Strategie der 36 velden: controle met exacte eindspeldatabases

Herm Jan Brascamp, *Strategie der 36 velden* ([hijkendtc.nl](https://www.hijkendtc.nl/strategie-der-36-velden-hjb/)),
24 secties met ruim 100 diagrammen, systematisch nagerekend met exacte databases voor het 6×6-bord.
Elke stelling is opgezocht of uitgerekend tot het einde van de partij: winst, remise of verlies, met de
afstand tot het einde in ply (DTM).

## Belangrijkste bevindingen

### De beginstand van 6×6 is remise

Met de databases tot en met 10 stukken en de volledige openingsboom is de beginstand exact opgelost: remise
(details en de tabel van alle 106 openingen van 3 ply in [`results/opening_6x6.md`](../results/opening_6x6.md)).
Alle vijf eerste zetten van wit houden remise. Na 1. 13-11 6-8 2. 11×6 is het remise, maar 2. … 2×9?? (de
randslag van hoofdstuk 7) verliest.

### Hoofdstuk 6/7: de randslag-opening 1. 13-11 6-8 2. 11×6 2×9

* **3. 15-12 wint snel (bevestigd):** winst in 25 ply.
* **Ook 3. 16-13 wint, anders dan het boek concludeert.** Na 3. 16-13 verliest zwart met elke zet (diagram 7.3).
  3. … 5-8 houdt het het langst vol: verlies in 52 ply.
* **De hoofdvariant is correct tot en met zet 9:**
  4. 14-11 1-5 5. 11×6 9×2 6. 15-12 5-7 7. 17-14 7-10 8. 13-11 4-7 9. 11×4 10-13.
  Daarna is **10. 4-1? de fout** die de winst weggeeft. **10. 12-8!** wint (39 ply), bijvoorbeeld
  10. … 13-16 11. 14-11 2-5 12. 8×1 16×6 13. 18-14.
  * Diagram 7.22 (zwart aan zet na 7. 17-14) is daarmee verloren voor zwart.
  * Diagram 6.9 klopt op zichzelf: met wit aan zet wint wit (18-15 of 1-15), met zwart aan zet is het
    remise. De nieuwe remise-conclusie van Jansen en Brascamp voor 6.9 houdt dus stand. Alleen ontstaat
    6.9 pas na de witte fout 10. 4-1.
#### De stelling na de randslag (diagram 7.1)

1. 13-11 6-8 2. 11×6 2×9, wit aan zet; wit wint (w/b = witte/zwarte schijf, W/B = dam):

```text
    +-------------+
  6 |   b   .   b |          1     2     3
  5 | b   b   .   |       4     5     6
  4 |   .   .   b |          7     8     9
  3 | .   .   .   |      10    11    12
  2 |   .   w   w |         13    14    15
  1 | w   w   w   |      16    17    18
    +-------------+
      a b c d e f
```

| zet | waarde |
|---|---|
| **15-12** | wit wint in 25 ply |
| **16-13** | wit wint in 53 ply |
| 14-11 | remise |
| 14-12 | remise |
| 17-13 | remise |

#### De fout op zet 10 in de hoofdvariant

1. 13-11 6-8 2. 11×6 2×9 3. 16-13 5-8 4. 14-11 1-5 5. 11×6 9×2 6. 15-12 5-7 7. 17-14 7-10 8. 13-11 4-7
9. 11×4 10-13, wit aan zet; wit wint, maar alleen met 10. 12-8!:

```text
    +-------------+
  6 |   .   b   b |          1     2     3
  5 | w   .   .   |       4     5     6
  4 |   .   .   . |          7     8     9
  3 | .   .   w   |      10    11    12
  2 |   b   w   . |         13    14    15
  1 | .   .   w   |      16    17    18
    +-------------+
      a b c d e f
```

| zet | waarde |
|---|---|
| **12-8!** | wit wint in 39 ply |
| 4-1? (boek) | remise: 13-16 11. 12-9 geeft diagram 6.9 met zwart aan zet |
| 12-9 | remise |
| 14-11 | remise |
| 18-15 | remise |

10. 12-8! 13-16 11. 14-11 2-5 12. 8×1 16×6: wit laat zwart een dam halen, haalt er zelf ook een en houdt de
schijven op 4 en 18. Het resterende eindspel, met wit aan zet, is gewonnen (33 ply), alleen met 18-14 of 1-15:

```text
    +-------------+
  6 |   W   .   b |          1     2     3
  5 | w   .   B   |       4     5     6
  4 |   .   .   . |          7     8     9
  3 | .   .   .   |      10    11    12
  2 |   .   .   . |         13    14    15
  1 | .   .   w   |      16    17    18
    +-------------+
      a b c d e f
```

Wit: dam 1, schijven 4 en 18; zwart: schijf 3, dam 6. Een verwant van diagram 2.2/3.1, met de dam op 1 in plaats
van de schijf op 10. Verder: 13. 18-14 6-13 14. 1-15 13-6 15. 4-1 6-13 16. 1-4 13-10 17. 4-11 10-2 18. 14-12 3-6
19. 11×3 2-9 20. 12-8 9-2 21. 15-12 2-7 22. 3-6 7-2 23. 6-9 2-7 24. 9-2 7-4 25. 12-15 4-14 26. 2-7 14×4
27. 15-18 4-1 28. 18-15 1×12 29. 15×8 en zwart heeft geen stukken meer.

#### Overige openingsbevindingen

* **Diagram 7.13 (wit aan zet, volgens het boek remise) is winst voor wit:** 9-6! 8×3 13-11! 7×14 18×11
  (13 ply). De remise na 6. 17-14 9-12 7. 14×9 bestaat wel, maar alleen met 7. … 4-7 (diagram 7.14 is
  inderdaad remise); 7. … 5-7 verliest.
* **Diagram 7.6 (na 4. 13-10 1-5) is remise, niet winst:** 5. 17-13 9-12! houdt stand. Het boek speelt
  5. … 3-6, en pas die zet verliest.
* **Diagram 7.2:** de "remisecombinatie" 11-7 die wit "voor de zekerheid" neemt, is juist de enige zet
  die niet verliest. Wit komt dus niet bedrogen uit.
* **Puzzel 7.15:** na 13. 15-12 4-1 wint wit niet; 4-1 is een van de twee remisezetten (de andere is 5-7).
* **Sectie 7.2:**
  * In de variant "5. … 1-5 6. 17-14!" geeft 17-14 de winst weg; 6. 18-14 wint.
  * "6. 17-14 7-10, meteen gewonnen voor zwart" is remise.
* **Sectie 7.3:** "4. 13-11 8×13 5. 17×10 4-7 6. 10×5 1×8 verliest wit zelfs" is remise.

### Eindspelen

* **Diagram 3.4:** na 1. 10-7 6-11 wint 7-4 niet (remise). Alleen **2. 15-18!** wint: 11×4 3. 14-11 4×14
  4. 18×11 3-6 5. 11×3. Dat 10-7 in 3.4 voorbarig is en dat 15-5 of 15-1 wint, klopt.
* **Diagram 8.7 ("remise, er valt niets zinnigs op te bouwen") is winst voor wit**, zowel met wit (39 ply)
  als met zwart aan zet (40 ply). Bijvoorbeeld 2-10 3-6 1-5 6-3 10-17 3-16 14-12 16-6 5-10 6-16 10-2 16-11
  17-14 11×18 12-8 …
* **Hoofdstuk 4, "bereikt wit helemaal niets":** voor de concrete stellingen bij deze tekst (tweede schijf
  op 14, dam op 18 en een vrije dam; diagram 4.12 met de zwarte dam op 13 of 16) wint wit, met wit en met
  zwart aan zet. Let op: de stukopstelling bij deze zinnen heb ik zelf afgeleid uit de tekst.
* **Diagram 5.7:**
  * Variant A (1. 9-2 3-13): wit moet direct slaan (2×17, twee stukken), dus 2. 14-12 kan niet en
    1. … 3-13 verliest meteen.
  * "Niet 6-3" in de hoofdvariant: 6-3 wint even snel als 12-8.
* **"De enige zet":** bij 3.3 (10-7 en 4-1), 9.8 (2-10) en 9.12 (8-5) winnen ook andere zetten. Die
  alternatieven zijn trager en keren terug naar eerdere stellingen. In de probleemconventie van het boek
  (herhaling telt niet als vooruitgang) klopt de bewering dus.

### Bevestigd

Alle overige diagrammen en vrijwel alle varianten kloppen, waaronder:

* de winstsystemen van hoofdstuk 1 t/m 5 (Scouppe, finesses 1 t/m 5, de vangstellingen);
* de compositie van Jannes van der Wal (6.1) en de opgaven 6.3 t/m 6.7;
* Manoury (6.6, 8.2);
* de kerstpuzzel van Hans Jansen (7.4: toch wint wit, alleen met 5. 14-11);
* het taaie eindspel van hoofdstuk 9 (diagram 9.1: alleen 2-7 wint, 49 ply), met de slotfinesse
  21. 15-12! 17×9 22. 10-17.

### Notatiefouten

28 varianten bevatten een onmogelijke zet. Meestal is de bedoeling duidelijk; dan is ook de
gecorrigeerde variant nagerekend (zie de tabellen):

* 1.1 en 4.1: 4. … 2-5 moet 10-5 zijn.
* 4.2: 2. 2-4 moet 2-5 zijn.
* 3.1: "op 11-6 komt 14-11" moet 18-14 zijn.
* 3.3: 12. 5-2 moet 10-2 zijn.
* 6.2: 7. 17-14 2-7 moet 2-5 zijn.
* 6.9: 2. 1-4 16-8 moet 13-8 zijn.
* 6.10 en 9.2: 3. … 10-13 moet 11-13 zijn.
* 6.13: 12. 2-13 moet 2-10 zijn.
* 9.4: 22. 13-17 moet 10-17 zijn.
* 7.3 en 8.1: 3. 16-11 moet 16-13 zijn.
* 7.3: 6. 14×10 moet 14×7 zijn.
* 7.16: 8. … 5×11 moet 4×11 zijn.
* 7.17: 7. … 2×10 moet 2×7 zijn.
* Sectie 7.7: 3. … 5-7 moet 5-8 zijn.

