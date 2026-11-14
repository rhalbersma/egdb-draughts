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

Voor stellingen met 11 of 12 stukken waarin beide kanten een dam hebben, bestaat geen database. In de volledige
openingsboom komen er 8.949 voor (vanaf ply 11). Ze worden niet weggelaten en niet weggesnoeid: ze krijgen het
ruimst mogelijke interval (van verlies tot winst), verfijnd met hun eigen slagen, en minimax wordt met onder- en
bovengrenzen over de hele boom teruggerekend. Voor de beginstand en voor alle stellingen tot en met ply 4,
inclusief hun afstanden, vallen de grenzen samen: die waarden zijn bewezen, wat die onbekende stellingen ook
waard zijn.

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
    +-------------+     +-------------------+
  6 |   b   .   b |     |     1     2     3 |
  5 | b   b   .   |     |  4     5     6    |
  4 |   .   .   b |     |     7     8     9 |
  3 | .   .   .   |     | 10    11    12    |
  2 |   .   w   w |     |    13    14    15 |
  1 | w   w   w   |     | 16    17    18    |
    +-------------+     +-------------------+
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
    +-------------+     +-------------------+
  6 |   .   b   b |     |     1     2     3 |
  5 | w   .   .   |     |  4     5     6    |
  4 |   .   .   . |     |     7     8     9 |
  3 | .   .   w   |     | 10    11    12    |
  2 |   b   w   . |     |    13    14    15 |
  1 | .   .   w   |     | 16    17    18    |
    +-------------+     +-------------------+
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
    +-------------+     +-------------------+
  6 |   W   .   b |     |     1     2     3 |
  5 | w   .   B   |     |  4     5     6    |
  4 |   .   .   . |     |     7     8     9 |
  3 | .   .   .   |     | 10    11    12    |
  2 |   .   .   . |     |    13    14    15 |
  1 | .   .   w   |     | 16    17    18    |
    +-------------+     +-------------------+
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


## Methode

### Spelregels en notatie

Internationale regels op het 6×6-bord: schijven slaan ook achteruit, vliegende dammen, slagplicht,
meerslag (dammen en schijven tellen even zwaar), Turkse slag, en een schijf promoveert alleen als hij
zijn zet op de laatste rij beëindigt. Wie aan zet is en geen zet heeft, verliest. Velden 1 t/m 18 zoals
in het boek: veld 1 = b6, veld 16 = a1, en de lange lijn is 16-13-11-8-6-3. Wit begint op 13-18.

### Databases

`src/egdb_6x6.cpp` berekent exacte waarden met afstand tot het einde van de partij (DTM in ply's).

* **Slices.** Een slice ligt vast door de velden van de witte en zwarte schijven en het aantal dammen
  per kleur. Binnen een slice veranderen alleen damzetten zonder slag de stelling. Elke andere zet
  (schijfzet, slag, promotie) gaat naar een slice met minder stukken, minder schijven of verder
  gevorderde schijven. De slices vormen dus een acyclische graaf en worden in die volgorde berekend.
* **Leapfrog-retrograde analyse, compact.** Er is één byte DTM per stelling, plus bitmaps voor de
  frontier:
  * Pass 0 zoekt alle uitgangen naar andere slices op.
  * Iteratie *n* (oneven) zoekt de winsten: voorgangers (un-moves van dammen) van stellingen die
    verloren zijn in *n*−1.
  * Iteratie *n* (even) zoekt de verliezen. Kandidaten (voorgangers van nieuw gewonnen stellingen)
    worden eerst in een bitmap verzameld en daarna met voorwaartse zetgeneratie geverifieerd.
  * De DTM wordt per iteratie bijgewerkt. Wat onbeslist blijft, is remise (herhaling).
* **Opbouw.** Alle slices tot en met 8 stukken (1,7 miljard stellingen, 4,5 miljoen slices, 513 s
  op 4 threads) en tot en met 9 stukken (7,3 miljard stellingen, 34 min) worden vooraf gebouwd, met
  kleursymmetrie (180° draaien en de kleuren wisselen), en opgeslagen als één byte-array met een
  offsettabel. Voor de opening is daarnaast eerst een database tot en met 10 stukken gebouwd met, in
  Syzygy-stijl, alleen winst/remise/verlies in 2 bits per stelling voor 9 en 10 stukken (25,7 miljard
  stellingen, 1 u 52 min), en daarna een volledige DTM-laag voor 10 stukken (18,4 miljard stellingen,
  1 byte per stelling, 18,4 GB) in een bestand op schijf dat via mmap wordt beschreven en gelezen, met
  checkpoints na elke groep slices (66 min). De langste DTM tot en met 9 stukken is 100 ply.
* **Boven de vooraf gebouwde grens.**
  * Stellingen met dammen aan beide kanten gaan naar slices die op verzoek worden berekend.
  * Alle andere stellingen kunnen zich niet herhalen: wie geen dam heeft, moet elke zet een schijf
    verzetten of slaan. Die stellingen worden met een forward alpha-beta-zoektocht tot in de database
    opgelost. De zoektocht rekent op mate-scores (winst in n = 30000 − n): een kindwaarde wordt één ply
    langer voor de ouder, en de zoekgrenzen gaan met de inverse afbeelding naar het kind. De
    transpositietabel bewaart grenzen relatief aan de stelling zelf. Omdat de boom acyclisch is en de
    bladeren exact zijn, levert een volledig venster de exacte DTM. Er zijn dus geen
    graph-history-problemen.

### Validatie

* De 6×6-cijfers voor drie dammen tegen één komen exact overeen met de bestaande kings-only-databases
  van deze repository: langste winst 17 ply, en 784 van de 4782 rustige stellingen met wit aan zet
  zijn remise.
* De verificatiepass berekent elke opgeslagen waarde opnieuw uit de waarden van alle opvolgers. Over
  alle 1,725 miljard stellingen tot en met 8 stukken geeft dat 0 fouten, en over alle 25,7 miljard
  stellingen tot en met 10 stukken (DTM: de 9-stukkendatabase plus de 10-stukkenlaag op schijf) ook 0
  fouten (50 minuten op 4 threads).
* Onafhankelijke implementaties geven dezelfde waarden, inclusief DTM:
  * de eerste solver met tellers;
  * retrograde analyse over de bereikbare graaf;
  * memo-minimax.
* De alpha-beta-zoektocht, met de database maar tot 6 stukken, geeft op 20 stellingen met 7 en 8
  stukken uit hoofdstuk 6 en 7 exact dezelfde waarden en DTM's voor elke zet (125 waarden) als de
  database tot 8 stukken.

### Diagrammen en claims

Alle diagrammen zijn uit de afbeeldingen op hijkendtc.nl gelezen en daarna één voor één visueel
gecontroleerd (`claims.py`). Elke bewering uit de tekst is vastgelegd:

* winst of remise per diagram, met wit en/of zwart aan zet;
* elke variant met de geclaimde uitkomst;
* de markeringen `!` en `?`;
* uitspraken als "de enige zet".

`check.py` speelt elke variant zet voor zet na en meldt:

* onmogelijke zetten (notatiefouten);
* zetten waarmee de bevoordeelde partij het resultaat weggeeft;
* `?`-zetten die het resultaat niet veranderen;
* "enige" zetten die niet de enige zijn;
* eindclaims die niet kloppen.

Waar een notatiefout een eenduidige bedoeling heeft, is ook de gecorrigeerde variant nagerekend.

## Resultaten per diagram en variant

Diagramclaims: 97 bevestigd, 15 onjuist. Varianten: 333, waarvan ℹ️ info: 7, ✏️ notatie: 28, ✅ ok: 273, ❌ onjuist: 9, ⚠️ opmerking: 16.

### Diagrammen

Per diagram het bord met de veldnummers en alle zetten van de partij aan zet, sterkste eerst: winst (snelste eerst), remise, verlies (taaiste eerst). De waarde van de stelling geldt voor de partij aan zet; bij een zet voor wie hem doet, met n het aantal ply tot het einde van de partij, de zet meegeteld (winst oneven, verlies even, D = remise). Erachter de claim van het boek, vertaald naar de partij aan zet (W, D, L; ≤D = "wit wint niet"). w/b = witte/zwarte schijf, W/B = dam.

#### Diagram 1.1, wit aan zet

*wit wint*

```text
    +-------------+     +-------------------+     wit aan zet: W13  (boek: W ✅)
  6 |   .   B   . |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 16-3   W13
  4 |   .   .   . |     |     7     8     9 |       1. 16-8   W13
  3 | .   .   .   |     | 10    11    12    |       1. 16-11  W13
  2 |   .   .   b |     |    13    14    15 |       1. 17-9   W13
  1 | W   W   W   |     | 16    17    18    |       1. 16-13  W17
    +-------------+     +-------------------+       1. 17-10  W17
      a b c d e f                                   1. 17-13  W17
                                                    1. 17-14  W17
                                                    1. 16-6   D
                                                    1. 17-12  D
                                                    1. 18-4   D
                                                    1. 18-7   D
                                                    1. 18-11  D
                                                    1. 18-14  D
```

#### Diagram 1.2, wit aan zet

*bedenk een winstsysteem*

```text
    +-------------+     +-------------------+     wit aan zet: W13  (boek: W ✅)
  6 |   W   .   b |     |     1     2     3 |
  5 | .   W   .   |     |  4     5     6    |       1. 12-14  W13
  4 |   .   .   . |     |     7     8     9 |       1. 1-4    D
  3 | .   .   W   |     | 10    11    12    |       1. 5-2    D
  2 |   .   .   . |     |    13    14    15 |       1. 5-7    D
  1 | B   .   .   |     | 16    17    18    |       1. 5-8    D
    +-------------+     +-------------------+       1. 5-10   D
      a b c d e f                                   1. 12-8   D
                                                    1. 12-9   D
                                                    1. 12-15  D
                                                    1. 12-17  D
```

#### Diagram 2.1, wit aan zet

*zwarte dam op de lange lijn, wit wint (schijf op 17)*

```text
    +-------------+     +-------------------+     wit aan zet: W9  (boek: W ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | W   .   .   |     |  4     5     6    |       1. 4-1    W9
  4 |   .   .   w |     |     7     8     9 |       1. 4-7    W13
  3 | .   .   w   |     | 10    11    12    |       1. 4-14   W13
  2 |   .   .   . |     |    13    14    15 |       1. 4-18   W13
  1 | B   w   .   |     | 16    17    18    |       1. 4-11   D
    +-------------+     +-------------------+       1. 9-6    D
      a b c d e f                                   1. 12-8   D
                                                    1. 17-13  D
                                                    1. 17-14  D
```

#### Diagram 2.1b, wit aan zet

*zwarte dam op 3: eerst een paar tempo's*

```text
    +-------------+     +-------------------+     wit aan zet: W15  (boek: W ✅)
  6 |   .   .   B |     |     1     2     3 |
  5 | W   .   .   |     |  4     5     6    |       1. 4-7    W15
  4 |   .   .   w |     |     7     8     9 |       1. 4-14   W15
  3 | .   .   w   |     | 10    11    12    |       1. 4-18   W15
  2 |   .   .   . |     |    13    14    15 |       1. 4-1    W19
  1 | .   w   .   |     | 16    17    18    |       1. 4-11   D
    +-------------+     +-------------------+       1. 9-6    D
      a b c d e f                                   1. 12-8   D
                                                    1. 17-13  D
                                                    1. 17-14  D
```

#### Diagram 2.2, wit aan zet

*wit heeft nog niet eens een dam, toch wint wit*

```text
    +-------------+     +-------------------+     wit aan zet: W45  (boek: W ✅)
  6 |   .   .   b |     |     1     2     3 |
  5 | w   .   B   |     |  4     5     6    |       1. 4-1    W45
  4 |   .   .   . |     |     7     8     9 |       1. 10-7   D
  3 | w   .   .   |     | 10    11    12    |       1. 18-14  D
  2 |   .   .   . |     |    13    14    15 |       1. 18-15  D
  1 | .   .   w   |     | 16    17    18    |
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 2.3, wit aan zet

*door schijf 15 gaat zwart verliezen*

```text
    +-------------+     +-------------------+     wit aan zet: W11  (boek: W ✅)
  6 |   .   W   . |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 17-12  W11
  4 |   .   B   . |     |     7     8     9 |       1. 2-5    D
  3 | .   .   .   |     | 10    11    12    |       1. 2-6    D
  2 |   .   .   b |     |    13    14    15 |       1. 2-7    D
  1 | .   W   W   |     | 16    17    18    |       1. 2-9    D
    +-------------+     +-------------------+       1. 2-10   D
      a b c d e f                                   1. 17-9   D
                                                    1. 17-10  D
                                                    1. 17-13  D
                                                    1. 17-14  D
                                                    1. 18-4   D
                                                    1. 18-7   D
                                                    1. 18-11  D
                                                    1. 18-14  D
```

#### Diagram 3.3, wit aan zet

*wit wint door 10-7*

```text
    +-------------+     +-------------------+     wit aan zet: W33  (boek: W ✅)
  6 |   .   .   b |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 10-7   W33
  4 |   .   .   . |     |     7     8     9 |       1. 15-1   W37
  3 | w   .   .   |     | 10    11    12    |       1. 14-11  D
  2 |   .   w   W |     |    13    14    15 |       1. 14-12  D
  1 | B   .   .   |     | 16    17    18    |       1. 15-5   D
    +-------------+     +-------------------+       1. 15-8   D
      a b c d e f                                   1. 15-12  D
                                                    1. 15-18  D
```

#### Diagram 3.4, wit aan zet

*na 10-7 maakt zwart remise*

```text
    +-------------+     +-------------------+     wit aan zet: W39
  6 |   .   .   b |     |     1     2     3 |
  5 | .   .   B   |     |  4     5     6    |       1. 15-5   W39
  4 |   .   .   . |     |     7     8     9 |       1. 15-1   W43
  3 | w   .   .   |     | 10    11    12    |       1. 10-7   D
  2 |   .   w   W |     |    13    14    15 |       1. 14-11  D
  1 | .   .   .   |     | 16    17    18    |       1. 14-12  D
    +-------------+     +-------------------+       1. 15-12  D
      a b c d e f                                   1. 15-18  D
                                                    1. 15-8   L4
```

#### Diagram 3.5, wit aan zet

*vangstelling*

```text
    +-------------+     +-------------------+     wit aan zet: W11  (boek: W ✅)
  6 |   .   B   b |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 8-12   W11
  4 |   .   W   w |     |     7     8     9 |       1. 8-1    W15
  3 | .   .   .   |     | 10    11    12    |       1. 8-15   W15
  2 |   .   .   . |     |    13    14    15 |       1. 8-13   W17
  1 | .   .   W   |     | 16    17    18    |       1. 8-11   W19
    +-------------+     +-------------------+       1. 8-16   W19
      a b c d e f                                   1. 18-4   W19
                                                    1. 18-11  W19
                                                    1. 18-14  W19
                                                    1. 18-15  W19
                                                    1. 8-5    D
                                                    1. 8-6    D
                                                    1. 9-6    D
                                                    1. 18-7   D
```

#### Diagram 3.5b, wit aan zet

*vangstelling met een dam op 9*

```text
    +-------------+     +-------------------+     wit aan zet: W11  (boek: W ✅)
  6 |   .   B   b |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 8-12   W11
  4 |   .   W   W |     |     7     8     9 |       1. 8-13   W13
  3 | .   .   .   |     | 10    11    12    |       1. 18-4   W13
  2 |   .   .   . |     |    13    14    15 |       1. 8-1    W15
  1 | .   .   W   |     | 16    17    18    |       1. 8-15   W15
    +-------------+     +-------------------+       1. 9-17   W15
      a b c d e f                                   1. 18-11  W15
                                                    1. 18-14  W15
                                                    1. 18-15  W15
                                                    1. 8-11   W17
                                                    1. 8-16   W17
                                                    1. 9-14   W17
                                                    1. 9-12   W19
                                                    1. 8-5    D
                                                    1. 8-6    D
                                                    1. 9-6    D
                                                    1. 18-7   D
```

#### Diagram 3.5c, wit aan zet

*luchtige opgave, wit wint*

```text
    +-------------+     +-------------------+     wit aan zet: W31  (boek: W ✅)
  6 |   .   .   b |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 14-11  W31
  4 |   .   W   B |     |     7     8     9 |       1. 8-1    D
  3 | w   .   .   |     | 10    11    12    |       1. 8-5    D
  2 |   .   w   . |     |    13    14    15 |       1. 8-11   D
  1 | .   .   .   |     | 16    17    18    |       1. 8-12   D
    +-------------+     +-------------------+       1. 8-13   D
      a b c d e f                                   1. 8-15   D
                                                    1. 8-16   D
                                                    1. 10-7   D
                                                    1. 14-12  D
                                                    1. 8-6    L10
```

#### Diagram 4.1a, wit aan zet

*dam op 9: winst langs dezelfde lijnen*

```text
    +-------------+     +-------------------+     wit aan zet: W11  (boek: W ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 16-13  W11
  4 |   .   .   B |     |     7     8     9 |       1. 16-3   W15
  3 | .   .   .   |     | 10    11    12    |       1. 16-8   W15
  2 |   .   .   b |     |    13    14    15 |       1. 16-11  W15
  1 | W   W   W   |     | 16    17    18    |       1. 17-10  W19
    +-------------+     +-------------------+       1. 17-13  W19
      a b c d e f                                   1. 16-6   D
                                                    1. 17-12  D
                                                    1. 17-14  D
                                                    1. 18-4   D
                                                    1. 18-7   D
                                                    1. 18-11  D
                                                    1. 18-14  D
```

#### Diagram 4.1b, wit aan zet

*dam op 10: winst langs dezelfde lijnen*

```text
    +-------------+     +-------------------+     wit aan zet: W9  (boek: W ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 16-6   W9
  4 |   .   .   . |     |     7     8     9 |       1. 16-13  W11
  3 | B   .   .   |     | 10    11    12    |       1. 17-9   W13
  2 |   .   .   b |     |    13    14    15 |       1. 16-3   W15
  1 | W   W   W   |     | 16    17    18    |       1. 16-8   W15
    +-------------+     +-------------------+       1. 16-11  W15
      a b c d e f                                   1. 17-14  W15
                                                    1. 17-12  D
                                                    1. 17-13  D
                                                    1. 18-4   D
                                                    1. 18-7   D
                                                    1. 18-11  D
                                                    1. 18-14  D
```

#### Diagram 4.1c, wit aan zet

*schijf op 18 i.p.v. dam: remise*

```text
    +-------------+     +-------------------+     wit aan zet: D  (boek: D ✅)
  6 |   .   B   . |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 16-3   D
  4 |   .   .   . |     |     7     8     9 |       1. 16-6   D
  3 | .   .   .   |     | 10    11    12    |       1. 16-8   D
  2 |   .   .   b |     |    13    14    15 |       1. 16-11  D
  1 | W   W   w   |     | 16    17    18    |       1. 16-13  D
    +-------------+     +-------------------+       1. 17-9   D
      a b c d e f                                   1. 17-10  D
                                                    1. 17-12  D
                                                    1. 17-13  D
                                                    1. 17-14  D
                                                    1. 18-14  D
```

#### Diagram 4.1d, wit aan zet

*schijf op 16 i.p.v. dam: remise*

```text
    +-------------+     +-------------------+     wit aan zet: D  (boek: D ✅)
  6 |   .   B   . |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 16-13  D
  4 |   .   .   . |     |     7     8     9 |       1. 17-9   D
  3 | .   .   .   |     | 10    11    12    |       1. 17-10  D
  2 |   .   .   b |     |    13    14    15 |       1. 17-12  D
  1 | w   W   W   |     | 16    17    18    |       1. 17-13  D
    +-------------+     +-------------------+       1. 17-14  D
      a b c d e f                                   1. 18-4   D
                                                    1. 18-7   D
                                                    1. 18-11  D
                                                    1. 18-14  D
```

#### Diagram 4.1e, wit aan zet

*schijf op 17 i.p.v. dam: wit wint moeiteloos*

```text
    +-------------+     +-------------------+     wit aan zet: W33  (boek: W ✅)
  6 |   .   B   . |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 17-13  W33
  4 |   .   .   . |     |     7     8     9 |       1. 17-14  W33
  3 | .   .   .   |     | 10    11    12    |       1. 16-3   W37
  2 |   .   .   b |     |    13    14    15 |       1. 16-8   W37
  1 | W   w   W   |     | 16    17    18    |       1. 16-11  W37
    +-------------+     +-------------------+       1. 16-13  W37
      a b c d e f                                   1. 16-6   D
                                                    1. 18-4   D
                                                    1. 18-7   D
                                                    1. 18-11  D
                                                    1. 18-14  D
```

#### Diagram 4.2b, wit aan zet

*zwarte dam op 3 in diagram 4.2: remise*

```text
    +-------------+     +-------------------+     wit aan zet: D  (boek: D ✅)
  6 |   .   W   B |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 2-5    D
  4 |   .   .   . |     |     7     8     9 |       1. 2-6    D
  3 | .   .   .   |     | 10    11    12    |       1. 2-7    D
  2 |   .   .   b |     |    13    14    15 |       1. 2-9    D
  1 | .   W   W   |     | 16    17    18    |       1. 2-10   D
    +-------------+     +-------------------+       1. 17-9   D
      a b c d e f                                   1. 17-10  D
                                                    1. 17-12  D
                                                    1. 17-13  D
                                                    1. 17-14  D
                                                    1. 18-4   D
                                                    1. 18-7   D
                                                    1. 18-11  D
                                                    1. 18-14  D
```

#### Diagram 4.3, wit aan zet

*slotstand met wit aan zet: remise*

```text
    +-------------+     +-------------------+     wit aan zet: D  (boek: D ✅)
  6 |   .   .   B |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 16-6   D
  4 |   .   .   . |     |     7     8     9 |       1. 16-8   D
  3 | .   .   .   |     | 10    11    12    |       1. 16-11  D
  2 |   .   .   b |     |    13    14    15 |       1. 16-13  D
  1 | W   .   W   |     | 16    17    18    |       1. 18-4   D
    +-------------+     +-------------------+       1. 18-7   D
      a b c d e f                                   1. 18-11  D
                                                    1. 18-14  D
```

#### Diagram 4.4, wit aan zet

*spoken: de drie dammen winnen niet*

```text
    +-------------+     +-------------------+     wit aan zet: D  (boek: D ✅)
  6 |   .   W   . |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 2-5    D
  4 |   .   .   . |     |     7     8     9 |       1. 2-6    D
  3 | .   .   .   |     | 10    11    12    |       1. 2-7    D
  2 |   .   .   b |     |    13    14    15 |       1. 2-9    D
  1 | B   W   W   |     | 16    17    18    |       1. 2-10   D
    +-------------+     +-------------------+       1. 17-9   D
      a b c d e f                                   1. 17-10  D
                                                    1. 17-12  D
                                                    1. 17-13  D
                                                    1. 17-14  D
                                                    1. 18-4   D
                                                    1. 18-7   D
                                                    1. 18-11  D
                                                    1. 18-14  D
```

#### Diagram 4.10, wit aan zet

*winstsysteem met drie schijven, dam op 3*

```text
    +-------------+     +-------------------+     wit aan zet: W9  (boek: W ✅)
  6 |   .   .   B |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 14-12  W9
  4 |   .   .   W |     |     7     8     9 |       1. 9-2    W15
  3 | .   .   .   |     | 10    11    12    |       1. 9-12   D
  2 |   .   w   b |     |    13    14    15 |       1. 14-11  D
  1 | .   w   w   |     | 16    17    18    |       1. 17-13  D
    +-------------+     +-------------------+       1. 9-6    L18
      a b c d e f
```

#### Diagram 4.10b, wit aan zet

*idem, dam op 16*

```text
    +-------------+     +-------------------+     wit aan zet: W9  (boek: W ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 9-12   W9
  4 |   .   .   W |     |     7     8     9 |       1. 9-2    W13
  3 | .   .   .   |     | 10    11    12    |       1. 14-11  D
  2 |   .   w   b |     |    13    14    15 |       1. 14-12  D
  1 | B   w   w   |     | 16    17    18    |       1. 17-13  D
    +-------------+     +-------------------+       1. 9-6    L18
      a b c d e f
```

#### Diagram 4.10c, wit aan zet

*idem, dam op 8*

```text
    +-------------+     +-------------------+     wit aan zet: W11  (boek: W ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 14-12  W11
  4 |   .   B   W |     |     7     8     9 |       1. 9-2    W15
  3 | .   .   .   |     | 10    11    12    |       1. 9-12   W15
  2 |   .   w   b |     |    13    14    15 |       1. 14-11  D
  1 | .   w   w   |     | 16    17    18    |       1. 17-13  D
    +-------------+     +-------------------+       1. 9-6    L18
      a b c d e f
```

#### Diagram 4.11, wit aan zet

*winstsysteem twee schijven (op 17) en twee dammen, dam op 3*

```text
    +-------------+     +-------------------+     wit aan zet: W9  (boek: W ✅)
  6 |   W   .   B |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 1-5    W9
  4 |   .   .   w |     |     7     8     9 |       1. 9-6    W11
  3 | .   .   .   |     | 10    11    12    |       1. 1-4    W13
  2 |   .   .   b |     |    13    14    15 |       1. 17-14  W23
  1 | .   w   W   |     | 16    17    18    |       1. 1-8    D
    +-------------+     +-------------------+       1. 1-12   D
      a b c d e f                                   1. 17-13  D
                                                    1. 18-4   D
                                                    1. 18-7   D
                                                    1. 18-11  D
                                                    1. 18-14  D
```

#### Diagram 4.11b, wit aan zet

*idem, dam op 16*

```text
    +-------------+     +-------------------+     wit aan zet: W7  (boek: W ✅)
  6 |   W   .   . |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 1-12   W7
  4 |   .   .   w |     |     7     8     9 |       1. 1-5    W11
  3 | .   .   .   |     | 10    11    12    |       1. 1-4    W13
  2 |   .   .   b |     |    13    14    15 |       1. 17-14  W23
  1 | B   w   W   |     | 16    17    18    |       1. 1-8    D
    +-------------+     +-------------------+       1. 9-6    D
      a b c d e f                                   1. 17-13  D
                                                    1. 18-4   D
                                                    1. 18-7   D
                                                    1. 18-11  D
                                                    1. 18-14  D
```

#### Diagram 4.11c, wit aan zet

*tweede schijf op 14, dam op 18 en een vrije dam: helemaal niets*

```text
    +-------------+     +-------------------+     wit aan zet: W21  (boek: D ❌)
  6 |   W   .   B |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 1-4    W21
  4 |   .   .   w |     |     7     8     9 |       1. 1-5    W21
  3 | .   .   .   |     | 10    11    12    |       1. 1-8    D
  2 |   .   w   b |     |    13    14    15 |       1. 1-12   D
  1 | .   .   W   |     | 16    17    18    |       1. 9-6    D
    +-------------+     +-------------------+       1. 14-11  D
      a b c d e f                                   1. 14-12  D
```

#### Diagram 4.11c, zwart aan zet

*tweede schijf op 14, dam op 18 en een vrije dam: helemaal niets*

```text
    +-------------+     +-------------------+     zwart aan zet: L22  (boek: D ❌)
  6 |   W   .   B |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1… 3-16   L22
  4 |   .   .   w |     |     7     8     9 |       1… 3-13   L20
  3 | .   .   .   |     | 10    11    12    |       1… 3-8    L6
  2 |   .   w   b |     |    13    14    15 |       1… 3-6    L2
  1 | .   .   W   |     | 16    17    18    |       1… 3-11   L2
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 4.11d, wit aan zet

*idem, zwarte dam op 16*

```text
    +-------------+     +-------------------+     wit aan zet: W21  (boek: D ❌)
  6 |   W   .   . |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 1-4    W21
  4 |   .   .   w |     |     7     8     9 |       1. 1-5    W21
  3 | .   .   .   |     | 10    11    12    |       1. 1-8    D
  2 |   .   w   b |     |    13    14    15 |       1. 1-12   D
  1 | B   .   W   |     | 16    17    18    |       1. 9-6    D
    +-------------+     +-------------------+       1. 14-11  D
      a b c d e f                                   1. 14-12  D
```

#### Diagram 4.11d, zwart aan zet

*idem, zwarte dam op 16*

```text
    +-------------+     +-------------------+     zwart aan zet: L22  (boek: D ❌)
  6 |   W   .   . |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1… 16-3   L22
  4 |   .   .   w |     |     7     8     9 |       1… 16-13  L20
  3 | .   .   .   |     | 10    11    12    |       1… 16-8   L6
  2 |   .   w   b |     |    13    14    15 |       1… 16-6   L2
  1 | B   .   W   |     | 16    17    18    |       1… 16-11  L2
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 4.12, wit aan zet

*finesse 4: met de zwarte dam op 3 wint wit*

```text
    +-------------+     +-------------------+     wit aan zet: W7  (boek: W ✅)
  6 |   W   .   B |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 9-6    W7
  4 |   W   .   w |     |     7     8     9 |       1. 1-5    W29
  3 | .   .   .   |     | 10    11    12    |       1. 7-4    W29
  2 |   .   .   b |     |    13    14    15 |       1. 7-10   W29
  1 | .   .   w   |     | 16    17    18    |       1. 7-14   W29
    +-------------+     +-------------------+       1. 7-2    W33
      a b c d e f                                   1. 7-5    W33
                                                    1. 1-4    W35
                                                    1. 1-8    D
                                                    1. 1-12   D
                                                    1. 7-11   D
                                                    1. 18-14  D
```

#### Diagram 4.12b, wit aan zet

*de zwarte dam kan hier op 13 staan (met schijf op 18 bereikt wit niet veel)*

```text
    +-------------+     +-------------------+     wit aan zet: W25  (boek: D ❌)
  6 |   W   .   . |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 1-12   W25
  4 |   W   .   w |     |     7     8     9 |       1. 7-2    W31
  3 | .   .   .   |     | 10    11    12    |       1. 7-4    W31
  2 |   B   .   b |     |    13    14    15 |       1. 7-5    W31
  1 | .   .   w   |     | 16    17    18    |       1. 7-14   W31
    +-------------+     +-------------------+       1. 1-4    W35
      a b c d e f                                   1. 1-5    W35
                                                    1. 7-10   W35
                                                    1. 1-8    D
                                                    1. 7-11   D
                                                    1. 9-6    D
                                                    1. 18-14  D
```

#### Diagram 4.12b, zwart aan zet

*de zwarte dam kan hier op 13 staan (met schijf op 18 bereikt wit niet veel)*

```text
    +-------------+     +-------------------+     zwart aan zet: L32  (boek: D ❌)
  6 |   W   .   . |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1… 13-16  L32
  4 |   W   .   w |     |     7     8     9 |       1… 13-3   L8
  3 | .   .   .   |     | 10    11    12    |       1… 13-8   L6
  2 |   B   .   b |     |    13    14    15 |       1… 13-10  L4
  1 | .   .   w   |     | 16    17    18    |       1… 13-17  L4
    +-------------+     +-------------------+       1… 13-6   L2
      a b c d e f                                   1… 13-11  L2
```

#### Diagram 4.12c, wit aan zet

*de zwarte dam kan hier op 16 staan (met schijf op 18 bereikt wit niet veel)*

```text
    +-------------+     +-------------------+     wit aan zet: W31  (boek: D ❌)
  6 |   W   .   . |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 7-4    W31
  4 |   W   .   w |     |     7     8     9 |       1. 7-14   W31
  3 | .   .   .   |     | 10    11    12    |       1. 7-2    W33
  2 |   .   .   b |     |    13    14    15 |       1. 7-5    W33
  1 | B   .   w   |     | 16    17    18    |       1. 1-4    W35
    +-------------+     +-------------------+       1. 1-5    W35
      a b c d e f                                   1. 7-10   W35
                                                    1. 1-8    D
                                                    1. 1-12   D
                                                    1. 7-11   D
                                                    1. 9-6    D
                                                    1. 18-14  D
```

#### Diagram 4.12c, zwart aan zet

*de zwarte dam kan hier op 16 staan (met schijf op 18 bereikt wit niet veel)*

```text
    +-------------+     +-------------------+     zwart aan zet: L26  (boek: D ❌)
  6 |   W   .   . |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1… 16-13  L26
  4 |   W   .   w |     |     7     8     9 |       1… 16-3   L8
  3 | .   .   .   |     | 10    11    12    |       1… 16-8   L6
  2 |   .   .   b |     |    13    14    15 |       1… 16-6   L2
  1 | B   .   w   |     | 16    17    18    |       1… 16-11  L2
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 5.1, wit aan zet

*Scouppewinst: zwart is uitgeteld*

```text
    +-------------+     +-------------------+     wit aan zet: W35  (boek: W ✅)
  6 |   W   .   B |     |     1     2     3 |
  5 | .   W   .   |     |  4     5     6    |       1. 1-4    W35
  4 |   .   .   . |     |     7     8     9 |       1. 5-2    W35
  3 | b   .   w   |     | 10    11    12    |       1. 5-8    D
  2 |   .   .   . |     |    13    14    15 |       1. 12-8   D
  1 | .   w   .   |     | 16    17    18    |       1. 12-9   D
    +-------------+     +-------------------+       1. 17-13  D
      a b c d e f                                   1. 17-14  D
                                                    1. 5-7    L16
```

#### Diagram 5.1, zwart aan zet

*Scouppewinst: zwart is uitgeteld*

```text
    +-------------+     +-------------------+     zwart aan zet: L22  (boek: L ✅)
  6 |   W   .   B |     |     1     2     3 |
  5 | .   W   .   |     |  4     5     6    |       1… 3-11   L22
  4 |   .   .   . |     |     7     8     9 |       1… 10-13  L18
  3 | b   .   w   |     | 10    11    12    |       1… 3-8    L8
  2 |   .   .   . |     |    13    14    15 |       1… 3-16   L8
  1 | .   w   .   |     | 16    17    18    |       1… 3-6    L4
    +-------------+     +-------------------+       1… 3-13   L4
      a b c d e f
```

#### Diagram 5.1b, wit aan zet

*dam op 16*

```text
    +-------------+     +-------------------+     wit aan zet: W7  (boek: W ✅)
  6 |   W   .   . |     |     1     2     3 |
  5 | .   W   .   |     |  4     5     6    |       1. 17-13  W7
  4 |   .   .   . |     |     7     8     9 |       1. 5-2    W25
  3 | b   .   w   |     | 10    11    12    |       1. 1-4    W35
  2 |   .   .   . |     |    13    14    15 |       1. 5-8    D
  1 | B   w   .   |     | 16    17    18    |       1. 12-8   D
    +-------------+     +-------------------+       1. 12-9   D
      a b c d e f                                   1. 17-14  D
                                                    1. 5-7    L16
```

#### Diagram 5.1c, wit aan zet

*dam op 13*

```text
    +-------------+     +-------------------+     wit aan zet: W3  (boek: W ✅)
  6 |   W   .   . |     |     1     2     3 |
  5 | .   W   .   |     |  4     5     6    |       1. 5-7    W3
  4 |   .   .   . |     |     7     8     9 |       1. 1-4    W35
  3 | b   .   w   |     | 10    11    12    |       1. 5-2    W35
  2 |   B   .   . |     |    13    14    15 |       1. 5-8    D
  1 | .   w   .   |     | 16    17    18    |       1. 12-8   D
    +-------------+     +-------------------+       1. 12-9   D
      a b c d e f                                   1. 17-14  D
```

#### Diagram 5.1d, wit aan zet

*dam op 6*

```text
    +-------------+     +-------------------+     wit aan zet: W3  (boek: W ✅)
  6 |   W   .   . |     |     1     2     3 |
  5 | .   W   B   |     |  4     5     6    |       1. 5-7    W3
  4 |   .   .   . |     |     7     8     9 |       1. 1-4    W35
  3 | b   .   w   |     | 10    11    12    |       1. 5-2    W35
  2 |   .   .   . |     |    13    14    15 |       1. 5-8    D
  1 | .   w   .   |     | 16    17    18    |       1. 12-8   D
    +-------------+     +-------------------+       1. 12-9   D
      a b c d e f                                   1. 17-13  D
                                                    1. 17-14  D
```

#### Diagram 5.1e, wit aan zet

*dam op 11*

```text
    +-------------+     +-------------------+     wit aan zet: W21  (boek: W ✅)
  6 |   W   .   . |     |     1     2     3 |
  5 | .   W   .   |     |  4     5     6    |       1. 17-14  W21
  4 |   .   .   . |     |     7     8     9 |       1. 5-7    W27
  3 | b   B   w   |     | 10    11    12    |       1. 1-4    W35
  2 |   .   .   . |     |    13    14    15 |       1. 5-2    W35
  1 | .   w   .   |     | 16    17    18    |       1. 5-8    D
    +-------------+     +-------------------+       1. 12-8   D
      a b c d e f                                   1. 12-9   D
                                                    1. 17-13  D
```

#### Diagram 5.1f, wit aan zet

*dam op 8*

```text
    +-------------+     +-------------------+     wit aan zet: W7  (boek: W ✅)
  6 |   W   .   . |     |     1     2     3 |
  5 | .   W   .   |     |  4     5     6    |       1. 17-14  W7
  4 |   .   B   . |     |     7     8     9 |       1. 5-2    W13
  3 | b   .   w   |     | 10    11    12    |       1. 1-4    D
  2 |   .   .   . |     |    13    14    15 |       1. 5-7    D
  1 | .   w   .   |     | 16    17    18    |       1. 12-9   D
    +-------------+     +-------------------+       1. 17-13  D
      a b c d e f
```

#### Diagram 5.1g, wit aan zet

*schijf 12 al op 9: remise*

```text
    +-------------+     +-------------------+     wit aan zet: D  (boek: D ✅)
  6 |   W   .   B |     |     1     2     3 |
  5 | .   W   .   |     |  4     5     6    |       1. 1-4    D
  4 |   .   .   w |     |     7     8     9 |       1. 5-2    D
  3 | b   .   .   |     | 10    11    12    |       1. 5-7    D
  2 |   .   .   . |     |    13    14    15 |       1. 5-8    D
  1 | .   w   .   |     | 16    17    18    |       1. 5-12   D
    +-------------+     +-------------------+       1. 5-15   D
      a b c d e f                                   1. 9-6    D
                                                    1. 17-13  D
                                                    1. 17-14  D
```

#### Diagram 5.2, wit aan zet

*winstsysteem, zwart kan nergens heen (dam op 1)*

```text
    +-------------+     +-------------------+     wit aan zet: W17  (boek: W ✅)
  6 |   B   .   . |     |     1     2     3 |
  5 | W   .   .   |     |  4     5     6    |       1. 17-14  W17
  4 |   .   .   . |     |     7     8     9 |       1. 11-3   W21
  3 | b   W   .   |     | 10    11    12    |       1. 11-6   W21
  2 |   .   .   . |     |    13    14    15 |       1. 11-13  W21
  1 | .   w   .   |     | 16    17    18    |       1. 11-16  W21
    +-------------+     +-------------------+       1. 4-7    D
      a b c d e f                                   1. 11-7   D
                                                    1. 11-8   D
                                                    1. 11-14  D
                                                    1. 11-18  D
                                                    1. 17-13  D
```

#### Diagram 5.2b, wit aan zet

*dam op 2*

```text
    +-------------+     +-------------------+     wit aan zet: W9  (boek: W ✅)
  6 |   .   B   . |     |     1     2     3 |
  5 | W   .   .   |     |  4     5     6    |       1. 4-1    W9
  4 |   .   .   . |     |     7     8     9 |       1. 11-8   W13
  3 | b   W   .   |     | 10    11    12    |       1. 11-3   W21
  2 |   .   .   . |     |    13    14    15 |       1. 11-13  W21
  1 | .   w   .   |     | 16    17    18    |       1. 11-16  W21
    +-------------+     +-------------------+       1. 17-14  W21
      a b c d e f                                   1. 4-7    D
                                                    1. 11-6   D
                                                    1. 11-7   D
                                                    1. 11-14  D
                                                    1. 11-18  D
                                                    1. 17-13  D
```

#### Diagram 5.2c, wit aan zet

*dam op 9*

```text
    +-------------+     +-------------------+     wit aan zet: W11  (boek: W ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | W   .   .   |     |  4     5     6    |       1. 11-8   W11
  4 |   .   .   B |     |     7     8     9 |       1. 4-1    W13
  3 | b   W   .   |     | 10    11    12    |       1. 11-3   W15
  2 |   .   .   . |     |    13    14    15 |       1. 11-16  W15
  1 | .   w   .   |     | 16    17    18    |       1. 11-13  W21
    +-------------+     +-------------------+       1. 4-7    D
      a b c d e f                                   1. 11-6   D
                                                    1. 11-7   D
                                                    1. 11-14  D
                                                    1. 11-18  D
                                                    1. 17-13  D
                                                    1. 17-14  D
```

#### Diagram 5.2d, wit aan zet

*dam op 7*

```text
    +-------------+     +-------------------+     wit aan zet: W7  (boek: W ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | W   .   .   |     |  4     5     6    |       1. 11-8   W7
  4 |   B   .   . |     |     7     8     9 |       1. 11-18  W15
  3 | b   W   .   |     | 10    11    12    |       1. 11-3   W21
  2 |   .   .   . |     |    13    14    15 |       1. 11-6   W21
  1 | .   w   .   |     | 16    17    18    |       1. 11-13  W21
    +-------------+     +-------------------+       1. 11-16  W21
      a b c d e f                                   1. 17-14  W23
                                                    1. 4-1    D
                                                    1. 11-14  D
                                                    1. 17-13  D
```

#### Diagram 5.2e, wit aan zet

*dam op 15*

```text
    +-------------+     +-------------------+     wit aan zet: W7  (boek: W ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | W   .   .   |     |  4     5     6    |       1. 4-1    W7
  4 |   .   .   . |     |     7     8     9 |       1. 11-3   W21
  3 | b   W   .   |     | 10    11    12    |       1. 11-6   W21
  2 |   .   .   B |     |    13    14    15 |       1. 11-13  W21
  1 | .   w   .   |     | 16    17    18    |       1. 11-16  W21
    +-------------+     +-------------------+       1. 17-14  W23
      a b c d e f                                   1. 4-7    D
                                                    1. 11-7   D
                                                    1. 11-8   D
                                                    1. 11-14  D
                                                    1. 11-18  D
                                                    1. 17-13  D
```

#### Diagram 5.2f, wit aan zet

*dam op 5*

```text
    +-------------+     +-------------------+     wit aan zet: W13  (boek: W ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | W   B   .   |     |  4     5     6    |       1. 4-1    W13
  4 |   .   .   . |     |     7     8     9 |       1. 4-7    W21
  3 | b   W   .   |     | 10    11    12    |       1. 11-3   W21
  2 |   .   .   . |     |    13    14    15 |       1. 11-6   W21
  1 | .   w   .   |     | 16    17    18    |       1. 11-13  W21
    +-------------+     +-------------------+       1. 11-16  W21
      a b c d e f                                   1. 17-14  W23
                                                    1. 11-7   D
                                                    1. 11-8   D
                                                    1. 11-14  D
                                                    1. 11-18  D
                                                    1. 17-13  D
```

#### Diagram 5.2g, wit aan zet

*dam op 12*

```text
    +-------------+     +-------------------+     wit aan zet: W7  (boek: W ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | W   .   .   |     |  4     5     6    |       1. 4-1    W7
  4 |   .   .   . |     |     7     8     9 |       1. 11-3   W21
  3 | b   W   B   |     | 10    11    12    |       1. 11-6   W21
  2 |   .   .   . |     |    13    14    15 |       1. 11-13  W21
  1 | .   w   .   |     | 16    17    18    |       1. 11-16  W21
    +-------------+     +-------------------+       1. 4-7    D
      a b c d e f                                   1. 11-7   D
                                                    1. 11-8   D
                                                    1. 11-14  D
                                                    1. 11-18  D
                                                    1. 17-13  D
                                                    1. 17-14  D
```

#### Diagram 5.3, wit aan zet

*als de witte schijf al op 9 staat is er geen winst mogelijk*

```text
    +-------------+     +-------------------+     wit aan zet: D  (boek: D ✅)
  6 |   W   .   . |     |     1     2     3 |
  5 | .   W   .   |     |  4     5     6    |       1. 1-4    D
  4 |   .   .   w |     |     7     8     9 |       1. 5-2    D
  3 | b   .   .   |     | 10    11    12    |       1. 5-7    D
  2 |   .   .   . |     |    13    14    15 |       1. 5-8    D
  1 | B   w   .   |     | 16    17    18    |       1. 5-12   D
    +-------------+     +-------------------+       1. 5-15   D
      a b c d e f                                   1. 9-6    D
                                                    1. 17-13  D
                                                    1. 17-14  D
```

#### Diagram 5.3, zwart aan zet

*als de witte schijf al op 9 staat is er geen winst mogelijk*

```text
    +-------------+     +-------------------+     zwart aan zet: D  (boek: D ✅)
  6 |   W   .   . |     |     1     2     3 |
  5 | .   W   .   |     |  4     5     6    |       1… 16-3   D
  4 |   .   .   w |     |     7     8     9 |       1… 16-11  L24
  3 | b   .   .   |     | 10    11    12    |       1… 10-13  L16
  2 |   .   .   . |     |    13    14    15 |       1… 16-6   L4
  1 | B   w   .   |     | 16    17    18    |       1… 16-8   L4
    +-------------+     +-------------------+       1… 16-13  L4
      a b c d e f
```

#### Diagram 5.4, wit aan zet

*remise*

```text
    +-------------+     +-------------------+     wit aan zet: D  (boek: D ✅)
  6 |   W   .   . |     |     1     2     3 |
  5 | .   W   .   |     |  4     5     6    |       1. 1-4    D
  4 |   .   .   w |     |     7     8     9 |       1. 5-2    D
  3 | b   B   w   |     | 10    11    12    |       1. 5-7    D
  2 |   .   .   . |     |    13    14    15 |       1. 5-8    D
  1 | .   .   .   |     | 16    17    18    |       1. 9-6    D
    +-------------+     +-------------------+       1. 12-8   D
      a b c d e f
```

#### Diagram 5.5, wit aan zet

*winstsysteem met een dam op 17*

```text
    +-------------+     +-------------------+     wit aan zet: W29  (boek: W ✅)
  6 |   W   .   B |     |     1     2     3 |
  5 | .   W   .   |     |  4     5     6    |       1. 17-14  W29
  4 |   .   .   w |     |     7     8     9 |       1. 1-4    W33
  3 | b   .   .   |     | 10    11    12    |       1. 5-2    W33
  2 |   .   .   . |     |    13    14    15 |       1. 5-12   W33
  1 | .   W   .   |     | 16    17    18    |       1. 5-15   W33
    +-------------+     +-------------------+       1. 17-12  W33
      a b c d e f                                   1. 5-7    D
                                                    1. 5-8    D
                                                    1. 9-6    D
                                                    1. 17-13  D
```

#### Diagram 5.5b, wit aan zet

*dam op 11*

```text
    +-------------+     +-------------------+     wit aan zet: W19  (boek: W ✅)
  6 |   W   .   . |     |     1     2     3 |
  5 | .   W   .   |     |  4     5     6    |       1. 5-7    W19
  4 |   .   .   w |     |     7     8     9 |       1. 17-14  W23
  3 | b   B   .   |     | 10    11    12    |       1. 1-4    W33
  2 |   .   .   . |     |    13    14    15 |       1. 5-2    W33
  1 | .   W   .   |     | 16    17    18    |       1. 5-12   W33
    +-------------+     +-------------------+       1. 5-15   W33
      a b c d e f                                   1. 17-12  W33
                                                    1. 5-8    D
                                                    1. 9-6    D
                                                    1. 17-13  D
```

#### Diagram 5.5c, wit aan zet

*dam op 16*

```text
    +-------------+     +-------------------+     wit aan zet: W27  (boek: W ✅)
  6 |   W   .   . |     |     1     2     3 |
  5 | .   W   .   |     |  4     5     6    |       1. 17-12  W27
  4 |   .   .   w |     |     7     8     9 |       1. 1-4    W31
  3 | b   .   .   |     | 10    11    12    |       1. 5-2    W31
  2 |   .   .   . |     |    13    14    15 |       1. 17-14  W31
  1 | B   W   .   |     | 16    17    18    |       1. 5-12   W33
    +-------------+     +-------------------+       1. 5-15   W33
      a b c d e f                                   1. 5-7    D
                                                    1. 5-8    D
                                                    1. 9-6    D
                                                    1. 17-13  D
```

#### Diagram 5.6, zwart aan zet

*finesse 5: zwart moet een dam weggeven*

```text
    +-------------+     +-------------------+     zwart aan zet: L24  (boek: L ✅)
  6 |   .   .   B |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1… 3-13   L24
  4 |   .   .   w |     |     7     8     9 |       1… 16-13  L24
  3 | .   .   w   |     | 10    11    12    |       1… 16-8   L20
  2 |   .   W   . |     |    13    14    15 |       1… 3-6    L14
  1 | B   w   .   |     | 16    17    18    |       1… 3-8    L14
    +-------------+     +-------------------+       1… 3-11   L14
      a b c d e f                                   1… 16-6   L14
                                                    1… 16-11  L12
```

#### Diagram 5.7, wit aan zet

*compositie op finesse 5, wit wint*

```text
    +-------------+     +-------------------+     wit aan zet: W43  (boek: W ✅)
  6 |   .   .   B |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 9-2    W43
  4 |   b   .   W |     |     7     8     9 |       1. 9-6    D
  3 | .   .   W   |     | 10    11    12    |       1. 12-1   D
  2 |   .   w   . |     |    13    14    15 |       1. 12-5   D
  1 | .   .   W   |     | 16    17    18    |       1. 12-8   D
    +-------------+     +-------------------+       1. 12-15  D
      a b c d e f                                   1. 14-11  D
                                                    1. 18-15  D
```

#### Diagram 6.1, wit aan zet

*compositie Jannes van der Wal*

```text
    +-------------+     +-------------------+     wit aan zet: W19  (boek: W ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | .   b   .   |     |  4     5     6    |       1. 9-6    W19
  4 |   .   b   w |     |     7     8     9 |       1. 10-7   D
  3 | w   b   w   |     | 10    11    12    |       1. 16-13  D
  2 |   .   .   w |     |    13    14    15 |
  1 | w   .   .   |     | 16    17    18    |
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 6.2, wit aan zet

*na 6. 15-12 5-7 in de randslag-opening*

```text
    +-------------+     +-------------------+     wit aan zet: W45
  6 |   .   b   b |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1. 17-14  W45
  4 |   b   .   . |     |     7     8     9 |       1. 12-8   D
  3 | .   .   w   |     | 10    11    12    |       1. 12-9   D
  2 |   w   .   . |     |    13    14    15 |       1. 13-10  D
  1 | .   w   w   |     | 16    17    18    |       1. 18-14  D
    +-------------+     +-------------------+       1. 18-15  D
      a b c d e f                                   1. 13-11  L12
```

#### Diagram 6.3, wit aan zet

*wit wint snel na 13-10 en na 13-11*

```text
    +-------------+     +-------------------+     wit aan zet: W21  (boek: W ✅)
  6 |   .   .   b |     |     1     2     3 |
  5 | b   b   .   |     |  4     5     6    |       1. 13-10  W21
  4 |   b   .   . |     |     7     8     9 |       1. 13-11  W37
  3 | .   .   w   |     | 10    11    12    |       1. 18-15  D
  2 |   w   w   . |     |    13    14    15 |       1. 12-9   L18
  1 | .   .   w   |     | 16    17    18    |       1. 14-11  L10
    +-------------+     +-------------------+       1. 12-8   L8
      a b c d e f
```

#### Diagram 6.4, zwart aan zet

*zwart maakt remise door 10-13*

```text
    +-------------+     +-------------------+     zwart aan zet: D  (boek: D ✅)
  6 |   .   .   b |     |     1     2     3 |
  5 | b   b   .   |     |  4     5     6    |       1… 10-13  D
  4 |   .   w   . |     |     7     8     9 |       1… 4-7    L18
  3 | b   .   w   |     | 10    11    12    |       1… 5-7    L18
  2 |   .   w   . |     |    13    14    15 |       1… 3-6    L14
  1 | .   .   w   |     | 16    17    18    |
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 6.5, wit aan zet

*de ene witte zet wint door oppositie, de andere niet*

```text
    +-------------+     +-------------------+     wit aan zet: W17  (boek: W ✅)
  6 |   .   .   b |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1. 12-9   W17
  4 |   b   w   . |     |     7     8     9 |       1. 8-5    D
  3 | b   .   w   |     | 10    11    12    |       1. 18-15  D
  2 |   .   w   . |     |    13    14    15 |       1. 14-11  L42
  1 | .   .   w   |     | 16    17    18    |       1. 8-6    L10
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 6.6, wit aan zet

*wit wint (Manoury 1770)*

```text
    +-------------+     +-------------------+     wit aan zet: W25  (boek: W ✅)
  6 |   W   .   b |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1. 12-9   W25
  4 |   .   .   . |     |     7     8     9 |       1. 1-5    D
  3 | .   .   w   |     | 10    11    12    |       1. 12-8   D
  2 |   .   w   . |     |    13    14    15 |       1. 14-11  D
  1 | B   .   w   |     | 16    17    18    |       1. 18-15  D
    +-------------+     +-------------------+       1. 1-8    L16
      a b c d e f
```

#### Diagram 6.6a, wit aan zet

*wit wint door 2-7, 10-13*

```text
    +-------------+     +-------------------+     wit aan zet: W5  (boek: W ✅)
  6 |   .   W   . |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1. 2-7    W5
  4 |   .   .   . |     |     7     8     9 |       1. 2-5    D
  3 | W   .   .   |     | 10    11    12    |       1. 2-6    D
  2 |   .   .   . |     |    13    14    15 |       1. 2-9    D
  1 | B   .   w   |     | 16    17    18    |       1. 10-5   D
    +-------------+     +-------------------+       1. 10-7   D
      a b c d e f                                   1. 10-13  D
                                                    1. 10-17  D
                                                    1. 18-14  D
                                                    1. 18-15  D
```

#### Diagram 6.7, wit aan zet

*wit wint met twee opmerkelijke randzetten*

```text
    +-------------+     +-------------------+     wit aan zet: W39  (boek: W ✅)
  6 |   .   .   b |     |     1     2     3 |
  5 | b   .   b   |     |  4     5     6    |       1. 13-10  W39
  4 |   b   .   . |     |     7     8     9 |       1. 12-9   D
  3 | .   .   w   |     | 10    11    12    |       1. 17-14  L16
  2 |   w   .   w |     |    13    14    15 |       1. 12-8   L10
  1 | .   w   .   |     | 16    17    18    |       1. 13-11  L10
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 6.7a, wit aan zet

*wit wint door 10-7, 9-6*

```text
    +-------------+     +-------------------+     wit aan zet: W7  (boek: W ✅)
  6 |   .   .   B |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1. 10-7   W7
  4 |   .   .   w |     |     7     8     9 |       1. 15-1   W23
  3 | w   .   .   |     | 10    11    12    |       1. 15-5   W25
  2 |   .   .   W |     |    13    14    15 |       1. 15-12  W25
  1 | .   w   .   |     | 16    17    18    |       1. 15-18  W27
    +-------------+     +-------------------+       1. 9-6    D
      a b c d e f                                   1. 15-8   D
                                                    1. 17-13  D
                                                    1. 17-14  D
```

#### Diagram 6.8, zwart aan zet

*na 7. 17-14, zwart aan zet*

```text
    +-------------+     +-------------------+     zwart aan zet: L44
  6 |   .   b   b |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1… 7-10   L44
  4 |   b   .   . |     |     7     8     9 |       1… 2-6    L24
  3 | .   .   w   |     | 10    11    12    |       1… 2-5    L22
  2 |   w   w   . |     |    13    14    15 |       1… 3-6    L22
  1 | .   .   w   |     | 16    17    18    |       1… 7-11   L14
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 6.9, wit aan zet

*wit aan zet wint, zwart aan zet remise*

```text
    +-------------+     +-------------------+     wit aan zet: W41  (boek: W ✅)
  6 |   W   b   b |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 18-15  W41
  4 |   .   .   w |     |     7     8     9 |       1. 1-15   W45
  3 | .   .   .   |     | 10    11    12    |       1. 1-4    D
  2 |   .   w   . |     |    13    14    15 |       1. 1-5    D
  1 | B   .   w   |     | 16    17    18    |       1. 1-12   D
    +-------------+     +-------------------+       1. 9-6    D
      a b c d e f                                   1. 14-11  D
                                                    1. 14-12  D
                                                    1. 1-8    L12
```

#### Diagram 6.9, zwart aan zet

*wit aan zet wint, zwart aan zet remise*

```text
    +-------------+     +-------------------+     zwart aan zet: D  (boek: D ✅)
  6 |   W   b   b |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1… 16-13  D
  4 |   .   .   w |     |     7     8     9 |       1… 2-6    L16
  3 | .   .   .   |     | 10    11    12    |       1… 16-6   L10
  2 |   .   w   . |     |    13    14    15 |       1… 3-6    L8
  1 | B   .   w   |     | 16    17    18    |       1… 16-8   L8
    +-------------+     +-------------------+       1… 16-11  L8
      a b c d e f                                   1… 2-5    L6
```

#### Diagram 6.9a, zwart aan zet

*zwart aan zet verliest na 16-13 door 7-5, 9-6*

```text
    +-------------+     +-------------------+     zwart aan zet: L30  (boek: L ✅)
  6 |   .   b   b |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1… 2-6    L30
  4 |   W   .   w |     |     7     8     9 |       1… 2-5    L28
  3 | .   .   w   |     | 10    11    12    |       1… 3-6    L14
  2 |   .   .   w |     |    13    14    15 |       1… 16-6   L12
  1 | B   .   .   |     | 16    17    18    |       1… 16-11  L8
    +-------------+     +-------------------+       1… 16-8   L6
      a b c d e f                                   1… 16-13  L6
```

#### Diagram 6.10, wit aan zet

*moeizame winst na twee wonderlijke zetten*

```text
    +-------------+     +-------------------+     wit aan zet: W59  (boek: W ✅)
  6 |   .   .   b |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1. 13-10  W59
  4 |   b   .   . |     |     7     8     9 |       1. 12-8   D
  3 | .   .   w   |     | 10    11    12    |       1. 12-9   D
  2 |   w   .   . |     |    13    14    15 |       1. 18-14  D
  1 | .   .   w   |     | 16    17    18    |       1. 18-15  D
    +-------------+     +-------------------+       1. 13-11  L6
      a b c d e f
```

#### Diagram 6.11, wit aan zet

*na 7. … 1-4? in de randslag-opening*

```text
    +-------------+     +-------------------+     wit aan zet: W57  (boek: W ✅)
  6 |   .   .   b |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1. 18-15  W57
  4 |   .   .   . |     |     7     8     9 |       1. 10-7   D
  3 | w   b   w   |     | 10    11    12    |       1. 12-9   D
  2 |   .   .   . |     |    13    14    15 |       1. 12-8   L14
  1 | .   .   w   |     | 16    17    18    |       1. 18-14  L8
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 6.12, wit aan zet

*met wit aan zet remise, hier (zwart aan zet) komt het net mooi uit*

```text
    +-------------+     +-------------------+     wit aan zet: D  (boek: D ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1. 10-7   D
  4 |   .   .   . |     |     7     8     9 |       1. 11-3   D
  3 | w   W   .   |     | 10    11    12    |       1. 11-6   D
  2 |   .   .   w |     |    13    14    15 |       1. 11-8   D
  1 | .   B   .   |     | 16    17    18    |       1. 11-16  D
    +-------------+     +-------------------+       1. 11-18  D
      a b c d e f                                   1. 15-12  D
                                                    1. 11-13  L10
                                                    1. 11-7   L6
                                                    1. 11-14  L6
```

#### Diagram 6.12, zwart aan zet

*met wit aan zet remise, hier (zwart aan zet) komt het net mooi uit*

```text
    +-------------+     +-------------------+     zwart aan zet: L14  (boek: L ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1… 4-7    L14
  4 |   .   .   . |     |     7     8     9 |       1… 17-9   L6
  3 | w   W   .   |     | 10    11    12    |       1… 17-12  L4
  2 |   .   .   w |     |    13    14    15 |       1… 17-13  L4
  1 | .   B   .   |     | 16    17    18    |       1… 17-14  L4
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 6.13, wit aan zet

*stand na 5. … 17-9, wit wint*

```text
    +-------------+     +-------------------+     wit aan zet: W49  (boek: W ✅)
  6 |   .   W   b |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 2-7    W49
  4 |   .   w   B |     |     7     8     9 |       1. 2-5    D
  3 | .   .   .   |     | 10    11    12    |       1. 2-10   D
  2 |   .   .   w |     |    13    14    15 |       1. 8-5    D
  1 | .   .   .   |     | 16    17    18    |       1. 8-6    D
    +-------------+     +-------------------+       1. 15-12  D
      a b c d e f                                   1. 2-6    L10
```

#### Diagram 6.14, wit aan zet

*stand na 5. … 17-14, wit wint*

```text
    +-------------+     +-------------------+     wit aan zet: W35  (boek: W ✅)
  6 |   .   W   b |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 2-10   W35
  4 |   .   w   . |     |     7     8     9 |       1. 2-9    W41
  3 | .   .   .   |     | 10    11    12    |       1. 2-6    W43
  2 |   .   B   w |     |    13    14    15 |       1. 2-5    W45
  1 | .   .   .   |     | 16    17    18    |       1. 2-7    D
    +-------------+     +-------------------+       1. 8-5    D
      a b c d e f                                   1. 8-6    D
                                                    1. 15-12  D
```

#### Diagram 7.1, wit aan zet

*na 1. 13-11 6-8 2. 11x6 2x9??: wit wint snel met 3. 15-12*

```text
    +-------------+     +-------------------+     wit aan zet: W25  (boek: W ✅)
  6 |   b   .   b |     |     1     2     3 |
  5 | b   b   .   |     |  4     5     6    |       1. 15-12  W25
  4 |   .   .   b |     |     7     8     9 |       1. 16-13  W53
  3 | .   .   .   |     | 10    11    12    |       1. 14-11  D
  2 |   .   w   w |     |    13    14    15 |       1. 14-12  D
  1 | w   w   w   |     | 16    17    18    |       1. 17-13  D
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 7.2, wit aan zet

*wit neemt voor de zekerheid een remisecombinatie*

```text
    +-------------+     +-------------------+     wit aan zet: D
  6 |   b   .   . |     |     1     2     3 |
  5 | .   b   b   |     |  4     5     6    |       1. 11-7   D
  4 |   .   .   b |     |     7     8     9 |       1. 16-13  L12
  3 | b   w   w   |     | 10    11    12    |       1. 11-8   L10
  2 |   .   w   w |     |    13    14    15 |       1. 12-8   L8
  1 | w   .   .   |     | 16    17    18    |
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 7.3, zwart aan zet

*na 3. 16-13!?: alleen 3. … 5-8 houdt remise*

```text
    +-------------+     +-------------------+     zwart aan zet: L52  (boek: D ❌)
  6 |   b   .   b |     |     1     2     3 |
  5 | b   b   .   |     |  4     5     6    |       1… 5-8    L52
  4 |   .   .   b |     |     7     8     9 |       1… 3-6    L22
  3 | .   .   .   |     | 10    11    12    |       1… 5-7    L22
  2 |   w   w   w |     |    13    14    15 |       1… 4-7    L18
  1 | .   w   w   |     | 16    17    18    |       1… 9-12   L18
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 7.4j, wit aan zet

*kerstpuzzel Hans Jansen (na 3. … 3-6 4. 13-10 6-8): toch wint wit*

```text
    +-------------+     +-------------------+     wit aan zet: W19  (boek: W ✅)
  6 |   b   .   . |     |     1     2     3 |
  5 | b   b   .   |     |  4     5     6    |       1. 14-11  W19
  4 |   .   b   b |     |     7     8     9 |       1. 14-12  D
  3 | w   .   .   |     | 10    11    12    |       1. 10-7   L18
  2 |   .   w   w |     |    13    14    15 |       1. 15-12  L18
  1 | .   w   w   |     | 16    17    18    |       1. 17-13  L16
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 7.4, wit aan zet

*na 3. 16-13 5-8*

```text
    +-------------+     +-------------------+     wit aan zet: W51  (boek: D ❌)
  6 |   b   .   b |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1. 14-11  W51
  4 |   .   b   b |     |     7     8     9 |       1. 13-10  D
  3 | .   .   .   |     | 10    11    12    |       1. 13-11  D
  2 |   w   w   w |     |    13    14    15 |       1. 14-12  D
  1 | .   w   w   |     | 16    17    18    |       1. 15-12  L20
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 7.5, zwart aan zet

*na 4. 13-10*

```text
    +-------------+     +-------------------+     zwart aan zet: D
  6 |   b   .   b |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1… 1-5    D
  4 |   .   b   b |     |     7     8     9 |       1… 8-11   D
  3 | w   .   .   |     | 10    11    12    |       1… 3-6    L18
  2 |   .   w   w |     |    13    14    15 |       1… 9-12   L16
  1 | .   w   w   |     | 16    17    18    |       1… 4-7    L12
    +-------------+     +-------------------+       1… 8-12   L12
      a b c d e f
```

#### Diagram 7.6, wit aan zet

*na 4. … 1-5: wit wint*

```text
    +-------------+     +-------------------+     wit aan zet: D  (boek: W ❌)
  6 |   .   .   b |     |     1     2     3 |
  5 | b   b   .   |     |  4     5     6    |       1. 10-7   D
  4 |   .   b   b |     |     7     8     9 |       1. 14-11  D
  3 | w   .   .   |     | 10    11    12    |       1. 14-12  D
  2 |   .   w   w |     |    13    14    15 |       1. 17-13  D
  1 | .   w   w   |     | 16    17    18    |       1. 15-12  L18
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 7.7, wit aan zet

*dat ziet er heikel uit voor wit; het enige is 6. 15-12 9x14 7. 17x12*

```text
    +-------------+     +-------------------+     wit aan zet: D  (boek: D ✅)
  6 |   b   .   b |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 15-12  D
  4 |   .   .   b |     |     7     8     9 |       1. 17-14  L20
  3 | w   b   .   |     | 10    11    12    |       1. 10-7   L18
  2 |   .   .   w |     |    13    14    15 |       1. 17-13  L12
  1 | .   w   w   |     | 16    17    18    |       1. 18-14  L12
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 7.8, zwart aan zet

*de enige remise is 7. … 3-6!*

```text
    +-------------+     +-------------------+     zwart aan zet: D  (boek: D ✅)
  6 |   b   .   b |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1… 3-6    D
  4 |   .   .   . |     |     7     8     9 |       1… 1-4    L58
  3 | w   b   w   |     | 10    11    12    |       1… 1-5    L58
  2 |   .   .   . |     |    13    14    15 |       1… 11-13  L12
  1 | .   .   w   |     | 16    17    18    |       1… 11-14  L10
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 7.9, zwart aan zet

*na 4. 14-11*

```text
    +-------------+     +-------------------+     zwart aan zet: L50
  6 |   b   .   b |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1… 1-5    L50
  4 |   .   b   b |     |     7     8     9 |       1… 3-6    L34
  3 | .   w   .   |     | 10    11    12    |       1… 4-7    L16
  2 |   w   .   w |     |    13    14    15 |       1… 8-12   L16
  1 | .   w   w   |     | 16    17    18    |       1… 9-12   L14
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 7.10, wit aan zet

*na 6. 18-14 wint wit, na 6. 17-14 remise*

```text
    +-------------+     +-------------------+     wit aan zet: W35  (boek: W ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | b   b   .   |     |  4     5     6    |       1. 18-14  W35
  4 |   .   b   b |     |     7     8     9 |       1. 13-10  D
  3 | .   .   .   |     | 10    11    12    |       1. 13-11  D
  2 |   w   .   w |     |    13    14    15 |       1. 15-12  D
  1 | .   w   w   |     | 16    17    18    |       1. 17-14  D
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 7.11, wit aan zet

*hoofdvariant na 5. … 9x2*

```text
    +-------------+     +-------------------+     wit aan zet: W47
  6 |   .   b   b |     |     1     2     3 |
  5 | b   b   .   |     |  4     5     6    |       1. 15-12  W47
  4 |   .   .   . |     |     7     8     9 |       1. 13-10  D
  3 | .   .   .   |     | 10    11    12    |       1. 17-14  D
  2 |   w   .   w |     |    13    14    15 |       1. 18-14  D
  1 | .   w   w   |     | 16    17    18    |       1. 13-11  L20
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 7.13, wit aan zet

*wit aan zet, remise*

```text
    +-------------+     +-------------------+     wit aan zet: W13  (boek: D ❌)
  6 |   .   .   . |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1. 9-6    W13
  4 |   b   b   w |     |     7     8     9 |       1. 13-10  D
  3 | .   .   .   |     | 10    11    12    |       1. 13-11  D
  2 |   w   .   w |     |    13    14    15 |       1. 15-12  D
  1 | .   .   w   |     | 16    17    18    |       1. 18-14  L12
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 7.13b, wit aan zet

*met schijf 18 op 17 gewonnen door 13-11*

```text
    +-------------+     +-------------------+     wit aan zet: W5  (boek: W ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1. 13-11  W5
  4 |   b   b   w |     |     7     8     9 |       1. 9-6    W17
  3 | .   .   .   |     | 10    11    12    |       1. 13-10  D
  2 |   w   .   w |     |    13    14    15 |       1. 15-12  D
  1 | .   w   .   |     | 16    17    18    |       1. 17-14  L12
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 7.14, wit aan zet

*wit aan zet, remise*

```text
    +-------------+     +-------------------+     wit aan zet: D  (boek: D ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | .   b   .   |     |  4     5     6    |       1. 9-6    D
  4 |   b   b   w |     |     7     8     9 |       1. 13-10  D
  3 | .   .   .   |     | 10    11    12    |       1. 15-12  D
  2 |   w   .   w |     |    13    14    15 |       1. 18-14  D
  1 | .   .   w   |     | 16    17    18    |       1. 13-11  L18
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 7.14b, wit aan zet

*met schijf 18 op 17 gewonnen door 13-11*

```text
    +-------------+     +-------------------+     wit aan zet: W31  (boek: W ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | .   b   .   |     |  4     5     6    |       1. 13-11  W31
  4 |   b   b   w |     |     7     8     9 |       1. 9-6    D
  3 | .   .   .   |     | 10    11    12    |       1. 13-10  D
  2 |   w   .   w |     |    13    14    15 |       1. 15-12  D
  1 | .   w   .   |     | 16    17    18    |       1. 17-14  D
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 7.15, wit aan zet

*twee puzzels*

```text
    +-------------+     +-------------------+     wit aan zet: D
  6 |   .   .   . |     |     1     2     3 |
  5 | B   b   .   |     |  4     5     6    |       1. 9-6    D
  4 |   .   .   w |     |     7     8     9 |       1. 13-3   D
  3 | .   .   .   |     | 10    11    12    |       1. 13-6   D
  2 |   W   .   w |     |    13    14    15 |       1. 13-8   D
  1 | .   .   .   |     | 16    17    18    |       1. 13-10  D
    +-------------+     +-------------------+       1. 13-16  D
      a b c d e f                                   1. 13-17  D
                                                    1. 15-12  D
                                                    1. 13-11  L12
```

#### Diagram 7.16, wit aan zet

*de beste zet is 6. 15-12*

```text
    +-------------+     +-------------------+     wit aan zet: W47
  6 |   .   b   b |     |     1     2     3 |
  5 | b   b   .   |     |  4     5     6    |       1. 15-12  W47
  4 |   .   .   . |     |     7     8     9 |       1. 13-10  D
  3 | .   .   .   |     | 10    11    12    |       1. 17-14  D
  2 |   w   .   w |     |    13    14    15 |       1. 18-14  D
  1 | .   w   w   |     | 16    17    18    |       1. 13-11  L20
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 7.17, zwart aan zet

*de beste zet is 6. … 5-7, de rest verliest snel*

```text
    +-------------+     +-------------------+     zwart aan zet: L46
  6 |   .   b   b |     |     1     2     3 |
  5 | b   b   .   |     |  4     5     6    |       1… 5-7    L46
  4 |   .   .   . |     |     7     8     9 |       1… 4-7    L22
  3 | .   .   w   |     | 10    11    12    |       1… 2-6    L20
  2 |   w   .   . |     |    13    14    15 |       1… 5-8    L18
  1 | .   w   w   |     | 16    17    18    |       1… 3-6    L14
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 7.18, wit aan zet

*na 6. … 5-7*

```text
    +-------------+     +-------------------+     wit aan zet: W45
  6 |   .   b   b |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1. 17-14  W45
  4 |   b   .   . |     |     7     8     9 |       1. 12-8   D
  3 | .   .   w   |     | 10    11    12    |       1. 12-9   D
  2 |   w   .   . |     |    13    14    15 |       1. 13-10  D
  1 | .   w   w   |     | 16    17    18    |       1. 18-14  D
    +-------------+     +-------------------+       1. 18-15  D
      a b c d e f                                   1. 13-11  L12
```

#### Diagram 7.19, zwart aan zet

*na 7. 18-14: drie van de vier zwarte zetten verliezen*

```text
    +-------------+     +-------------------+     zwart aan zet: D  (boek: D ✅)
  6 |   .   b   b |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1… 2-6    D
  4 |   b   .   . |     |     7     8     9 |       1… 3-6    L18
  3 | .   .   w   |     | 10    11    12    |       1… 7-10   L16
  2 |   w   w   . |     |    13    14    15 |       1… 7-11   L14
  1 | .   w   .   |     | 16    17    18    |       1… 2-5    L12
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 7.20, wit aan zet

*wit heeft een dijk van een stand en bereikt niets*

```text
    +-------------+     +-------------------+     wit aan zet: D  (boek: D ✅)
  6 |   .   .   b |     |     1     2     3 |
  5 | b   .   b   |     |  4     5     6    |       1. 12-8   D
  4 |   .   .   . |     |     7     8     9 |       1. 12-9   D
  3 | b   w   w   |     | 10    11    12    |       1. 17-13  D
  2 |   .   w   . |     |    13    14    15 |       1. 11-7   L10
  1 | .   w   .   |     | 16    17    18    |       1. 11-8   L8
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 7.21, zwart aan zet

*na 7. 18-15*

```text
    +-------------+     +-------------------+     zwart aan zet: D  (boek: D ✅)
  6 |   .   b   b |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1… 2-5    D
  4 |   b   .   . |     |     7     8     9 |       1… 3-6    D
  3 | .   .   w   |     | 10    11    12    |       1… 2-6    L40
  2 |   w   .   w |     |    13    14    15 |       1… 7-10   L20
  1 | .   w   .   |     | 16    17    18    |       1… 7-11   L18
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 7.22, zwart aan zet

*na 7. 17-14: op drie van de vier zetten wint wit*

```text
    +-------------+     +-------------------+     zwart aan zet: L44  (boek: D ❌)
  6 |   .   b   b |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1… 7-10   L44
  4 |   b   .   . |     |     7     8     9 |       1… 2-6    L24
  3 | .   .   w   |     | 10    11    12    |       1… 2-5    L22
  2 |   w   w   . |     |    13    14    15 |       1… 3-6    L22
  1 | .   .   w   |     | 16    17    18    |       1… 7-11   L14
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 7.23, wit aan zet

*slotstand van de hoofdvariant*

```text
    +-------------+     +-------------------+     wit aan zet: D  (boek: D ✅)
  6 |   .   W   . |     |     1     2     3 |
  5 | .   .   b   |     |  4     5     6    |       1. 12-8   D
  4 |   .   .   w |     |     7     8     9 |       1. 2-5    L14
  3 | B   .   w   |     | 10    11    12    |       1. 2-7    L14
  2 |   .   .   w |     |    13    14    15 |
  1 | .   .   .   |     | 16    17    18    |
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 8.1, wit aan zet

*remise*

```text
    +-------------+     +-------------------+     wit aan zet: D  (boek: D ✅)
  6 |   .   .   W |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1. 3-6    D
  4 |   .   .   . |     |     7     8     9 |       1. 3-8    D
  3 | w   .   .   |     | 10    11    12    |       1. 3-11   D
  2 |   .   .   . |     |    13    14    15 |       1. 3-16   D
  1 | .   B   w   |     | 16    17    18    |       1. 10-7   D
    +-------------+     +-------------------+       1. 18-14  D
      a b c d e f                                   1. 18-15  D
                                                    1. 3-13   L12
```

#### Diagram 8.2, wit aan zet

*zwart aan zet wit wint, wit aan zet remise*

```text
    +-------------+     +-------------------+     wit aan zet: D  (boek: D ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1. 10-7   D
  4 |   .   .   . |     |     7     8     9 |       1. 14-11  D
  3 | w   .   .   |     | 10    11    12    |       1. 14-12  D
  2 |   .   w   . |     |    13    14    15 |       1. 17-13  D
  1 | B   w   w   |     | 16    17    18    |       1. 18-15  D
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 8.2, zwart aan zet

*zwart aan zet wit wint, wit aan zet remise*

```text
    +-------------+     +-------------------+     zwart aan zet: L18  (boek: L ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1… 4-7    L18
  4 |   .   .   . |     |     7     8     9 |       1… 16-3   L8
  3 | w   .   .   |     | 10    11    12    |       1… 16-6   L8
  2 |   .   w   . |     |    13    14    15 |       1… 16-8   L8
  1 | B   w   w   |     | 16    17    18    |       1… 16-11  L6
    +-------------+     +-------------------+       1… 16-13  L6
      a b c d e f
```

#### Diagram 8.3, wit aan zet

*wit wint*

```text
    +-------------+     +-------------------+     wit aan zet: W23  (boek: W ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1. 15-1   W23
  4 |   .   .   w |     |     7     8     9 |       1. 15-5   W25
  3 | w   .   .   |     | 10    11    12    |       1. 15-12  W25
  2 |   .   .   W |     |    13    14    15 |       1. 15-18  W27
  1 | B   w   .   |     | 16    17    18    |       1. 9-6    D
    +-------------+     +-------------------+       1. 10-7   D
      a b c d e f                                   1. 15-8   D
                                                    1. 17-13  D
                                                    1. 17-14  D
```

#### Diagram 8.3, zwart aan zet

*wit wint*

```text
    +-------------+     +-------------------+     zwart aan zet: L20  (boek: L ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1… 16-11  L20
  4 |   .   .   w |     |     7     8     9 |       1… 4-7    L16
  3 | w   .   .   |     | 10    11    12    |       1… 16-13  L10
  2 |   .   .   W |     |    13    14    15 |       1… 16-3   L8
  1 | B   w   .   |     | 16    17    18    |       1… 16-6   L4
    +-------------+     +-------------------+       1… 16-8   L4
      a b c d e f
```

#### Diagram 8.4, wit aan zet

*de zwarte dam pendelt tussen 3 en 6: remise*

```text
    +-------------+     +-------------------+     wit aan zet: D  (boek: D ✅)
  6 |   .   .   B |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1. 10-7   D
  4 |   .   .   . |     |     7     8     9 |       1. 12-8   D
  3 | w   .   w   |     | 10    11    12    |       1. 12-9   D
  2 |   .   .   . |     |    13    14    15 |       1. 17-13  D
  1 | .   W   w   |     | 16    17    18    |       1. 17-14  D
    +-------------+     +-------------------+       1. 18-14  D
      a b c d e f                                   1. 18-15  D
```

#### Diagram 8.4b, wit aan zet

*met de zwarte dam op 16 wint Manoury*

```text
    +-------------+     +-------------------+     wit aan zet: W5  (boek: W ✅)
  6 |   .   .   . |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1. 10-7   W5
  4 |   .   .   . |     |     7     8     9 |       1. 12-8   D
  3 | w   .   w   |     | 10    11    12    |       1. 12-9   D
  2 |   .   .   . |     |    13    14    15 |       1. 17-14  D
  1 | B   W   w   |     | 16    17    18    |       1. 18-14  D
    +-------------+     +-------------------+       1. 18-15  D
      a b c d e f                                   1. 17-13  L8
```

#### Diagram 8.5, wit aan zet

*wit wint*

```text
    +-------------+     +-------------------+     wit aan zet: W7  (boek: W ✅)
  6 |   .   W   B |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1. 2-7    W7
  4 |   .   .   w |     |     7     8     9 |       1. 2-5    W21
  3 | .   .   .   |     | 10    11    12    |       1. 2-10   W23
  2 |   .   .   W |     |    13    14    15 |       1. 15-1   W23
  1 | .   w   .   |     | 16    17    18    |       1. 15-5   W23
    +-------------+     +-------------------+       1. 15-12  W23
      a b c d e f                                   1. 15-18  W23
                                                    1. 17-14  W41
                                                    1. 2-6    D
                                                    1. 9-6    D
                                                    1. 15-8   D
                                                    1. 17-13  D
```

#### Diagram 8.5, zwart aan zet

*wit wint*

```text
    +-------------+     +-------------------+     zwart aan zet: L20  (boek: L ✅)
  6 |   .   W   B |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1… 3-11   L20
  4 |   .   .   w |     |     7     8     9 |       1… 3-16   L20
  3 | .   .   .   |     | 10    11    12    |       1… 4-7    L18
  2 |   .   .   W |     |    13    14    15 |       1… 3-6    L6
  1 | .   w   .   |     | 16    17    18    |       1… 3-8    L4
    +-------------+     +-------------------+       1… 3-13   L4
      a b c d e f
```

#### Diagram 8.6, wit aan zet

*zwart ontkomt niet aan Manoury; wit aan zet speelt 2-5*

```text
    +-------------+     +-------------------+     wit aan zet: W23  (boek: W ✅)
  6 |   .   W   B |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1. 2-5    W23
  4 |   .   .   w |     |     7     8     9 |       1. 2-10   W27
  3 | .   .   .   |     | 10    11    12    |       1. 17-10  W27
  2 |   .   .   . |     |    13    14    15 |       1. 17-12  W29
  1 | .   W   w   |     | 16    17    18    |       1. 17-14  W29
    +-------------+     +-------------------+       1. 18-15  W37
      a b c d e f                                   1. 18-14  W39
                                                    1. 2-6    D
                                                    1. 2-7    D
                                                    1. 9-6    D
                                                    1. 17-13  D
```

#### Diagram 8.6, zwart aan zet

*zwart ontkomt niet aan Manoury; wit aan zet speelt 2-5*

```text
    +-------------+     +-------------------+     zwart aan zet: L26  (boek: L ✅)
  6 |   .   W   B |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1… 3-8    L26
  4 |   .   .   w |     |     7     8     9 |       1… 3-11   L22
  3 | .   .   .   |     | 10    11    12    |       1… 4-7    L22
  2 |   .   .   . |     |    13    14    15 |       1… 3-6    L6
  1 | .   W   w   |     | 16    17    18    |       1… 3-16   L6
    +-------------+     +-------------------+       1… 3-13   L4
      a b c d e f
```

#### Diagram 8.7, wit aan zet

*remise, er valt niets zinnigs op te bouwen*

```text
    +-------------+     +-------------------+     wit aan zet: W39  (boek: D ❌)
  6 |   W   W   B |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1. 2-10   W39
  4 |   .   .   . |     |     7     8     9 |       1. 14-12  W39
  3 | .   .   .   |     | 10    11    12    |       1. 1-5    W41
  2 |   .   w   w |     |    13    14    15 |       1. 1-12   W41
  1 | .   .   .   |     | 16    17    18    |       1. 2-5    W41
    +-------------+     +-------------------+       1. 15-12  W45
      a b c d e f                                   1. 1-8    D
                                                    1. 2-6    D
                                                    1. 2-7    D
                                                    1. 2-9    D
                                                    1. 14-11  D
```

#### Diagram 8.7, zwart aan zet

*remise, er valt niets zinnigs op te bouwen*

```text
    +-------------+     +-------------------+     zwart aan zet: L40  (boek: D ❌)
  6 |   W   W   B |     |     1     2     3 |
  5 | b   .   .   |     |  4     5     6    |       1… 3-13   L40
  4 |   .   .   . |     |     7     8     9 |       1… 3-16   L40
  3 | .   .   .   |     | 10    11    12    |       1… 4-7    L14
  2 |   .   w   w |     |    13    14    15 |       1… 3-6    L6
  1 | .   .   .   |     | 16    17    18    |       1… 3-11   L6
    +-------------+     +-------------------+       1… 3-8    L4
      a b c d e f
```

#### Diagram 9.1, wit aan zet

*wit wint*

```text
    +-------------+     +-------------------+     wit aan zet: W49  (boek: W ✅)
  6 |   .   W   b |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 2-7    W49
  4 |   .   w   B |     |     7     8     9 |       1. 2-5    D
  3 | .   .   .   |     | 10    11    12    |       1. 2-10   D
  2 |   .   .   w |     |    13    14    15 |       1. 8-5    D
  1 | .   .   .   |     | 16    17    18    |       1. 8-6    D
    +-------------+     +-------------------+       1. 15-12  D
      a b c d e f                                   1. 2-6    L10
```

#### Diagram 9.2, zwart aan zet

*vangstelling*

```text
    +-------------+     +-------------------+     zwart aan zet: L34  (boek: L ✅)
  6 |   .   B   b |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1… 2-9    L34
  4 |   .   w   . |     |     7     8     9 |       1… 3-6    L16
  3 | .   W   .   |     | 10    11    12    |       1… 2-6    L6
  2 |   .   .   w |     |    13    14    15 |       1… 2-10   L6
  1 | .   .   .   |     | 16    17    18    |       1… 2-5    L4
    +-------------+     +-------------------+       1… 2-7    L4
      a b c d e f
```

#### Diagram 9.3, zwart aan zet

*vangstelling*

```text
    +-------------+     +-------------------+     zwart aan zet: L34  (boek: L ✅)
  6 |   .   .   b |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1… 17-9   L34
  4 |   .   w   . |     |     7     8     9 |       1… 3-6    L16
  3 | .   W   .   |     | 10    11    12    |       1… 17-12  L8
  2 |   .   .   w |     |    13    14    15 |       1… 17-10  L6
  1 | .   B   .   |     | 16    17    18    |       1… 17-13  L4
    +-------------+     +-------------------+       1… 17-14  L4
      a b c d e f
```

#### Diagram 9.4, wit aan zet

*wit wint door 10-13*

```text
    +-------------+     +-------------------+     wit aan zet: W33  (boek: W ✅)
  6 |   .   .   b |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 10-13  W33
  4 |   .   w   B |     |     7     8     9 |       1. 10-2   W41
  3 | W   .   .   |     | 10    11    12    |       1. 10-7   W49
  2 |   .   .   w |     |    13    14    15 |       1. 8-5    D
  1 | .   .   .   |     | 16    17    18    |       1. 8-6    D
    +-------------+     +-------------------+       1. 10-5   D
      a b c d e f                                   1. 10-17  D
                                                    1. 15-12  D
```

#### Diagram 9.5, wit aan zet

*wit wint door 9-14*

```text
    +-------------+     +-------------------+     wit aan zet: W37  (boek: W ✅)
  6 |   .   .   b |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 9-14   W37
  4 |   .   w   W |     |     7     8     9 |       1. 9-2    W41
  3 | B   .   .   |     | 10    11    12    |       1. 9-6    W43
  2 |   .   .   w |     |    13    14    15 |       1. 9-12   W43
  1 | .   .   .   |     | 16    17    18    |       1. 9-17   W43
    +-------------+     +-------------------+       1. 8-5    D
      a b c d e f                                   1. 8-6    D
                                                    1. 15-12  D
```

#### Diagram 9.6, wit aan zet

*wit wint door 2-9*

```text
    +-------------+     +-------------------+     wit aan zet: W39  (boek: W ✅)
  6 |   .   W   b |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 2-9    W39
  4 |   .   w   . |     |     7     8     9 |       1. 2-5    W43
  3 | .   .   .   |     | 10    11    12    |       1. 2-6    W43
  2 |   .   .   w |     |    13    14    15 |       1. 2-7    W43
  1 | .   B   .   |     | 16    17    18    |       1. 2-10   W45
    +-------------+     +-------------------+       1. 8-5    D
      a b c d e f                                   1. 8-6    D
                                                    1. 15-12  D
```

#### Diagram 9.7, wit aan zet

*zwart aan zet wint wit; met wit aan zet remise*

```text
    +-------------+     +-------------------+     wit aan zet: D  (boek: D ✅)
  6 |   .   W   b |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1. 2-5    D
  4 |   .   w   . |     |     7     8     9 |       1. 2-6    D
  3 | .   .   w   |     | 10    11    12    |       1. 2-7    D
  2 |   .   .   B |     |    13    14    15 |       1. 2-9    D
  1 | .   .   .   |     | 16    17    18    |       1. 2-10   D
    +-------------+     +-------------------+       1. 8-5    D
      a b c d e f                                   1. 8-6    D
                                                    1. 12-9   D
```

#### Diagram 9.7, zwart aan zet

*zwart aan zet wint wit; met wit aan zet remise*

```text
    +-------------+     +-------------------+     zwart aan zet: L30  (boek: L ✅)
  6 |   .   W   b |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1… 15-18  L30
  4 |   .   w   . |     |     7     8     9 |       1… 3-6    L24
  3 | .   .   w   |     | 10    11    12    |
  2 |   .   .   B |     |    13    14    15 |
  1 | .   .   .   |     | 16    17    18    |
    +-------------+     +-------------------+
      a b c d e f
```

#### Diagram 9.8, zwart aan zet

*de witte dam heeft zijn ronde voltooid*

```text
    +-------------+     +-------------------+     zwart aan zet: L38  (boek: L ✅)
  6 |   .   W   b |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1… 7-4    L38
  4 |   B   w   . |     |     7     8     9 |       1… 7-18   L38
  3 | .   .   .   |     | 10    11    12    |       1… 7-14   L36
  2 |   .   .   w |     |    13    14    15 |       1… 3-6    L6
  1 | .   .   .   |     | 16    17    18    |       1… 7-10   L6
    +-------------+     +-------------------+       1… 7-5    L4
      a b c d e f                                   1… 7-11   L4
```

#### Diagram 9.12, zwart aan zet

*de magneet*

```text
    +-------------+     +-------------------+     zwart aan zet: L34  (boek: L ✅)
  6 |   .   .   b |     |     1     2     3 |
  5 | .   .   .   |     |  4     5     6    |       1… 14-4   L34
  4 |   .   w   . |     |     7     8     9 |       1… 14-9   L34
  3 | W   .   .   |     | 10    11    12    |       1… 14-18  L34
  2 |   .   B   w |     |    13    14    15 |       1… 14-17  L32
  1 | .   .   .   |     | 16    17    18    |       1… 3-6    L18
    +-------------+     +-------------------+       1… 14-11  L8
      a b c d e f                                   1… 14-12  L8
                                                    1… 14-7   L4
```

#### Diagram 9.slot, wit aan zet

*eindstand*

```text
    +-------------+     +-------------------+     wit aan zet: W5
  6 |   .   .   . |     |     1     2     3 |
  5 | .   w   b   |     |  4     5     6    |       1. 5-1    W5
  4 |   .   .   B |     |     7     8     9 |       1. 5-2    D
  3 | .   .   .   |     | 10    11    12    |       1. 17-10  D
  2 |   .   .   . |     |    13    14    15 |       1. 17-12  D
  1 | .   W   .   |     | 16    17    18    |       1. 17-13  D
    +-------------+     +-------------------+       1. 17-14  D
      a b c d e f
```


### Varianten

De waarden gelden voor de partij die aan het einde van de variant aan zet is (kolom "aan zet"): W, D of L met de afstand in ply; de claim van het boek is naar diezelfde partij vertaald.

#### Sectie 1

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✏️ | 1.1 | 1. 17-9 2-10 2. 16-11 10-2 3. 11-13 2-10 4. 13-17 2-5 5. 9-12 15×8 6. 18-11 8×13 7. 17×2 | – | wit wint | – | *tekst: 4. … 2-5*<br>ONMOGELIJK: zet 8 (zwart) 2-5 is niet legaal in B:WK9,K17,K18:BK10,15; legale zetten: 10-7, 10-5, 10-2, 10-13 |
| ✅ | 1.1 | 1. 17-9 2-10 2. 16-11 10-2 3. 11-13 2-10 4. 13-17 10-5 5. 9-12 15×8 6. 18-11 8×13 7. 17×2 | zwart | L | L0 | *gecorrigeerd: 4. … 10-5* |
| ✅ | 1.2 | 1. 5-2? 3-6 | wit | D | D | *natuurlijk niet 1. 5-2 3-6, remise*<br>bevestigd: 5-2? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 12-14) |
| ✅ | 1.2 | 1. 12-14 16-13 2. 5-10 13-16 3. 1-15 16-6 4. 10-2 6-9 5. 14-17 | zwart | L | L6 |  |

#### Sectie 2

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 2.1 | 1. 4-1 16-3 2. 9-6 3×15 3. 17-14 | zwart | L | L4 |  |
| ✅ | 2.1 | 1. 4-1 16-11 2. 17-14 | zwart | L | L6 |  |
| ✅ | 2.1b | 1. 4-18 3-16 2. 18-14 16-3 3. 14-4 3-16 4. 4-1 | zwart | L | L8 | *eerst een paar tempo's, dan 4-1 enz.* |
| ✅ | 2.2 | 1. 4-1 6-16 2. 18-14 | zwart | L | L36 | *schijf 3 kan niet van zijn plaats* |
| ✅ | 2.3 | 1. 17-12 8-1 2. 2-5 15×8 3. 5×15 | zwart | L | L4 |  |
| ✅ | 2.3 | 1. 17-12 8-13 2. 2-10 | zwart | L | L4 |  |
| ✅ | 2.3 | 1. 17-12 8-3 2. 18-7 15×8 3. 2-6 8-12 4. 6-16 12-15 5. 7-18 | zwart | L | L2 |  |
| ✅ | 2.3 | 1. 17-12 8-16 2. 2-9 15×8 3. 18-11 8×13 4. 9-17 | zwart | L | L0 |  |
| ✅ | 2.3 | 1. 17-12 8-16 2. 2-10 15×8 3. 10-13 | zwart | L | L6 |  |

#### Sectie 3.1

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 2.2 | 1. 18-14 6-9 2. 14-11 9-6 3. 11-7 6-16 | wit | D | D | *gevolgd door 3-6-9, remise*<br>zet 1 (wit) 18-14 geeft de winst weg (remise) (behoudend: 4-1) |
| ✅ | 2.2 | 1. 4-1[enige] | zwart | L | L44 | *dus de enige zet is 1. 4-1*<br>bevestigd: 4-1 is inderdaad de enige behoudende zet |
| ✅ | 2.2 | 1. 4-1 6-16 2. 18-14 16-6 3. 1-15 6-16 | wit | W | W33 | *wit heeft zijn doel bereikt, diagram 3.3* |
| ✅ | 2.2 | 1. 4-1 6-11! 2. 1-15[enige] 11-16 3. 18-14 16-6 4. 15-5! 6-16 5. 5-1 16-6 6. 1-15 6-16 | wit | W | W33 | *zwart draait de tempo's om, wit ook*<br>bevestigd: 1-15 is inderdaad de enige behoudende zet |
| ✏️ | 2.2 | 1. 4-1 6-11 2. 1-15 11-6 3. 14-11 6-16 | – | wit wint | – | *tekst: op 11-6 komt 14-11 6-16 en wit is er weer*<br>ONMOGELIJK: zet 5 (wit) 14-11 is niet legaal in W:W10,K15,18:B3,K6; legale zetten: 18-14, 15-1, 10-7, 15-12, 15-5, 15-8 |
| ✅ | 2.2 | 1. 4-1 6-11 2. 1-15 11-6 3. 18-14 6-16 | wit | W | W33 | *gecorrigeerd: 3. 18-14 6-16 (diagram 3.3)* |
| ✅ | 2.2 | 1. 4-1 6-11 2. 1-15 11-16 3. 18-14 16-6 4. 15-5 6-16 5. 5-1 16-6 6. 1-15 6-9 7. 14-11 9-6 8. 15-8 6-2 9. 11-7 2-6 10. 8-16 6-8 11. 16×6 3×8 12. 7-5 8×1 13. 10-7 | zwart | L | L4 | *allemaal gedwongen zetten voor wit, maar wel gewonnen* |

#### Sectie 3.2

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ⚠️ | 3.3 | 7. 10-7[enige] 16-13 8. 7-4[enige] 13-16 9. 4-1[enige] 16-6 10. 15-5 6-13 11. 5-10 13-16 12. 1-15 16-6 13. 10-2 6-9 14. 14-11 9-17 15. 15-5 17-9 16. 5-10 9-12 17. 2-6 | zwart | L | L2 | zet 1 10-7 (W33) "de enige": ook 15-1 (W37) behoudt/behouden het resultaat<br>zet 5 4-1 (W29) "de enige": ook 15-1 (W33) behoudt/behouden het resultaat<br>bevestigd: 7-4 is inderdaad de enige behoudende zet |
| ✅ | 3.3 | 7. 10-7 16-13 8. 15-1 13-11 | wit | D | D | *op 15-1 komt 13-11*<br>zet 3 (wit) 15-1 geeft de winst weg (remise) (behoudend: 7-4) |
| ✅ | 3.3 | 7. 10-7 16-13 8. 7-5 3-6 | wit | D | D | *op 7-5 komt 3-6*<br>zet 3 (wit) 7-5 geeft de winst weg (remise) (behoudend: 7-4) |
| ℹ️ | 3.3 | 7. 10-7 16-13 8. 7-4 13-16 9. 15-1 16-13 | wit | – | W31 | *op 15-1 moet wit na 16-13 terug naar 15 met herhaling* |
| ✏️ | 3.3 | 7. 10-7 16-13 8. 7-4 13-16 9. 4-1 16-6 10. 15-5 6-13 11. 5-10 13-6 12. 5-2 | – | wit wint | – | *tekst: of 11. … 13-6 12. 5-2*<br>ONMOGELIJK: zet 11 (wit) 5-2 is niet legaal in W:WK1,K10,14:B3,K6; legale zetten: 10-2, 1-15, 10-17, 1-5, 10-7, 10-5, 1-4, 1-12, 1-8, 10-13, 14-11, 14-12 |
| ✅ | 3.3 | 7. 10-7 16-13 8. 7-4 13-16 9. 4-1 16-6 10. 15-5 6-13 11. 5-10 13-6 12. 10-2 | zwart | L | L18 | *gecorrigeerd: 12. 10-2* |
| ℹ️ | 3.3 | 7. 15-1 | zwart | – | L36 | *wit kan niet wachten (na 15-1 of 15-5 terug naar 3.3 met herhaling)* |
| ℹ️ | 3.3 | 7. 15-5 | zwart | – | D | *idem*<br>zet 1 (wit) 15-5 geeft de winst weg (remise) (behoudend: 10-7, 15-1) |
| ✅ | 3.3 | 7. 10-7 16-13 8. 7-4 13-16 9. 4-1 16-6 10. 15-5 6-13 11. 5-10 13-16 12. 1-15 16-6 13. 10-2 6-9 14. 15-12 3-6 | wit | D | D | *op 15-12 is 3-6 remise*<br>zet 15 (wit) 15-12 geeft de winst weg (remise) (behoudend: 14-11) |
| ✅ | 3.3 | 7. 10-7 16-13 8. 7-4 13-16 9. 4-1 16-6 10. 15-5 6-13 11. 5-10 13-16 12. 1-15 16-6 13. 10-2 6-9 14. 14-11 3-6 15. 15-8 | zwart | L | L12 | *op 3-6 gewoon 15-8* |
| ✅ | 3.5 | 1. 18-14 | zwart | L | L18 | *dan wint 18-14* |
| ✅ | 3.5 | 1. 18-4 | zwart | L | L18 | *of 18-4* |
| ✅ | 3.5b | 1. 18-14 | zwart | L | L14 | *het stuk op 9 mag (ook) een dam zijn* |

#### Sectie 3.3

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ❌ | 3.4 | 1. 10-7 6-11 2. 7-4 | zwart | L | D | *niet 1. … 6-11, want dan wint wit na 7-4*<br>zet 1 (wit) 10-7: geeft de winst weg (remise); behoudend waren: 15-5, 15-1<br>zet 3 (wit) 7-4: geeft de winst weg (remise); behoudend waren: 15-18<br>zet 2 (zwart) 6-11 verliest (behoudend: 6-2) |
| ⚠️ | 3.4 | 1. 10-7 6-13 2. 7-4 | zwart | L | L30 | *evenmin 1. … 6-13*<br>zet 1 (wit) 10-7: geeft de winst weg (remise); behoudend waren: 15-5, 15-1<br>zet 2 (zwart) 6-13 verliest (behoudend: 6-2) |
| ⚠️ | 3.4 | 1. 10-7 6-16 2. 7-4 | zwart | L | L30 | *of 6-16*<br>zet 1 (wit) 10-7: geeft de winst weg (remise); behoudend waren: 15-5, 15-1<br>zet 2 (zwart) 6-16 verliest (behoudend: 6-2) |
| ⚠️ | 3.4 | 1. 10-7 6-9 2. 14-11 9-6 3. 15-8 6-2 4. 7-4 2-7 5. 8-6 3×13 6. 4×16 | zwart | L | L0 | *maar ook niet 1. … 6-9*<br>zet 1 (wit) 10-7: geeft de winst weg (remise); behoudend waren: 15-5, 15-1<br>zet 2 (zwart) 6-9 verliest (behoudend: 6-2) |
| ✅ | 3.4 | 1. 10-7 6-2 2. 7-4 3-6 3. 14-11 6-9 4. 4-1 2-6 5. 1-8 6-3 | wit | D | D | *de remise*<br>zet 1 (wit) 10-7 geeft de winst weg (remise) (behoudend: 15-5, 15-1) |
| ✅ | 3.4 | 1. 10-7 6-2 2. 7-4 3-6 3. 14-11 6-9 4. 15-8 2-7 5. 8-3 7×14 6. 3-11 14×7 7. 4×11 9-12 | wit | D | D | zet 1 (wit) 10-7 geeft de winst weg (remise) (behoudend: 15-5, 15-1) |
| ✅ | 3.4 | 1. 10-7 6-2 2. 15-5 3-6 3. 14-11 6-8 | wit | D | D | *op 2. 15-5 is 3-6 3. 14-11 6-8 remise*<br>zet 1 (wit) 10-7 geeft de winst weg (remise) (behoudend: 15-5, 15-1) |
| ✅ | 3.5c | 1. 14-11 9-6 2. 10-7 6-2 3. 7-4 2-7 4. 8-6 3×13 5. 4×16 | zwart | L | L0 | *variant 1* |
| ✅ | 3.5c | 1. 14-11 9-2 2. 11-7 2-6 3. 8-16 6-8 4. 16×6 3×8 5. 7-5 8×1 6. 10-7 1-4 7. 7-5 4-7 8. 5×10 | zwart | L | L0 | *variant 2* |

#### Sectie 4.1

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✏️ | 1.1 | 1. 17-9 2-10 2. 16-11 10-2 3. 11-13 2-10 4. 13-17 2-5 5. 9-12 15×8 6. 18-11 8×13 7. 17×2 | – | wit wint | – | *slotfinesse*<br>ONMOGELIJK: zet 8 (zwart) 2-5 is niet legaal in B:WK9,K17,K18:BK10,15; legale zetten: 10-7, 10-5, 10-2, 10-13 |

#### Sectie 4.2

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 2.3 | 1. 17-12[enige] | zwart | L | L10 | *de winnende zet is 1. 17-12*<br>bevestigd: 17-12 is inderdaad de enige behoudende zet |
| ✏️ | 2.3 | 1. 17-12 8-1 2. 2-4 | – | wit wint | – | *op 1. … 8-1 wint wit meteen door 2. 2-4 (vergelijk 2-5 in hoofdstuk 2)*<br>ONMOGELIJK: zet 3 (wit) 2-4 is niet legaal in W:WK2,K12,K18:BK1,15; legale zetten: 2-5, 12-9, 12-14, 12-17, 2-7, 2-10, 2-6, 2-9, 12-8, 12-5, 18-14, 18-11, 18-7, 18-4 |
| ✅ | 2.3 | 1. 17-12 8-3 2. 18-4 15×8 3. 2-6 8-12 4. 6-16 12-15 5. 4-18 | zwart | L | L2 | *finesse 1* |
| ✅ | 2.3 | 1. 17-12 8-16 2. 2-10 15×8 3. 10-13 | zwart | L | L6 | *finesse 2* |
| ✅ | 2.3 | 1. 17-12 8-16 2. 2-9 15×8 3. 18-11 8×13 4. 9-17 | zwart | L | L0 | *finesse 3* |
| ✅ | 4.2b | 1. 17-12 15×8 2. 2-6 8-12 3. 6-16 12-15 | wit | D | D | *met de zwarte dam op 3 is het remise* |
| ✅ | 4.4 | 1. 17-14 16-13 2. 14-9 13-16 3. 9-17 | zwart | D | D | *spoken* |
| ✅ | 4.4 | 1. 17-14 16-8 2. 14-12 | zwart | L | L10 | *op 16-8 verliest 14-12 (echt)*<br>zet 2 (zwart) 16-8 verliest (behoudend: 16-13, 16-3) |
| ✅ | 4.4 | 1. 17-14 16-3 | wit | D | D | *op 16-3 ziet zwart het spook* |
| ✅ | 4.4 | 1. 17-14 16-13 2. 14-9 13-8 | wit | W | W11 | *geen 13-8 (terecht)*<br>zet 4 (zwart) 13-8 verliest (behoudend: 13-3, 13-16) |
| ✅ | 4.4 | 1. 17-14 16-13 2. 14-9 13-3 | wit | D | D | *en 13-3 (spook)* |

#### Sectie 4.3

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 4.10 | 1. 14-12 15×8 2. 9-6 | zwart | L | L6 | *finesse 1* |
| ✅ | 4.10b | 1. 9-12 15×8 2. 17-13 | zwart | L | L6 | *finesse 2* |
| ✅ | 4.10c | 1. 14-12 8-16 2. 17-14 15×8 3. 14-11 8×13 4. 9-17 | zwart | L | L0 | *finesse 3* |
| ✅ | 4.10c | 1. 14-12 8-3 2. 9-2 15×8 3. 2-6 | zwart | L | L6 | *finesse 1* |
| ✅ | 4.11b | 1. 1-12 15×8 2. 17-13 | zwart | L | L4 |  |
| ✅ | 4.11 | 1. 1-5 3-16 2. 5-12 | zwart | L | L6 |  |
| ✅ | 4.11 | 1. 9-6 3×16 2. 1-12 | zwart | L | L8 | *(of 9-6 3x16 1-12)* |

#### Sectie 5.1

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 5.1b | 1. 17-13 16×15 2. 5-8 15×5 3. 1×8 | zwart | L | L2 |  |
| ✅ | 5.1c | 1. 5-7 | zwart | L | L2 |  |
| ✅ | 5.1d | 1. 5-7 | zwart | L | L2 |  |
| ✅ | 5.1e | 1. 17-14 11×18 2. 5-8 | zwart | L | L18 |  |
| ✅ | 5.1e | 1. 5-7 11×4 2. 12-8 | zwart | L | L24 | *grappig* |
| ✅ | 5.1e | 1. 5-7 11×4 2. 1-8 4-1 | wit | D | D | *en niet 2. 1-8 4-1 =*<br>zet 3 (wit) 1-8 geeft de winst weg (remise) (behoudend: 12-8) |
| ✅ | 5.1f | 1. 17-14 8×15 2. 5-8 | zwart | L | L4 |  |
| ✅ | 5.1 | 1. 5-2 3-16 2. 1-4 16-3 3. 4-18 3-16 4. 18-14 16-3 5. 14-4 3-16 6. 4-1 16-3 7. 2-5 | zwart | L | L22 | *oneven aantal wachtzetten* |
| ✅ | 5.1 | 1. 5-2 3-11 2. 17-14 | zwart | L | L18 | *op 3-11 komt 17-14, 5-8* |
| ✅ | 5.2b | 1. 11-8 | zwart | L | L12 |  |
| ✅ | 5.2c | 1. 11-8 | zwart | L | L10 |  |
| ✅ | 5.2d | 1. 11-8 | zwart | L | L6 |  |
| ✅ | 5.2e | 1. 4-1 | zwart | L | L6 |  |
| ✅ | 5.2f | 1. 4-1 5-2 2. 1-12 | zwart | L | L10 |  |
| ✅ | 5.2g | 1. 4-1 12-9 2. 1-5 | zwart | L | L10 |  |
| ✅ | 5.2 | 1. 17-14 1-5 2. 4-1 | zwart | L | L10 |  |

#### Sectie 5.2

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 5.3 | 1. … 16-3 2. 17-14 10-13 | wit | D | D | *gewoon 2. … 10-13 is nu remise* |
| ✅ | 5.3 | 1. … 16-3 2. 17-14 3-16 3. 14-12 16-11 4. 5-7 11×4 5. 1-8 4-1 | wit | D | D | *nu wel!* |
| ✅ | 5.3 | 1. 17-14 10-13 2. 5-8 13-17 3. 8-3 | zwart | L | L4 | *dan is het meteen uit*<br>zet 2 (zwart) 10-13 verliest (behoudend: 16-3) |
| ✅ | 5.3 | 1. 17-14 16-3 2. 14-12 10-13 3. 9-6 | zwart | L | L4 | *op 2. … 10-13 komt 9-6, 5-8*<br>zet 4 (zwart) 10-13 verliest (behoudend: 3-11, 3-16) |
| ✅ | 5.3 | 1. 17-14 16-3 2. 14-12 3-11 | wit | D | D | *2. … 3-11 kan nu wel* |
| ✅ | 5.3 | 1. 17-14 16-3 2. 14-12 3-16 3. 1-4 10-13? 4. 5-8 13-17 5. 8-3 | zwart | L | L12 | *met 12-8 W+*<br>bevestigd: 10-13? verliest, terecht als fout gemarkeerd (beter: 16-13, 16-3) |
| ✅ | 5.3 | 1. 17-14 16-3 2. 14-12 3-16 3. 1-4 16-3 4. 4-14 10-13 5. 14-17 13-16 6. 5-7! | zwart | L | L18 | *zwart moet een dam weggeven*<br>zet 10 (zwart) 13-16 verliest (behoudend: 3-8) |
| ✅ | 5.3 | 1. 17-14 16-3 2. 14-12 3-16 3. 1-4 16-3 4. 4-14 10-13 5. 14-17 3-8 | wit | D | D | *maar jammer: remise* |
| ✅ | 5.3 | 1. 17-14 16-3 2. 14-12 3-16 3. 1-4 16-3 4. 4-14 3-16 5. 14-17 16-3 | wit | D | D | *zwart blijft heen en weer gaan* |

#### Sectie 5.3

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 5.5b | 1. 5-7 | zwart | L | L18 |  |
| ✅ | 5.5b | 1. 17-14 | zwart | L | L22 |  |
| ✅ | 5.5c | 1. 17-12 16-3 2. 9-6 3×15 3. 5-8 15×5 4. 1×8 | zwart | L | L2 |  |
| ✅ | 5.5 | 1. 1-4 3-16 2. 4-18 16-3 3. 18-14 3-16 4. 14-4 16-3 5. 4-1 | zwart | L | L28 | *oneven aantal stille zetten* |
| ✅ | 5.5 | 1. 17-14 3-16 2. 14-12 | zwart | L | L26 | *het oneven aantal kan ook één zijn* |
| ✅ | 5.5 | 1. 17-14 10-13 2. 14-17 13-16 3. 1-4 | zwart | L | L16 |  |
| ✅ | 5.5 | 1. 17-14 10-13 2. 14-17 13-16 3. 5-7 | zwart | L | L16 |  |

#### Sectie 5.5

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 5.7 | 1. 12-15 7-10 2. 15-1 10-13 3. 1-8 | zwart | D | D | *daarmee bereikt wit niets*<br>zet 1 (wit) 12-15 geeft de winst weg (remise) (behoudend: 9-2) |
| ✅ | 5.7 | 1. 9-2 7-10 2. 12-9 10-13 3. 14-11 13×8 4. 9-6 | zwart | L | L6 | *good old finesse 1* |
| ✅ | 5.7 | 1. 9-2 7-10 2. 12-9 3-8 3. 14-12 | zwart | L | L18 |  |
| ✏️ | 5.7 | 1. 9-2 3-13 2. 14-12 | – | wit wint | – | *zie A*<br>ONMOGELIJK: zet 3 (wit) 14-12 is niet legaal in W:WK2,K12,14,K18:B7,K13; legale zetten: 2x17 |
| ✅ | 5.7 | 1. 9-2 7-10 2. 12-9 3-16 3. 14-12 10-13 4. 2-6 13-17 5. 12-8 17-13 6. 18-11 13-10 7. 6-3 10-13 8. 8-6 | zwart | L | L4 |  |
| ✅ | 5.7 | 1. 9-2 7-10 2. 12-9 3-16 3. 14-12 10-13 4. 12-8 | zwart | L | L12 | *(of 12-8)* |
| ✅ | 5.7 | 1. 9-2 7-10 2. 12-9 3-16 3. 14-12 10-13 4. 9-6? | zwart | ≥D | D | *maar niet 9-6*<br>bevestigd: 9-6? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 2-6, 12-8) |
| ❌ | 5.7 | 1. 9-2 7-10 2. 12-9 3-16 3. 14-12 10-13 4. 2-6 13-17 5. 6-3? | zwart | ≥D | L10 | *niet 6-3*<br>zet 9 (wit) 6-3?: als fout gemarkeerd, maar verandert het resultaat niet (W11 blijft W11) |
| ✅ | 5.7 | 1. 9-2 7-10 2. 12-9 3-16 3. 14-12 16-3 4. 18-14 10-13 5. 14-17 13-16 6. 2-7 | zwart | L | L16 | *finesse 5* |
| ✅ | 5.7 | 1. 9-2 7-10 2. 12-9 3-16 3. 14-12 16-3 4. 18-14 3-16 5. 14-17 | zwart | L | L34 | *bouwt 5.2 of 5.5 op* |
| ✏️ | 5.7 | 1. 9-2 3-13 2. 14-12 13-3 3. 18-14 | – | wit wint | – | *A*<br>ONMOGELIJK: zet 3 (wit) 14-12 is niet legaal in W:WK2,K12,14,K18:B7,K13; legale zetten: 2x17 |
| ✏️ | 5.7 | 1. 9-2 3-13 2. 14-12 13-16 3. 18-14 16-3 4. 14-17 | – | wit wint | – | *A*<br>ONMOGELIJK: zet 3 (wit) 14-12 is niet legaal in W:WK2,K12,14,K18:B7,K13; legale zetten: 2x17 |
| ⚠️ | 5.7 | 1. 12-1 7-10 2. 1-4 10-13 | wit | W | W9 | *ook nu verliest 2. … 10-13*<br>zet 1 (wit) 12-1: geeft de winst weg (remise); behoudend waren: 9-2<br>zet 4 (zwart) 10-13 verliest (behoudend: 3-16) |
| ⚠️ | 5.7 | 1. 12-1 7-10 2. 1-4 3-16 3. 14-12 16-13 4. 18-14 | zwart | L | L30 | zet 1 (wit) 12-1: geeft de winst weg (remise); behoudend waren: 9-2<br>zet 6 (zwart) 16-13 verliest (behoudend: 10-13) |
| ⚠️ | 5.7 | 1. 12-1 7-10 2. 1-4 3-16 3. 14-12 16-3 4. 18-14 | zwart | L | L30 | zet 1 (wit) 12-1: geeft de winst weg (remise); behoudend waren: 9-2<br>zet 6 (zwart) 16-3 verliest (behoudend: 10-13) |
| ✅ | 5.7 | 1. 12-1 7-10 2. 1-4 3-16 3. 14-12 10-13 4. 12-8 13-17 5. 18-11 17-10 | wit | D | D | *remise na 1. 12-1*<br>zet 1 (wit) 12-1 geeft de winst weg (remise) (behoudend: 9-2) |

#### Sectie 6.3

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 6.1 | 1. 10-7 11×4 2. 9-6 | zwart | ≥D | D | *wint niet*<br>zet 1 (wit) 10-7 geeft de winst weg (remise) (behoudend: 9-6) |
| ✅ | 6.1 | 1. 9-6 8×3 2. 12-8 11×6 3. 10-7 5×10 4. 15-12!! 6-9 5. 12-8 3-6 6. 8×3 9-12 7. 3-11 10-13 8. 11-8 | zwart | L | L4 |  |
| ✅ | 6.1 | 1. 9-6 8×3 2. 12-8 11×6 3. 10-7 5×10 4. 15-12 6-9 5. 12-8 3-6 6. 8×3 9-12 7. 3-11 12-15 8. 11-18 | zwart | L | L2 |  |
| ✅ | 6.1 | 1. 9-6 8×3 2. 12-8 11×6 3. 10-7 5×10 4. 15-12 6-9 5. 12-8 9-12 | wit | W | W9 | *9-12 wint door oppositie* |

#### Sectie 6.1

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 6.2 | 7. 18-14 2-6 | wit | D | D | *met 7. … 2-6 haalt zwart snel en elegant remise*<br>zet 1 (wit) 18-14 geeft de winst weg (remise) (behoudend: 17-14) |
| ⚠️ | 6.2 | 7. 18-14 2-5 8. 12-8 | zwart | L | L12 | *(zie 7.19)*<br>zet 1 (wit) 18-14: geeft de winst weg (remise); behoudend waren: 17-14<br>zet 2 (zwart) 2-5 verliest (behoudend: 2-6) |
| ⚠️ | 6.2 | 7. 18-14 7-10 8. 12-8 | zwart | L | L14 | *(zie 7.19)*<br>zet 1 (wit) 18-14: geeft de winst weg (remise); behoudend waren: 17-14<br>zet 2 (zwart) 7-10 verliest (behoudend: 2-6) |
| ⚠️ | 6.2 | 7. 18-14 3-6 8. 13-11 | zwart | L | L16 | *(zie 7.19)*<br>zet 1 (wit) 18-14: geeft de winst weg (remise); behoudend waren: 17-14<br>zet 2 (zwart) 3-6 verliest (behoudend: 2-6) |
| ✏️ | 6.2 | 7. 17-14 2-7 | – | – | – | *tekst: "na 7. 17-14 2-7" (diagram 6.3 toont 2-5)*<br>ONMOGELIJK: zet 2 (zwart) 2-7 is niet legaal in B:W12,13,14,18:B2,3,4,7; legale zetten: 7-10, 2-6, 2-5, 3-6, 7-11 |
| ✅ | 6.2 | 7. 17-14 2-5 | wit | W | W21 | *diagram 6.3* |

#### Sectie 6.3

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 6.3 | 1. 13-10 3-6 2. 12-9 6-8 3. 18-15 8-11 4. 14-12 5-8 5. 10×5 8×1 6. 12-8 | zwart | L | L8 |  |
| ✅ | 6.3 | 1. 13-10 5-8 2. 10×5 8×1 3. 12-8 | zwart | L | L14 |  |
| ✅ | 6.3 | 1. 13-11 7-10 2. 11-7 4×11 3. 14×2 10-13 4. 12-8 13-17 5. 2-7 17-9 6. 7-11 9-14 7. 8-6 | zwart | L | L2 |  |
| ✅ | 6.3 | 1. 13-11 7-10 2. 11-7 4×11 3. 14×2 10-13 4. 12-8 13-17 5. 2-7 17-9 6. 7-11 9-2 7. 18-15 2-9 8. 8-5 | zwart | L | L32 |  |
| ✅ | 6.3 | 1. 13-11 7-10 2. 11-7 4×11 3. 14×2 10-13 4. 12-8 13-17 5. 2-7 17-9 6. 7-11 9-17 7. 8-5 | zwart | L | L30 |  |
| ✅ | 6.3 | 1. 13-11 3-6 2. 12-9 5-8 3. 9×2 8×13 4. 2×17 | zwart | L | L4 |  |

#### Sectie 6.1

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 6.3 | 1. 13-11 7-10 2. 11-8?? | zwart | D | D | *wit kan het nog makkelijk verprutsen*<br>bevestigd: 11-8?? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 11-7) |
| ✅ | 6.4 | 9. … 10-13 10. 8×1 3-6!! | wit | D | D | *zwart maakt nu remise* |

#### Sectie 6.3

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 6.5 | 1. 18-15 7-11 2. 8×13 10×17 3. 12-9 | zwart | ≥D | D | *wint niet*<br>zet 1 (wit) 18-15 geeft de winst weg (remise) (behoudend: 12-9) |
| ✅ | 6.5 | 1. 12-9 7-11 2. 8×13 10×12 3. 9×14 4-7 4. 18-15 3-6 5. 15-12 7-10 6. 14-11 | zwart | L | L6 | *de oplossing* |

#### Sectie 6.2

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 6.4 | 9. … 10-13 10. 8×1 13-16?? | wit | W | W25 | *diagram 6.6*<br>bevestigd: 13-16?? verliest, terecht als fout gemarkeerd (beter: 3-6) |

#### Sectie 6.4

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 6.6 | 1. 12-9 3-6 2. 9×2 16-13 3. 1-5 13-3 4. 5-10 3-16 5. 10-17 16-3 6. 14-11 3×16 7. 2-7 4×11 8. 17-13 11-14 9. 18×11 | zwart | L | L0 | *Manoury 1770* |
| ✅ | 6.6a | 1. 2-7 4×11 2. 10-13 | zwart | L | L2 | *wit wint door 2-7, 10-13* |
| ✅ | 6.7 | 1. 13-10 6-8 2. 10×5 8×1 3. 12-8 | zwart | L | L14 |  |
| ✅ | 6.7 | 1. 13-10 7-11 2. 12-9 6-8 3. 9-6 | zwart | L | L10 |  |
| ✅ | 6.7 | 1. 13-10 7-11 2. 12-9 11-13 3. 9×2 13-16 4. 15-12 3-6 5. 2×9 16-3 6. 9-2 3-16 7. 2-5 16-6 8. 12-9 6-3 9. 5-1 3-16 10. 1-15 16-3 11. 10-7 4×11 12. 9-6 3×8 13. 15×5 | zwart | L | L2 |  |
| ✅ | 6.7 | 1. 13-10 7-11 2. 12-9 11-13 3. 9×2 13-16 4. 15-12 3-6 5. 2×9 16-3 6. 9-2 3-16 7. 12-9? 16-8 | wit | D | D | *12-9? 16-8 =*<br>bevestigd: 12-9? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 2-5, 2-9) |
| ✅ | 6.7 | 1. 13-10 7-11 2. 12-9 11-13 3. 9×2 13-16 4. 15-12 3-6 5. 2×9 16-3 6. 9-2 3-16 7. 2-5 16-8? 8. 5-1 8×15 9. 17-13! | zwart | L | L18 | *16-8? 5-1 8x15 17-13! W+*<br>bevestigd: 16-8?: verliest sneller (L22 i.p.v. L26) |
| ✅ | 6.7a | 1. 10-7 4×11 2. 9-6 | zwart | L | L4 | *wit wint door 10-7, 9-6* |

#### Sectie 6.2

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 6.8 | 7. … 7-10 8. 13-11 4-7 9. 11×4 10-13 10. 4-1 13-16 11. 12-9 | zwart | D | D | *diagram 6.9 met zwart aan zet*<br>zet 6 (wit) 4-1 geeft de winst weg (remise) (behoudend: 12-8) |
| ✅ | 6.9 | 11. … 16-13 12. 18-15 13-16 13. 1-4 3-6 14. 4-1 6-8 15. 1×12 2-5 16. 12×1 | zwart | D | D | *een vervelende variant, standaard-remise* |
| ✅ | 6.9 | 11. … 16-13 12. 18-15 13-16 13. 14-12 3-6 | wit | D | D | *op 14-12 is 3-6 remise* |
| ✅ | 6.9 | 11. … 16-13 12. 18-15 13-16 13. 15-12 3-6 | wit | D | D | *op 15-12 is 3-6 remise* |

#### Sectie 6.5

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 6.9 | 1. 14-11 16×6 2. 1-5 2×7 3. 9×2 7-10 | wit | ≤D | D | *niet meer dan remise*<br>zet 1 (wit) 14-11 geeft de winst weg (remise) (behoudend: 18-15, 1-15) |
| ✅ | 6.9 | 1. 1-4 3-6! 2. 4-1 6-8 3. 1×12 2-5 4. 12×1 | zwart | D | D | *de standaardremise van hoofdstuk 2*<br>zet 1 (wit) 1-4 geeft de winst weg (remise) (behoudend: 18-15, 1-15) |
| ✅ | 6.9 | 1. 14-12 3-6 | wit | D | D | *helemaal meteen remise*<br>zet 1 (wit) 14-12 geeft de winst weg (remise) (behoudend: 18-15, 1-15) |
| ✅ | 6.9 | 1. 18-15 16-13 2. 14-12! 13-11 3. 1-4 11-16 4. 4-7 16-13 5. 7-5! 2×7 6. 9-6 3×8 7. 12×17 | zwart | L | L0 | *diagram 6.9a, vondst van Hans Jansen* |
| ✅ | 6.9 | 1. 18-15 16-13 2. 14-12 3-6 3. 1-4 | zwart | L | L30 | *op 3-6 komt 1-4 en 6-8 gaat niet* |
| ✏️ | 6.9 | 1. 18-15 16-13 2. 1-4 16-8 3. 14-12 | – | wit wint | – | *trouwens: 2. 1-4 16-8 3. 14-12 kon ook*<br>ONMOGELIJK: zet 4 (zwart) 16-8 is niet legaal in B:WK4,9,14,15:B2,3,K13; legale zetten: 13-8, 13-6, 13-16, 3-6, 13-10, 2-5, 2-6, 13-11, 13-17 |
| ✅ | 6.9 | 1. 18-15 16-13 2. 1-4 13-8 3. 14-12 | zwart | L | L36 | *idem met 13-8 (de zwarte dam staat op 13)* |
| ✅ | 6.9 | 1. 18-15 16-13 2. 14-12 13-11 3. 1-4 11-6 4. 4-18 6-16 5. 18-7 | zwart | L | L30 |  |
| ✅ | 6.9 | 1. 18-15 16-13 2. 14-12 13-11 3. 1-4 11-13 4. 4-14 13-16 5. 14-7 | zwart | L | L30 |  |
| ✅ | 6.9 | 1. 18-15 16-13 2. 14-12 13-11 3. 1-4 11-13 4. 4-14 13-6 5. 14-18 | zwart | L | L32 |  |
| ✅ | 6.9a | 1. … 16-13 2. 7-5 2×7 3. 9-6 | zwart | L | L2 |  |
| ✅ | 6.9 | 1. … 16-13 2. 1-4 3-6 3. 4-1 6-8 4. 1×12 2-5 5. 12×1 | zwart | D | D | *zwart aan zet, remise* |
| ✅ | 6.9 | 1. … 16-13 2. 14-12 3-6 3. 1-4 13-16 4. 4-7 16-13 5. 18-15 2-5 6. 7×2 13-10! | wit | D | D | *tweede poging* |
| ✅ | 6.9 | 1. … 16-13 2. 18-15 13-16 3. 1-12 16-13 | wit | D | D | *wit komt niet verder* |
| ✅ | 6.9 | 1. … 16-13 2. 1-12 13-16?? 3. 12-15 16-13 4. 15-1 13-16 | wit | W | W41 | *diagram 6.9 met wit aan zet*<br>bevestigd: 13-16?? verliest, terecht als fout gemarkeerd (beter: 3-6) |
| ✅ | 6.9 | 1. … 16-13 2. 1-12 3-6 | wit | D | D | *maar zwart speelt 2. … 3-6, remise* |

#### Sectie 6.6

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 6.10 | 1. 18-15 3-6 | wit | D | D | *natuurlijk meteen remise*<br>zet 1 (wit) 18-15 geeft de winst weg (remise) (behoudend: 13-10) |
| ✅ | 6.10 | 1. 12-9 7-10 | wit | D | D | *gaat snel*<br>zet 1 (wit) 12-9 geeft de winst weg (remise) (behoudend: 13-10) |
| ✅ | 6.10 | 1. 18-14 3-6! 2. 13-11 6-9! 3. 12-8 7-10 4. 8-5 4-7 5. 11×4 10-13 | wit | D | D | *een aardige poging*<br>zet 1 (wit) 18-14 geeft de winst weg (remise) (behoudend: 13-10) |
| ✅ | 6.10 | 1. 13-10! 7-11 2. 18-15! 3-6 3. 12-8 11-13 4. 8×3 13-17 5. 3-11 | zwart | L | L14 | *grappige winst, diagram 6.12* |
| ✏️ | 6.10 | 1. 13-10 7-11 2. 18-15 4-7 3. 10×5 10-13 4. 12-8 13-17 5. 5-2 | – | wit wint | – | *tekst: 3. … 10-13*<br>ONMOGELIJK: zet 6 (zwart) 10-13 is niet legaal in B:W5,12,15:B3,11; legale zetten: 11-13, 3-6, 11-14 |
| ✅ | 6.10 | 1. 13-10 7-11 2. 18-15 4-7 3. 10×5 11-13 4. 12-8 13-17 5. 5-2 | zwart | L | L50 | *gecorrigeerd: 3. … 11-13 (naar diagram 6.13/6.14)* |
| ✅ | 6.13 | 6. 8-5 9-17 | wit | D | D | *gevolgd door 3-6, 17-13*<br>zet 1 (wit) 8-5 geeft de winst weg (remise) (behoudend: 2-7) |
| ✅ | 6.13 | 6. 2-7 9-6 7. 7-11 6-2 8. 11-13 2-10 9. 13-17 10-2 10. 17-9 2-7 11. 9-2 7-14 | wit | W | W35 | *diagram 6.14* |
| ✏️ | 6.13 | 6. 2-7 9-6 7. 7-11 6-2 8. 11-13 2-10 9. 13-17 10-2 10. 17-9 2-7 11. 9-2 7-14 12. 2-13 14-17 13. 8-5 3-6 14. 15-12 17×9 15. 10-17 | – | wit wint | – | *tekst: 12. 2-13*<br>ONMOGELIJK: zet 13 (wit) 2-13 is niet legaal in W:WK2,8,15:B3,K14; legale zetten: 2-10, 2-9, 2-6, 2-5, 2-7, 8-5, 8-6, 15-12 |
| ✅ | 6.13 | 6. 2-7 9-6 7. 7-11 6-2 8. 11-13 2-10 9. 13-17 10-2 10. 17-9 2-7 11. 9-2 7-14 12. 2-10 14-17 13. 8-5 3-6 14. 15-12 17×9 15. 10-17 | zwart | L | L4 | *gecorrigeerd: 12. 2-10 (diagram 6.15)* |
| ✅ | 6.13 | 6. 2-7 9-6 7. 7-11 6-2 8. 11-13 2-10 9. 13-17 10-2 10. 17-9 2-7 11. 9-2 7-14 12. 2-10 14-17 13. 8-5 3-6 14. 5-2 6-8! | wit | D | D | *na 14. 5-2 6-8! is het remise*<br>zet 17 (wit) 5-2 geeft de winst weg (remise) (behoudend: 15-12) |

#### Sectie 7.1

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 7.1 | 3. 15-12! 3-6 4. 12-8! 5×12 5. 18-15 6-8 6. 14-11 8×13 7. 15×8 | zwart | L | L10 | *uit* |
| ✅ | 7.1 | 3. 15-12 4-7 4. 12-8! 5×12 5. 18-15 | zwart | L | L20 | *uit* |
| ✅ | 7.1 | 3. 15-12 5-7 4. 12-8 | zwart | L | L20 | *uit* |
| ✅ | 7.1 | 3. 15-12 5-8 4. 12×5 1×8 5. 16-13 3-6 6. 18-15 | zwart | L | L8 | *uit* |
| ℹ️ | 7.2 | 1. 11-7 5-8 2. 12×5 1×8 3. 7-4 8-11 4. 14×7 10×5 | wit | – | D | *wit neemt "voor de zekerheid" een remisecombinatie* |

#### Sectie 7.2

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 7.1 | 3. 14-12 | zwart | D | D | *met 3. 14-12 helemaal niets*<br>zet 1 (wit) 14-12 geeft de winst weg (remise) (behoudend: 15-12, 16-13) |
| ✅ | 7.1 | 3. 14-11 5-8 4. 11×6 3×8 5. 16-13 4-7! 6. 13-11 8×13 7. 17×5 1×8 8. 18-14 9-12 | wit | D | D | *eieren voor zijn geld*<br>zet 1 (wit) 14-11 geeft de winst weg (remise) (behoudend: 15-12, 16-13) |
| ❌ | 7.1 | 3. 14-11 5-8 4. 11×6 3×8 5. 16-13 1-5 6. 17-14! | zwart | L | D | *op 5. … 1-5 verliest zwart*<br>zet 1 (wit) 14-11: geeft de winst weg (remise); behoudend waren: 15-12, 16-13<br>zet 7 (wit) 17-14: geeft de winst weg (remise); behoudend waren: 18-14<br>zet 6 (zwart) 1-5 verliest (behoudend: 4-7) |
| ❌ | 7.1 | 3. 14-11 5-8 4. 11×6 3×8 5. 16-13 4-7 6. 17-14 7-10 | wit | L | D | *tekst "9. 17-14 7-10": meteen gewonnen voor zwart*<br>zet 1 (wit) 14-11 geeft de winst weg (remise) (behoudend: 15-12, 16-13) |
| ✅ | 7.1 | 3. 14-11 5-8 4. 11×6 3×8 5. 16-13 4-7 6. 13-10 7-11 | wit | L | L14 | *tekst "9. 13-10 7-11": meteen gewonnen voor zwart*<br>zet 1 (wit) 14-11 geeft de winst weg (remise) (behoudend: 15-12, 16-13)<br>zet 7 (wit) 13-10 verliest (behoudend: 13-11, 17-14, 18-14) |
| ✅ | 7.1 | 3. 14-11 5-8 4. 11×6 3×8 5. 16-13 4-7 6. 18-14 9-12! 7. 14×9 7-11 8. 13-10 11-13 9. 10-7 13-16 10. 7-4 16-11 11. 17-14 11×18 12. 15-12 8×15 13. 9-6 18-11 14. 6-2 | zwart | D | D | *tekst "9. 18-14 ...": wit haalt nog net remise*<br>zet 1 (wit) 14-11 geeft de winst weg (remise) (behoudend: 15-12, 16-13) |
| ❌ | 7.3 | 3. … 5-8[enige] | wit | D | W51 | *de enige weg naar remise*<br>zet 1 5-8 (L52) "de enige": ook 3-6 (L22), 5-7 (L22), 4-7 (L18), 9-12 (L18) behoudt/behouden het resultaat |
| ✅ | 7.3 | 3. … 5-7 4. 13-11 3-6 5. 15-12 1-5 6. 12-8 5×12 7. 18-15 6-8 8. 11×6 9×2 9. 15×8 | zwart | L | L10 |  |
| ✅ | 7.3 | 3. … 3-6 4. 13-11 6-8 | wit | D | D | *13-11 6-8 =*<br>zet 2 (wit) 13-11 geeft de winst weg (remise) (behoudend: 13-10) |
| ✏️ | 7.3 | 3. … 3-6 4. 13-10!! 5-8 5. 10-7 4×11 6. 14×10 | – | wit wint | – | *tekst: 6. 14x10*<br>ONMOGELIJK: zet 6 (wit) 14×10 is niet legaal in W:W14,15,17,18:B1,6,8,9,11; legale zetten: 14x7 |
| ✅ | 7.3 | 3. … 3-6 4. 13-10 5-8 5. 10-7 4×11 6. 14×7 | zwart | L | L14 | *gecorrigeerd: 6. 14x7* |
| ✅ | 7.3 | 3. … 3-6 4. 13-10!! 6-8!! | wit | W | W19 | *diagram 7.4 (kerstpuzzel)* |
| ✅ | 7.4j | 5. 14-12 9×14 6. 18×6 5-7 | wit | D | D | zet 1 (wit) 14-12 geeft de winst weg (remise) (behoudend: 14-11) |
| ✅ | 7.4j | 5. 10-7?? 5×10 6. 14-12 9×14 7. 18×6 10-13! 8. 17×10 4-7 9. 10×5 1×3 | wit | L | L6 | *zwart wint zelfs*<br>bevestigd: 10-7?? verliest zelfs, terecht als fout gemarkeerd (beter: 14-11) |
| ✏️ | 7.4j | 5. 14-11 8×13 6. 15-12 9×14 7. 18×16 5-8 8. 17-13 1-5 9. 13-11 8×13 10. 16×13 | – | wit wint | – | ONMOGELIJK: zet 11 (wit) 16×13 is niet legaal in W:W10,16:B4,5,13; legale zetten: 16x11, 10x17 |
| ✏️ | 7.4j | 5. 14-11 8×13 6. 15-12 9×14 7. 18×16 4-7 8. 17-14 1-5 9. 16-13 | – | wit wint | – | ONMOGELIJK: zet 8 (zwart) 1-5 is niet legaal in B:W10,14,16:B1,5,7; legale zetten: 1-4, 5-8, 7-11 |
| ✅ | 7.4j | 5. 14-11 8×13 6. 15-12 9×14 7. 18×16 5-7 8. 10×5 1×8 9. 16-13! 8-12 10. 13-11 12-15 11. 17-13!! | zwart | L | L6 | *tekst: "dus 8. 5-7 10x5 9. 1x8 16-13! 10. 8-12 13-11 11. 12-15 17-13!!"* |

#### Sectie 7.3

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✏️ | 7.1 | 3. 16-11 5-8 | – | – | – | *tekst: "3. 16-11 5-8" (bedoeld 16-13)*<br>ONMOGELIJK: zet 1 (wit) 16-11 is niet legaal in W:W14,15,16,17,18:B1,3,4,5,9; legale zetten: 15-12, 16-13, 14-11, 14-12, 17-13 |
| ✅ | 7.4 | 4. 14-12 | zwart | D | D | *loopt het snel remise*<br>zet 1 (wit) 14-12 geeft de winst weg (remise) (behoudend: 14-11) |
| ❌ | 7.4 | 4. 13-11 8×13 5. 17×10 4-7 6. 10×5 1×8 | wit | L | D | *verliest wit zelfs*<br>zet 1 (wit) 13-11 geeft de winst weg (remise) (behoudend: 14-11) |
| ✅ | 7.5 | 4. … 3-6 5. 10-7 | zwart | L | L16 | *meteen uit*<br>zet 1 (zwart) 3-6 verliest (behoudend: 1-5, 8-11) |
| ✅ | 7.5 | 4. … 1-5 5. 17-13 3-6 6. 13-11 8×13 7. 10×17 5-7 8. 17-13 7-10 9. 14-11 10×17 10. 18-14 17×12 11. 15×3 | zwart | L | L16 | *diagram 7.6*<br>zet 3 (zwart) 3-6 verliest (behoudend: 9-12) |
| ℹ️ | 7.5 | 4. … 8-11 5. 14×7 4×11 6. 15-12[enige] 9×14 7. 17×12 | zwart | – | D | *het enige is 6. 15-12*<br>bevestigd: 15-12 is inderdaad de enige behoudende zet |
| ✅ | 7.7 | 6. 15-12[enige] | zwart | D | D | *het enige is 6. 15-12, en dat is meteen heel goed*<br>bevestigd: 15-12 is inderdaad de enige behoudende zet |
| ✅ | 7.8 | 7. … 3-6![enige] | wit | D | D | *de enige remise*<br>bevestigd: 3-6 is inderdaad de enige behoudende zet |
| ✅ | 7.8 | 7. … 1-4 8. 18-15 3-6 9. 12-8 11-14 10. 8×3 14-17 11. 3-11 | zwart | L | L14 | zet 1 (zwart) 1-4 verliest (behoudend: 3-6) |
| ✅ | 7.8 | 7. … 1-4 8. 18-15 4-7 | wit | W | W55 | *de taaie winst*<br>zet 1 (zwart) 1-4 verliest (behoudend: 3-6) |
| ✅ | 7.8 | 7. … 1-5 8. 18-15 5-7 | wit | W | W55 | *de taaie winst*<br>zet 1 (zwart) 1-5 verliest (behoudend: 3-6) |
| ✅ | 7.8 | 7. … 1-5 8. 18-15 3-6 9. 12-8 5×12 10. 15×3 11-14 11. 3-11 | zwart | L | L2 | zet 1 (zwart) 1-5 verliest (behoudend: 3-6) |
| ✅ | 7.8 | 7. … 3-6 8. 12-8 11-13 9. 8×3 13-17 | wit | D | D | *wit komt niet verder* |
| ✅ | 7.9 | 4. … 9-12 5. 11×6 3×8 6. 13-11 8×13 7. 17×10 | zwart | L | L8 |  |
| ✅ | 7.9 | 4. … 3-6 5. 17-14 1-5 6. 13-10 8×13 7. 10×17 5-7 8. 17-13 7-10 9. 14-11 10×17 10. 18-14 17×12 11. 15×3 | zwart | L | L16 |  |

#### Sectie 7.4

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 7.10 | 6. 18-14 9-12 7. 14×9 4-7 8. 13-11 8×13 9. 17×10 7-11 10. 15-12 | zwart | L | L26 |  |
| ✅ | 7.10 | 6. 18-14 4-7 7. 13-11 8×13 8. 17×10 | zwart | L | L4 |  |
| ✅ | 7.10 | 6. 18-14 9-12 7. 14×9 4-7 8. 17-14 | zwart | D | D | *17-14 is remise*<br>zet 5 (wit) 17-14 geeft de winst weg (remise) (behoudend: 13-11) |
| ❌ | 7.10 | 6. 17-14 9-12 7. 14×9 5-7 | wit | D | W13 | *diagram 7.13*<br>zet 4 (zwart) 5-7: verliest; behoudend waren: 4-7<br>zet 1 (wit) 17-14 geeft de winst weg (remise) (behoudend: 18-14) |
| ✅ | 7.10 | 6. 17-14 9-12 7. 14×9 4-7 | wit | D | D | *diagram 7.14*<br>zet 1 (wit) 17-14 geeft de winst weg (remise) (behoudend: 18-14) |
| ✅ | 7.13b | 1. 13-11 | zwart | L | L4 |  |
| ✅ | 7.14b | 1. 13-11 | zwart | L | L30 |  |
| ✅ | 7.13 | 8. 13-10 7-11 9. 10-7?? 8-12 | wit | L | L12 | bevestigd: 10-7?? verliest, terecht als fout gemarkeerd (beter: 9-6, 15-12)<br>zet 1 (wit) 13-10 geeft de winst weg (remise) (behoudend: 9-6) |
| ✅ | 7.13 | 8. 15-12 8×15 9. 9-6 7-10 10. 13-11 4-7 | wit | D | D | zet 1 (wit) 15-12 geeft de winst weg (remise) (behoudend: 9-6) |
| ✅ | 7.13 | 8. 15-12 8×15 9. 13-10 7-11 10. 9-6 4-7 | wit | D | D | zet 1 (wit) 15-12 geeft de winst weg (remise) (behoudend: 9-6) |
| ✅ | 7.13 | 8. 9-6 8×3 9. 13-10 7-11 10. 15-12 3-6 11. 12-8 11-13 12. 8×3 13-17 | wit | D | D | zet 3 (wit) 13-10 geeft de winst weg (remise) (behoudend: 13-11) |
| ⚠️ | 7.13 | 8. 9-6 8×3 9. 13-10 7-11 10. 15-12 4-7 | wit | W | W39 | *4-7 verliest*<br>zet 3 (wit) 13-10: geeft de winst weg (remise); behoudend waren: 13-11<br>zet 6 (zwart) 4-7 verliest (behoudend: 3-6) |
| ℹ️ | 7.14 | 8. 18-14 7-10 9. 14-11 10×17 10. 11×6 17-14!! 11. 6-3 14-18 12. 3-13 18-4 | wit | – | D | *diagram 7.15* |
| ❌ | 7.15 | 13. 15-12 4-1 | wit | W | D | *puzzel 1: hoe wint wit na 13. 15-12 4-1?* |
| ✅ | 7.15 | 13. 15-12 | zwart | D | D | *puzzel 2: hoe maakt zwart remise na 13. 15-12?* |

#### Sectie 7.5

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 7.16 | 6. 13-11 5-8 7. 11×6 3×8 8. 18-14 8-11 | wit | L | L10 | zet 1 (wit) 13-11 verliest zelfs (behoudend: 15-12) |
| ✅ | 7.16 | 6. 13-11 5-8 7. 11×6 3×8 8. 17-14 8-11 | wit | L | L12 | zet 1 (wit) 13-11 verliest zelfs (behoudend: 15-12) |
| ✅ | 7.16 | 6. 13-11 5-8 7. 11×6 3×8 8. 17-13 4-7 | wit | L | L14 | zet 1 (wit) 13-11 verliest zelfs (behoudend: 15-12) |
| ✏️ | 7.16 | 6. 13-10 5-8 7. 17-14 8-11 8. 14×7 5×11 9. 15-12 3-6 10. 12-8 11-13 11. 8×3 13-16 | – | remise | – | *tekst: 8. 14x7 5x11*<br>ONMOGELIJK: zet 6 (zwart) 5×11 is niet legaal in B:W7,10,15,18:B2,3,4; legale zetten: 4x11<br>zet 1 (wit) 13-10 geeft de winst weg (remise) (behoudend: 15-12) |
| ✅ | 7.16 | 6. 13-10 5-8 7. 17-14 8-11 8. 14×7 4×11 9. 15-12 3-6 10. 12-8 11-13 11. 8×3 13-16 | wit | D | D | *gecorrigeerd: 8. … 4x11*<br>zet 1 (wit) 13-10 geeft de winst weg (remise) (behoudend: 15-12) |
| ✏️ | 7.16 | 6. 13-10 5-8 7. 17-14 8-11 8. 14×7 4×11 9. 15-12 2-7 10. 18-15 | – | wit wint | – | *(* 9. … 2-7 10. 18-15 W+), met 8. … 4x11*<br>zet 1 (wit) 13-10: geeft de winst weg (remise); behoudend waren: 15-12<br>ONMOGELIJK: zet 8 (zwart) 2-7 is niet legaal in B:W10,12,18:B2,3,11; legale zetten: 3-6, 2-5, 2-6, 11-13, 11-14 |
| ⚠️ | 7.16 | 6. 13-10 5-8 7. 17-14 8-11 8. 14×7 4×11 9. 15-12 2-6 10. 12-9 6-8 11. 9-6 8-12 12. 6-2 | zwart | L | L14 | *(* 9. … 2-6 ... W+), met 8. … 4x11*<br>zet 1 (wit) 13-10: geeft de winst weg (remise); behoudend waren: 15-12<br>zet 8 (zwart) 2-6 verliest (behoudend: 3-6) |
| ✅ | 7.16 | 6. 17-14 5-8 7. 14-11 2-5 | wit | D | D | zet 1 (wit) 17-14 geeft de winst weg (remise) (behoudend: 15-12) |
| ✅ | 7.16 | 6. 17-14 5-8 7. 14-12 3-6 | wit | D | D | zet 1 (wit) 17-14 geeft de winst weg (remise) (behoudend: 15-12) |
| ✅ | 7.16 | 6. 17-14 5-8 7. 13-10 8-11 | wit | D | D | zet 1 (wit) 17-14 geeft de winst weg (remise) (behoudend: 15-12) |
| ✅ | 7.16 | 6. 18-14 5-8 | wit | D | D | zet 1 (wit) 18-14 geeft de winst weg (remise) (behoudend: 15-12) |
| ✅ | 7.16 | 6. 15-12 | zwart | L | L46 | *de beste zet* |
| ✅ | 7.17 | 6. … 3-6 7. 13-11!! | zwart | L | L12 |  |
| ✅ | 7.17 | 6. … 2-6 7. 12-9 6-8 8. 9-6 | zwart | L | L16 |  |
| ✅ | 7.17 | 6. … 4-7 7. 18-14 7-10 8. 13-11 3-6 9. 12-8 5×12 10. 14×9 | zwart | L | L8 |  |
| ✅ | 7.17 | 6. … 4-7 7. 18-14 3-6 8. 13-11 6-8 9. 11×4 8×15 10. 14-12 15×8 11. 4-1 | zwart | L | L12 |  |
| ✅ | 7.17 | 6. … 4-7 7. 18-14 2-6 8. 12-9 6-8 9. 13-11 8×13 10. 17×10 | zwart | L | L8 |  |
| ✏️ | 7.17 | 6. … 5-8 7. 12×5 2×10 8. 18-14 3-6 9. 13-11 | – | wit wint | – | *tekst: 7. … 2x10*<br>ONMOGELIJK: zet 3 (zwart) 2×10 is niet legaal in B:W5,13,17,18:B2,3,4; legale zetten: 2x7 |
| ✅ | 7.17 | 6. … 5-8 7. 12×5 2×7 8. 18-14 3-6 9. 13-11 | zwart | L | L12 | *gecorrigeerd: 7. … 2x7* |

#### Sectie 7.6

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✏️ | 7.18 | 7. 13-10 7-11 8. 17-14 4-7 9. 10×5 2×10 10. 12-9 10-13 11. 14×7 10×5 12. 18-15 | – | remise | – | ONMOGELIJK: zet 6 (zwart) 2×10 is niet legaal in B:W5,12,14,18:B2,3,11; legale zetten: 2x7<br>zet 1 (wit) 13-10 geeft de winst weg (remise) (behoudend: 17-14) |
| ✏️ | 7.18 | 7. 12-9 2-7 8. 13-10 7-11 9. 17-14 4-7 10. 14-12 5-8 11. 10×5 8×15 | – | remise | – | *tekst: 7. 12-9 2-7*<br>ONMOGELIJK: zet 2 (zwart) 2-7 is niet legaal in B:W9,13,17,18:B2,3,4,7; legale zetten: 2-5, 7-10, 3-6, 7-11, 2-6<br>zet 1 (wit) 12-9 geeft de winst weg (remise) (behoudend: 17-14) |
| ✅ | 7.19 | 7. … 2-5 8. 12-8 | zwart | L | L12 | zet 1 (zwart) 2-5 verliest (behoudend: 2-6) |
| ✅ | 7.19 | 7. … 7-10 8. 12-8 | zwart | L | L14 | zet 1 (zwart) 7-10 verliest (behoudend: 2-6) |
| ✅ | 7.19 | 7. … 3-6 8. 13-11 | zwart | L | L16 | zet 1 (zwart) 3-6 verliest (behoudend: 2-6) |
| ✅ | 7.19 | 7. … 2-6!! 8. 13-11 7-10!! | wit | D | D | *een dijk van een stand* |
| ✅ | 7.21 | 7. … 7-10 8. 12-8 | zwart | L | L18 | zet 1 (zwart) 7-10 verliest (behoudend: 2-5, 3-6) |
| ✅ | 7.21 | 7. … 2-6 | wit | W | W39 | *wit wint, zie diagram 6.7*<br>zet 1 (zwart) 2-6 verliest (behoudend: 2-5, 3-6) |
| ✅ | 7.21 | 7. … 3-6 | wit | D | D |  |
| ✅ | 7.21 | 7. … 2-5 | wit | D | D |  |
| ✅ | 7.21 | 7. … 3-6 8. 17-14 2-5 9. 13-10 6-9 10. 14-11 7×14 11. 12×17 5-8 12. 17-13 4-7 | wit | D | D |  |

#### Sectie 7.7

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 7.22 | 7. … 2-6 8. 13-10! 6-8 9. 10×5 8×1 10. 12-8 | zwart | L | L14 |  |
| ✅ | 7.22 | 7. … 2-6 8. 13-10 7-11 9. 14×7 4×11 10. 12-9 6-8 11. 9-6 | zwart | L | L16 |  |
| ✅ | 7.22 | 7. … 3-6 8. 13-11 2-5 9. 12-9 | zwart | L | L16 |  |
| ✅ | 7.22 | 7. … 3-6 8. 13-11 6-9 9. 12-8 7-10 10. 8-6 4-7 11. 11×4 10-13 12. 6-3 | zwart | L | L10 |  |
| ✅ | 7.22 | 7. … 3-6 8. 13-11 7-10 9. 12-8 6-9 | wit | W | W17 |  |
| ✅ | 7.22 | 7. … 2-5 | wit | W | W21 | *wit kan op twee mooie manieren winnen* |
| ✅ | 7.22 | 7. … 7-10 8. 13-11 4-7 9. 11×4 10-13 10. 4-1 13-16 11. 12-9 16-13 12. 14-12 3-6 13. 1-4 13-16 14. 4-7 16-13 15. 18-15 2-5!! 16. 7×2 13-10 | wit | D | D | *de hoofdvariant, diagram 7.23*<br>zet 6 (wit) 4-1 geeft de winst weg (remise) (behoudend: 12-8) |
| ✅ | 7.22 | 7. … 7-10 8. 13-11 4-7 9. 11×4 10-13 10. 12-8! 13-16 11. 14-11 2-5 12. 8×1 16×6 13. 18-14 | zwart | L | L32 | *database: 10. 12-8! in plaats van 10. 4-1* |
| ❌ | 7.1 | 3. 16-13 5-8 4. 14-11 1-5 5. 11×6 9×2 6. 15-12 5-7 7. 17-14 7-10 | wit | D | W43 | *kansrijkste spel voor wit, beste verdediging voor zwart* |
| ✏️ | 7.1 | 3. 16-13 5-7 4. 14-11 1-5 | – | – | – | *tekst sectie 7.7: "3. … 5-7 4. 14-11 1-5"*<br>ONMOGELIJK: zet 4 (zwart) 1-5 is niet legaal in B:W11,13,15,17,18:B1,3,4,7,9; legale zetten: 7x14 |

#### Sectie 8

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ⚠️ | 7.1 | 3. 16-13 5-8 4. 14-11 1-5 5. 11×6 3×8 6. 17-14 9-12 7. 14×9 5-7 8. 9-6 8×3 9. 13-10 7-11 10. 15-12 3-6 11. 12-8 11-13 12. 8×3 13-17 | wit | D | D | *diagram 8.1*<br>zet 10 (zwart) 5-7: verliest; behoudend waren: 4-7<br>zet 7 (wit) 17-14 geeft de winst weg (remise) (behoudend: 18-14)<br>zet 13 (wit) 13-10 geeft de winst weg (remise) (behoudend: 13-11) |
| ✅ | 8.1 | 1. 18-15 17-9 2. 3-8 9-12 | wit | D | D | *wit komt niet verder* |
| ✅ | 8.1 | 1. 18-15 17-9 2. 3-8 9-17? 3. 8-11 | zwart | L | L14 | *9-17? 8-11 W+*<br>bevestigd: 9-17? verliest, terecht als fout gemarkeerd (beter: 9-12) |
| ✏️ | 8.1 | 1. 3-8 17-9 2. 8-16 9-2 3. 3-11 | – | – | – | *tekst: 3. 3-11*<br>ONMOGELIJK: zet 5 (wit) 3-11 is niet legaal in W:W10,K16,18:BK2,4; legale zetten: 10-7, 16-11, 16-8, 16-3, 18-14, 18-15, 16-13, 16-6 |
| ✅ | 8.1 | 1. 3-8 17-9 2. 8-16 9-2 3. 16-11 2-9 4. 10-7 9-17 5. 7-5 17-10! 6. 5-2 4-7 7. 11×4 10-13 | wit | D | D | *gecorrigeerd: 3. 16-11* |
| ✅ | 8.1 | 1. 3-8 17-9 2. 8-16 9-17 | wit | D | D | *9-17 is ook remise* |
| ✅ | 8.2 | 1. … 16-8 2. 14-11 8×16 3. 10-7 4×11 4. 17-13 11-14 5. 18×11 | zwart | L | L0 | *Manoury* |
| ✅ | 8.2 | 1. … 16-6 2. 14-11 6×16 3. 10-7 4×11 4. 17-13 11-14 5. 18×11 | zwart | L | L0 | *(of 6)* |
| ✅ | 8.2 | 1. … 16-3 2. 14-11 3×16 3. 10-7 4×11 4. 17-13 11-14 5. 18×11 | zwart | L | L0 | *(of 3)* |
| ✅ | 8.3 | 1. … 16-3 2. 10-7 4×11 3. 9-6 | zwart | L | L4 |  |
| ✅ | 8.3 | 1. … 16-11 2. 17-14 | zwart | L | L18 | *16-11 17-14, 9-6* |
| ✅ | 8.3 | 1. 15-1 | zwart | L | L22 | *wit aan zet speelt 15-1 met winst* |
| ✅ | 8.4b | 1. 10-7 4×11 2. 17-13 | zwart | L | L2 | *Manoury* |
| ✅ | 8.5 | 1. 2-7 4×11 2. 9-6 | zwart | L | L4 |  |
| ✅ | 8.5 | 1. … 3-11 2. 17-14 | zwart | L | L18 |  |
| ✅ | 8.5 | 1. … 3-16 2. 2-5 | zwart | L | L20 |  |
| ✅ | 8.6 | 1. … 3-8 2. 17-12 | zwart | L | L24 |  |
| ✅ | 8.6 | 1. … 3-11 2. 18-14 | zwart | L | L20 |  |
| ✅ | 8.6 | 1. … 3-16 2. 2-7 4×11 3. 17-13 | zwart | L | L2 |  |
| ✅ | 8.6 | 1. 2-5 | zwart | L | L22 | *als wit aan zet is speelt hij 2-5* |

#### Sectie 9.1

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✅ | 9.1 | 1. 2-10? 9-6 | wit | ≤D | D | *zwart verovert de lange lijn*<br>bevestigd: 2-10? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 2-7) |
| ✏️ | 9.1 | 1. 8-5? 9-17 2. 5-2 3-6 | – | notwit wint | – | *tekst: 9-17 8-2 3-6, 17-13*<br>ONMOGELIJK: zet 3 (wit) 5-2 is niet legaal in W:WK2,5,15:B3,K17; legale zetten: 2-9, 5-1, 15-12, 2-6<br>bevestigd: 8-5? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 2-7) |
| ✅ | 9.1 | 1. 8-5? 9-17 | wit | ≤D | D | *zwart verovert de lange lijn*<br>bevestigd: 8-5? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 2-7) |
| ✅ | 9.1 | 1. 2-7 9-6 2. 7-11 6-9? 3. 8-5 | zwart | L | L32 | *wint snel*<br>bevestigd: 6-9?: verliest sneller (L34 i.p.v. L46) |
| ✅ | 9.1 | 1. 2-7 9-2? 2. 7-11 | zwart | L | L34 | *diagram 9.2*<br>bevestigd: 9-2?: verliest sneller (L36 i.p.v. L48) |
| ✅ | 9.1 | 1. 2-7 9-17? 2. 7-11 | zwart | L | L34 | *diagram 9.3*<br>bevestigd: 9-17?: verliest sneller (L36 i.p.v. L48) |
| ✅ | 9.4 | 1. 10-13 9-2 2. 13-11 | zwart | L | L34 |  |
| ✅ | 9.4 | 1. 10-13 9-17 2. 13-11 | zwart | L | L34 |  |
| ✅ | 9.5 | 1. 9-14 10-2 2. 14-11 | zwart | L | L34 |  |
| ✅ | 9.5 | 1. 9-14 10-17 2. 14-11 | zwart | L | L34 |  |
| ℹ️ | 9.6 | 1. 2-9 17-13 | wit | – | W5 | *tekst: "2-9 17-13 met 9.5"* |
| ✅ | 9.6 | 1. 2-9 17-10 | wit | W | W37 | *naar diagram 9.5* |
| ✅ | 9.1 | 1. 2-7 9-6 2. 7-11 6-2 3. 11-16? 2-7 4. 15-12 7-4 | wit | D | D | *er is niets te doen tegen 4-1*<br>bevestigd: 11-16? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 11-13, 11-4, 11-14, 11-18) |
| ✅ | 9.1 | 1. 2-7 9-6 2. 7-11 6-2 3. 11-13 2-7? 4. 15-12 7-18 5. 13-10 18-15 6. 10-2 | zwart | L | L30 | *diagram 9.7*<br>bevestigd: 2-7?: verliest sneller (L36 i.p.v. L44) |
| ✅ | 9.7 | 6. … 15-18 7. 2-5 18-15 8. 12-9 15-18 9. 5-1 18-7 10. 1-4 7-10 11. 4-18 10-2 12. 18-14 2-10 13. 9-6 10-2 14. 14-9 2-7 15. 6-2 | zwart | L | L6 | *routinematig* |
| ✏️ | 9.7 | 1. 2-9 18-15 | – | – | – | *tekst bij 9.7 met wit aan zet*<br>ONMOGELIJK: zet 2 (zwart) 18-15 is niet legaal in B:W8,K9,12:B3,K15; legale zetten: 15-18, 3-6 |

#### Sectie 9.2

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✏️ | 7.1 | 3. 16-13 5-8 4. 13-10 8-11 5. 14×7 4×11 6. 15-12 9×14 7. 17×12 1-4 8. 18-15 4-7 9. 10×5 10-13 | – | – | – | *tekst: 9. … 10-13*<br>ONMOGELIJK: zet 14 (zwart) 10-13 is niet legaal in B:W5,12,15:B3,11; legale zetten: 11-13, 3-6, 11-14<br>zet 3 (wit) 13-10 geeft de winst weg (remise) (behoudend: 14-11)<br>zet 10 (zwart) 1-4 verliest (behoudend: 3-6) |
| ⚠️ | 7.1 | 3. 16-13 5-8 4. 13-10 8-11 5. 14×7 4×11 6. 15-12 9×14 7. 17×12 1-4 8. 18-15 4-7 9. 10×5 11-13 10. 12-8 13-17 11. 5-2 17-9 | wit | W | W49 | *gecorrigeerd: 9. … 11-13, diagram 9.1*<br>zet 3 (wit) 13-10: geeft de winst weg (remise); behoudend waren: 14-11<br>zet 10 (zwart) 1-4 verliest (behoudend: 3-6) |
| ✅ | 9.1 | 12. 2-5 9-6 | wit | D | D | zet 1 (wit) 2-5 geeft de winst weg (remise) (behoudend: 2-7) |
| ✅ | 9.1 | 12. 2-10 9-6 | wit | D | D | zet 1 (wit) 2-10 geeft de winst weg (remise) (behoudend: 2-7) |
| ✏️ | 9.1 | 12. 8-5 9-17 13. 5-2 3-6 | – | remise | – | ONMOGELIJK: zet 3 (wit) 5-2 is niet legaal in W:WK2,5,15:B3,K17; legale zetten: 2-9, 5-1, 15-12, 2-6<br>zet 1 (wit) 8-5 geeft de winst weg (remise) (behoudend: 2-7) |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-9 14. 8-5 | zwart | L | L32 |  |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-16 2-7 15. 15-12 7-4 | wit | D | D | zet 5 (wit) 11-16 geeft de winst weg (remise) (behoudend: 11-13, 11-4, 11-14, 11-18) |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-13 2-7 15. 15-12 7-18 16. 13-10 | zwart | L | L32 |  |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-13 2-7 15. 15-12 7-4 16. 13-10 | zwart | L | L32 |  |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-13 2-10 15. 13-16 10-7 | wit | D | D | zet 7 (wit) 13-16 geeft de winst weg (remise) (behoudend: 13-17, 13-11) |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-13 2-10 15. 13-17 10-7 16. 15-12 7-4 17. 17-10 | zwart | L | L32 |  |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-13 2-10 15. 13-17 10-7 16. 15-12 7-18 17. 17-10 | zwart | L | L32 |  |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-13 2-10 15. 13-17 10-2 16. 17-10 2-6 | wit | D | D | zet 9 (wit) 17-10 geeft de winst weg (remise) (behoudend: 17-9, 17-13, 17-14) |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-13 2-10 15. 13-17 10-2 16. 17-12 2-6 | wit | D | D | zet 9 (wit) 17-12 geeft de winst weg (remise) (behoudend: 17-9, 17-13, 17-14) |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-13 2-10 15. 13-17 10-2 16. 15-12 2-6 | wit | D | D | zet 9 (wit) 15-12 geeft de winst weg (remise) (behoudend: 17-9, 17-13, 17-14) |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-13 2-10 15. 13-17 10-2 16. 17-9 2-10 17. 9-14 | zwart | L | L36 |  |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-13 2-10 15. 13-17 10-2 16. 17-9 2-7 17. 9-2 | zwart | L | L38 | *diagram 9.8* |

#### Sectie 9.3

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ⚠️ | 9.8 | 17. … 7-14 18. 2-10[enige] | zwart | L | L34 | *de enige winnende zet*<br>zet 2 2-10 (W35) "de enige": ook 2-9 (W41), 2-6 (W43), 2-5 (W45) behoudt/behouden het resultaat |
| ✅ | 9.8 | 17. … 7-18 18. 15-12? 18-15 | wit | D | D | *verbazend genoeg remise*<br>bevestigd: 15-12? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 2-5, 2-9, 2-6, 2-10) |
| ✅ | 9.8 | 17. … 7-18 18. 2-5 18-14 19. 5-10 | zwart | L | L34 | *de snelste winst* |
| ✅ | 9.8 | 17. … 7-18 18. 2-5 18-4 19. 15-12 | zwart | L | L28 | *op 18-4 wint 15-12 wel* |
| ✅ | 9.8 | 17. … 7-18 18. 2-10 18-14 19. 10-5 14-17 20. 5-7 17-9 21. 7-2 9-14 22. 2-10 | zwart | L | L34 | *ook 18. 2-10 is winnend* |
| ✅ | 9.8 | 17. … 7-4 18. 15-12? 4-1 | wit | D | D | bevestigd: 15-12? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 2-5, 2-9, 2-6, 2-10) |
| ✅ | 9.8 | 17. … 7-4 18. 2-5 4-14 19. 5-10 | zwart | L | L34 |  |
| ✅ | 9.8 | 17. … 7-4 18. 2-5 4-18 19. 15-12 | zwart | L | L28 |  |
| ✅ | 9.8 | 17. … 7-4 18. 2-10 | zwart | L | L44 | *ook 18. 2-10 is winnend* |

#### Sectie 9.4

| | vanaf | variant | aan zet | boek | database | toelichting |
|---|---|---|---|---|---|---|
| ✏️ | 9.12 | 19. … 14-17 20. 8-5 3-6! 21. 15-12! 17×9 22. 13-17 | – | – | – | *tekst: 22. 13-17*<br>ONMOGELIJK: zet 6 (wit) 13-17 is niet legaal in W:W5,K10:B6,K9; legale zetten: 10-17, 5-1, 5-2, 10-13, 10-7 |
| ✅ | 9.12 | 19. … 14-17 20. 8-5 3-6 21. 15-12 17×9 22. 10-17 | zwart | L | L4 | *gecorrigeerd: 22. 10-17 (slotstand)* |
| ✅ | 9.12 | 19. … 14-4 20. 15-12 | zwart | L | L32 |  |
| ✅ | 9.12 | 19. … 14-18 20. 15-12 | zwart | L | L32 |  |
| ✅ | 9.12 | 19. … 14-9 20. 10-13 | zwart | L | L32 |  |
| ✅ | 9.12 | 19. … 14-12 20. 10-13 | zwart | L | L6 |  |
| ✅ | 9.12 | 19. … 14-12 20. 8-6 | zwart | L | L8 |  |
| ⚠️ | 9.12 | 19. … 14-17 20. 8-5[enige] | zwart | L | L30 | *wit moet wel verder*<br>zet 2 8-5 (W31) "de enige": ook 10-7 (W43), 10-5 (W43), 10-2 (W51) behoudt/behouden het resultaat |
| ✅ | 9.12 | 19. … 14-17 20. 8-5 3-6 21. 5-2? 6-8 | wit | D | D | bevestigd: 5-2? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 15-12) |
| ✅ | 9.12 | 19. … 14-17 20. 8-5 3-6 21. 15-12[enige] | zwart | L | L6 | bevestigd: 15-12 is inderdaad de enige behoudende zet |

Database-statistiek: `built 10 dtm 99 slices 25029665 dtm bytes 7342710904 wdl bytes 0 extra slices 0 positions 0 tt 0 nodes 0`
