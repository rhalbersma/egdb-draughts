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

1. 13-11 6-8 2. 11×6 2×9, wit aan zet; wit wint:

```
     a b c d e f
 6     ●   ·   ●       1  2  3
 5   ●   ●   ·        4  5  6
 4     ·   ·   ●       7  8  9
 3   ·   ·   ·       10 11 12
 2     ·   ○   ○      13 14 15
 1   ○   ○   ○       16 17 18
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

```
     a b c d e f
 6     ·   ●   ●       1  2  3
 5   ○   ·   ·        4  5  6
 4     ·   ·   ·       7  8  9
 3   ·   ·   ○       10 11 12
 2     ●   ○   ·      13 14 15
 1   ·   ·   ○       16 17 18
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

```
     a b c d e f
 6     ○   ·   ●       1  2  3
 5   ○   ·   ●        4  5  6
 4     ·   ·   ·       7  8  9
 3   ·   ·   ·       10 11 12
 2     ·   ·   ·      13 14 15
 1   ·   ·   ○       16 17 18
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
  offsettabel. Voor de opening is daarnaast een database tot en met 10 stukken gebouwd (25,7 miljard
  stellingen, 25 miljoen slices, 1 u 52 min): DTM tot en met 8 stukken en, in Syzygy-stijl, alleen
  winst/remise/verlies in 2 bits per stelling voor 9 en 10 stukken (samen 7,8 GB).
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
  alle 1,725 miljard stellingen tot en met 8 stukken geeft dat 0 fouten.
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

Tussen haakjes: de afstand tot het einde van de partij in ply (DTM) bij perfect spel.

| diagram | stelling | boek, wit aan zet | database | boek, zwart aan zet | database | behoudende zetten (wit aan zet) |
|---|---|---|---|---|---|---|
| 1.1 | `WK16,K17,K18:BK2,15` | wit wint | wit wint (13) ✅ | – | wit wint (12)  | 16-11 (13), 16-8 (13), 16-3 (13), 17-9 (13), 16-13 (17), 17-13 (17), … |
| 1.2 | `WK1,K5,K12:B3,K16` | wit wint | wit wint (13) ✅ | – | remise  | 12-14 (13) |
| 2.1 | `WK4,9,12,17:BK16` | wit wint | wit wint (9) ✅ | – | wit wint (16)  | 4-1 (9), 4-7 (13), 4-14 (13), 4-18 (13) |
| 2.1b | `WK4,9,12,17:BK3` | wit wint | wit wint (15) ✅ | – | wit wint (10)  | 4-7 (15), 4-14 (15), 4-18 (15), 4-1 (19) |
| 2.2 | `W4,10,18:B3,K6` | wit wint | wit wint (45) ✅ | – | remise  | 4-1 (45) |
| 2.3 | `WK2,K17,K18:BK8,15` | wit wint | wit wint (11) ✅ | – | remise  | 17-12 (11) |
| 3.3 | `W10,14,K15:B3,K16` | wit wint | wit wint (33) ✅ | – | wit wint (40)  | 10-7 (33), 15-1 (37) |
| 3.4 | `W10,14,K15:B3,K6` | – | wit wint (39)  | – | wit wint (34)  | 15-5 (39), 15-1 (43) |
| 3.5 | `WK8,9,K18:BK2,3` | wit wint | wit wint (11) ✅ | – | wit wint (16)  | 8-12 (11), 8-1 (15), 8-15 (15), 8-13 (17), 8-11 (19), 8-16 (19), … |
| 3.5b | `WK8,K9,K18:BK2,3` | wit wint | wit wint (11) ✅ | – | wit wint (12)  | 8-12 (11), 8-13 (13), 18-4 (13), 8-1 (15), 8-15 (15), 9-17 (15), … |
| 3.5c | `WK8,10,14:B3,K9` | wit wint | wit wint (31) ✅ | – | remise  | 14-11 (31) |
| 4.1a | `WK16,K17,K18:BK9,15` | wit wint | wit wint (11) ✅ | – | wit wint (14)  | 16-13 (11), 16-11 (15), 16-8 (15), 16-3 (15), 17-13 (19), 17-10 (19) |
| 4.1b | `WK16,K17,K18:BK10,15` | wit wint | wit wint (9) ✅ | – | wit wint (14)  | 16-6 (9), 16-13 (11), 17-9 (13), 16-11 (15), 16-8 (15), 16-3 (15), … |
| 4.1c | `WK16,K17,18:BK2,15` | remise | remise ✅ | – | remise  | 16-13, 16-11, 16-8, 16-6, 16-3, 17-13, … |
| 4.1d | `W16,K17,K18:BK2,15` | remise | remise ✅ | – | remise  | 16-13, 17-13, 17-10, 17-14, 17-12, 17-9, … |
| 4.1e | `WK16,17,K18:BK2,15` | wit wint | wit wint (33) ✅ | – | wit wint (34)  | 17-13 (33), 17-14 (33), 16-13 (37), 16-11 (37), 16-8 (37), 16-3 (37) |
| 4.2b | `WK2,K17,K18:BK3,15` | remise | remise ✅ | – | remise  | 2-5, 2-7, 2-10, 2-6, 2-9, 17-13, … |
| 4.3 | `WK16,K18:BK3,15` | remise | remise ✅ | – | wit wint (2)  | 16-13, 16-11, 16-8, 16-6, 18-14, 18-11, … |
| 4.4 | `WK2,K17,K18:B15,K16` | remise | remise ✅ | – | remise  | 2-5, 2-7, 2-10, 2-6, 2-9, 17-13, … |
| 4.10 | `WK9,14,17,18:BK3,15` | wit wint | wit wint (9) ✅ | – | wit wint (12)  | 14-12 (9), 9-2 (15) |
| 4.10b | `WK9,14,17,18:BK16,15` | wit wint | wit wint (9) ✅ | – | wit wint (12)  | 9-12 (9), 9-2 (13) |
| 4.10c | `WK9,14,17,18:BK8,15` | wit wint | wit wint (11) ✅ | – | wit wint (10)  | 14-12 (11), 9-2 (15), 9-12 (15) |
| 4.11 | `WK1,9,17,K18:BK3,15` | wit wint | wit wint (9) ✅ | – | wit wint (8)  | 1-5 (9), 9-6 (11), 1-4 (13), 17-14 (23) |
| 4.11b | `WK1,9,17,K18:BK16,15` | wit wint | wit wint (7) ✅ | – | wit wint (10)  | 1-12 (7), 1-5 (11), 1-4 (13), 17-14 (23) |
| 4.11c | `WK1,9,14,K18:BK3,15` | remise | wit wint (21) ❌ | remise | wit wint (22) ❌ | 1-4 (21), 1-5 (21) |
| 4.11d | `WK1,9,14,K18:BK16,15` | remise | wit wint (21) ❌ | remise | wit wint (22) ❌ | 1-4 (21), 1-5 (21) |
| 4.12 | `WK1,K7,9,18:BK3,15` | wit wint | wit wint (7) ✅ | – | wit wint (32)  | 9-6 (7), 1-5 (29), 7-4 (29), 7-10 (29), 7-14 (29), 7-5 (33), … |
| 4.12b | `WK1,K7,9,18:BK13,15` | remise | wit wint (25) ❌ | remise | wit wint (32) ❌ | 1-12 (25), 7-4 (31), 7-5 (31), 7-2 (31), 7-14 (31), 1-4 (35), … |
| 4.12c | `WK1,K7,9,18:BK16,15` | remise | wit wint (31) ❌ | remise | wit wint (26) ❌ | 7-4 (31), 7-14 (31), 7-5 (33), 7-2 (33), 1-4 (35), 1-5 (35), … |
| 5.1 | `WK1,K5,12,17:BK3,10` | wit wint | wit wint (35) ✅ | wit wint | wit wint (22) ✅ | 1-4 (35), 5-2 (35) |
| 5.1b | `WK1,K5,12,17:BK16,10` | wit wint | wit wint (7) ✅ | – | wit wint (36)  | 17-13 (7), 5-2 (25), 1-4 (35) |
| 5.1c | `WK1,K5,12,17:BK13,10` | wit wint | wit wint (3) ✅ | – | wit wint (36)  | 5-7 (3), 1-4 (35), 5-2 (35) |
| 5.1d | `WK1,K5,12,17:BK6,10` | wit wint | wit wint (3) ✅ | – | wit wint (36)  | 5-7 (3), 1-4 (35), 5-2 (35) |
| 5.1e | `WK1,K5,12,17:BK11,10` | wit wint | wit wint (21) ✅ | – | wit wint (36)  | 17-14 (21), 5-7 (27), 1-4 (35), 5-2 (35) |
| 5.1f | `WK1,K5,12,17:BK8,10` | wit wint | wit wint (7) ✅ | – | wit wint (6)  | 17-14 (7), 5-2 (13) |
| 5.1g | `WK1,K5,9,17:BK3,10` | remise | remise ✅ | – | remise  | 1-4, 5-2, 5-7, 5-8, 5-12, 5-15, … |
| 5.2 | `WK4,K11,17:BK1,10` | wit wint | wit wint (17) ✅ | – | wit wint (14)  | 17-14 (17), 11-6 (21), 11-3 (21), 11-13 (21), 11-16 (21) |
| 5.2b | `WK4,K11,17:BK2,10` | wit wint | wit wint (9) ✅ | – | wit wint (14)  | 4-1 (9), 11-8 (13), 11-3 (21), 11-13 (21), 11-16 (21), 17-14 (21) |
| 5.2c | `WK4,K11,17:BK9,10` | wit wint | wit wint (11) ✅ | – | wit wint (10)  | 11-8 (11), 4-1 (13), 11-3 (15), 11-16 (15), 11-13 (21) |
| 5.2d | `WK4,K11,17:BK7,10` | wit wint | wit wint (7) ✅ | – | remise  | 11-8 (7), 11-18 (15), 11-6 (21), 11-3 (21), 11-13 (21), 11-16 (21), … |
| 5.2e | `WK4,K11,17:BK15,10` | wit wint | wit wint (7) ✅ | – | wit wint (18)  | 4-1 (7), 11-6 (21), 11-3 (21), 11-13 (21), 11-16 (21), 17-14 (23) |
| 5.2f | `WK4,K11,17:BK5,10` | wit wint | wit wint (13) ✅ | – | wit wint (18)  | 4-1 (13), 4-7 (21), 11-6 (21), 11-3 (21), 11-13 (21), 11-16 (21), … |
| 5.2g | `WK4,K11,17:BK12,10` | wit wint | wit wint (7) ✅ | – | wit wint (18)  | 4-1 (7), 11-6 (21), 11-3 (21), 11-13 (21), 11-16 (21) |
| 5.3 | `WK1,K5,9,17:B10,K16` | remise | remise ✅ | remise | remise ✅ | 1-4, 5-2, 5-7, 5-8, 5-12, 5-15, … |
| 5.4 | `WK1,K5,9,12:B10,K11` | remise | remise ✅ | – | remise  | 1-4, 5-2, 5-7, 5-8, 9-6, 12-8 |
| 5.5 | `WK1,K5,9,K17:BK3,10` | wit wint | wit wint (29) ✅ | – | wit wint (28)  | 17-14 (29), 1-4 (33), 5-2 (33), 5-12 (33), 5-15 (33), 17-12 (33) |
| 5.5b | `WK1,K5,9,K17:BK11,10` | wit wint | wit wint (19) ✅ | – | wit wint (30)  | 5-7 (19), 17-14 (23), 1-4 (33), 5-2 (33), 5-12 (33), 5-15 (33), … |
| 5.5c | `WK1,K5,9,K17:BK16,10` | wit wint | wit wint (27) ✅ | – | wit wint (30)  | 17-12 (27), 1-4 (31), 5-2 (31), 17-14 (31), 5-12 (33), 5-15 (33) |
| 5.6 | `W9,12,K14,17:BK3,K16` | – | wit wint (23)  | wit wint | wit wint (24) ✅ | 14-4 (23), 14-18 (23), 14-7 (25) |
| 5.7 | `WK9,K12,14,K18:BK3,7` | wit wint | wit wint (43) ✅ | – | remise  | 9-2 (43) |
| 6.1 | `W9,10,12,15,16:B5,8,11` | wit wint | wit wint (19) ✅ | – | wit wint (10)  | 9-6 (19) |
| 6.2 | `W12,13,17,18:B2,3,4,7` | – | wit wint (45)  | – | wit wint (24)  | 17-14 (45) |
| 6.3 | `W12,13,14,18:B3,4,5,7` | wit wint | wit wint (21) ✅ | – | wit wint (52)  | 13-10 (21), 13-11 (37) |
| 6.4 | `W8,12,14,18:B3,4,5,10` | – | wit wint (15)  | remise | remise ✅ | 8x1 (15) |
| 6.5 | `W8,12,14,18:B3,4,7,10` | wit wint | wit wint (17) ✅ | – | zwart wint (35)  | 12-9 (17) |
| 6.6 | `WK1,12,14,18:B3,4,K16` | wit wint | wit wint (25) ✅ | – | remise  | 12-9 (25) |
| 6.6a | `WK2,K10,18:B4,K16` | wit wint | wit wint (5) ✅ | – | remise  | 2-7 (5) |
| 6.7 | `W12,13,15,17:B3,4,6,7` | wit wint | wit wint (39) ✅ | – | wit wint (34)  | 13-10 (39) |
| 6.7a | `W9,10,K15,17:BK3,4` | wit wint | wit wint (7) ✅ | – | wit wint (24)  | 10-7 (7), 15-1 (23), 15-12 (25), 15-5 (25), 15-18 (27) |
| 6.8 | `W12,13,14,18:B2,3,4,7` | – | remise  | – | wit wint (44)  | 12-9, 13-10, 13-11, 18-15 |
| 6.9 | `WK1,9,14,18:B2,3,K16` | wit wint | wit wint (41) ✅ | remise | remise ✅ | 18-15 (41), 1-15 (45) |
| 6.9a | `WK7,9,12,15:B2,3,K16` | – | wit wint (37)  | wit wint | wit wint (30) ✅ | 7-4 (37), 7-14 (37), 7-18 (39) |
| 6.10 | `W12,13,18:B3,4,7` | wit wint | wit wint (59) ✅ | – | remise  | 13-10 (59) |
| 6.11 | `W10,12,18:B3,4,11` | wit wint | wit wint (57) ✅ | – | remise  | 18-15 (57) |
| 6.12 | `W10,K11,15:B4,K17` | remise | remise ✅ | wit wint | wit wint (14) ✅ | 10-7, 11-8, 11-6, 11-3, 11-16, 11-18, … |
| 6.13 | `WK2,8,15:B3,K9` | wit wint | wit wint (49) ✅ | – | wit wint (40)  | 2-7 (49) |
| 6.14 | `WK2,8,15:B3,K14` | wit wint | wit wint (35) ✅ | – | wit wint (50)  | 2-10 (35), 2-9 (41), 2-6 (43), 2-5 (45) |
| 7.1 | `W14,15,16,17,18:B1,3,4,5,9` | wit wint | wit wint (25) ✅ | – | remise  | 15-12 (25), 16-13 (53) |
| 7.2 | `W11,12,14,15,16:B1,5,6,9,10` | – | remise  | – | zwart wint (19)  | 11-7 |
| 7.3 | `W13,14,15,17,18:B1,3,4,5,9` | – | remise  | remise | wit wint (52) ❌ | 13-10, 13-11, 14-11, 14-12, 15-12 |
| 7.4j | `W10,14,15,17,18:B1,4,5,8,9` | wit wint | wit wint (19) ✅ | – | remise  | 14-11 (19) |
| 7.4 | `W13,14,15,17,18:B1,3,4,8,9` | remise | wit wint (51) ❌ | – | wit wint (22)  | 14-11 (51) |
| 7.5 | `W10,14,15,17,18:B1,3,4,8,9` | – | remise  | – | remise  | 10-7, 14-11, 14-12, 17-13 |
| 7.6 | `W10,14,15,17,18:B3,4,5,8,9` | wit wint | remise ❌ | – | remise  | 10-7, 14-11, 14-12, 17-13 |
| 7.7 | `W10,15,17,18:B1,3,9,11` | remise | remise ✅ | – | zwart wint (15)  | 15-12 |
| 7.8 | `W10,12,18:B1,3,11` | – | remise  | remise | remise ✅ | 10-7 |
| 7.9 | `W11,13,15,17,18:B1,3,4,8,9` | – | remise  | – | wit wint (50)  | 11x6 |
| 7.10 | `W13,15,17,18:B4,5,8,9` | wit wint | wit wint (35) ✅ | – | remise  | 18-14 (35) |
| 7.11 | `W13,15,17,18:B2,3,4,5` | – | wit wint (47)  | – | remise  | 15-12 (47) |
| 7.13 | `W9,13,15,18:B4,7,8` | remise | wit wint (13) ❌ | – | remise  | 9-6 (13) |
| 7.13b | `W9,13,15,17:B4,7,8` | wit wint | wit wint (5) ✅ | – | remise  | 13-11 (5), 9-6 (17) |
| 7.14 | `W9,13,15,18:B5,7,8` | remise | remise ✅ | – | remise  | 9-6, 13-10, 15-12, 18-14 |
| 7.14b | `W9,13,15,17:B5,7,8` | wit wint | wit wint (31) ✅ | – | remise  | 13-11 (31) |
| 7.15 | `W9,K13,15:BK4,5` | – | remise  | – | remise  | 9-6, 13-10, 13-8, 13-6, 13-3, 13-16, … |
| 7.16 | `W13,15,17,18:B2,3,4,5` | – | wit wint (47)  | – | remise  | 15-12 (47) |
| 7.17 | `W12,13,17,18:B2,3,4,5` | – | remise  | – | wit wint (46)  | 13-10, 13-11, 17-14, 18-14, 18-15 |
| 7.18 | `W12,13,17,18:B2,3,4,7` | – | wit wint (45)  | – | wit wint (24)  | 17-14 (45) |
| 7.19 | `W12,13,14,17:B2,3,4,7` | – | wit wint (17)  | remise | remise ✅ | 13-11 (17), 12-9 (21) |
| 7.20 | `W11,12,14,17:B3,4,6,10` | remise | remise ✅ | – | wit wint (10)  | 12-8, 12-9, 17-13 |
| 7.21 | `W12,13,15,17:B2,3,4,7` | – | remise  | remise | remise ✅ | 12-9, 17-14 |
| 7.22 | `W12,13,14,18:B2,3,4,7` | – | remise  | remise | wit wint (44) ❌ | 12-9, 13-10, 13-11, 18-15 |
| 7.23 | `WK2,9,12,15:B6,K10` | remise | remise ✅ | – | wit wint (26)  | 12-8 |
| 8.1 | `WK3,10,18:B4,K17` | remise | remise ✅ | – | remise  | 3-6, 3-8, 3-11, 3-16, 10-7, 18-14, … |
| 8.2 | `W10,14,17,18:B4,K16` | remise | remise ✅ | wit wint | wit wint (18) ✅ | 10-7, 14-11, 14-12, 17-13, 18-15 |
| 8.3 | `W9,10,K15,17:B4,K16` | wit wint | wit wint (23) ✅ | wit wint | wit wint (20) ✅ | 15-1 (23), 15-12 (25), 15-5 (25), 15-18 (27) |
| 8.4 | `W10,12,K17,18:BK3,4` | remise | remise ✅ | – | remise  | 10-7, 12-8, 12-9, 17-13, 17-14, 18-14, … |
| 8.4b | `W10,12,K17,18:B4,K16` | wit wint | wit wint (5) ✅ | – | remise  | 10-7 (5) |
| 8.5 | `WK2,9,K15,17:BK3,4` | wit wint | wit wint (7) ✅ | wit wint | wit wint (20) ✅ | 2-7 (7), 2-5 (21), 2-10 (23), 15-12 (23), 15-5 (23), 15-1 (23), … |
| 8.6 | `WK2,9,K17,18:BK3,4` | wit wint | wit wint (23) ✅ | wit wint | wit wint (26) ✅ | 2-5 (23), 2-10 (27), 17-10 (27), 17-14 (29), 17-12 (29), 18-15 (37), … |
| 8.7 | `WK1,K2,14,15:BK3,4` | remise | wit wint (39) ❌ | remise | wit wint (40) ❌ | 2-10 (39), 14-12 (39), 1-5 (41), 1-12 (41), 2-5 (41), 15-12 (45) |
| 9.1 | `WK2,8,15:B3,K9` | wit wint | wit wint (49) ✅ | – | wit wint (40)  | 2-7 (49) |
| 9.2 | `W8,K11,15:BK2,3` | – | wit wint (45)  | wit wint | wit wint (34) ✅ | 11-13 (45), 11-4 (49), 11-14 (49), 11-18 (49) |
| 9.3 | `W8,K11,15:B3,K17` | – | wit wint (29)  | wit wint | wit wint (34) ✅ | 8-5 (29), 11-16 (31), 11-7 (43) |
| 9.4 | `W8,K10,15:B3,K9` | wit wint | wit wint (33) ✅ | – | remise  | 10-13 (33), 10-2 (41), 10-7 (49) |
| 9.5 | `W8,K9,15:B3,K10` | wit wint | wit wint (37) ✅ | – | wit wint (40)  | 9-14 (37), 9-2 (41), 9-6 (43), 9-12 (43), 9-17 (43) |
| 9.6 | `WK2,8,15:B3,K17` | wit wint | wit wint (39) ✅ | – | wit wint (50)  | 2-9 (39), 2-5 (43), 2-7 (43), 2-6 (43), 2-10 (45) |
| 9.7 | `WK2,8,12:B3,K15` | remise | remise ✅ | wit wint | wit wint (30) ✅ | 2-5, 2-7, 2-10, 2-6, 2-9, 8-5, … |
| 9.8 | `WK2,8,15:B3,K7` | – | wit wint (3)  | wit wint | wit wint (38) ✅ | 2x10 (3) |
| 9.12 | `W8,K10,15:B3,K14` | – | wit wint (43)  | wit wint | wit wint (34) ✅ | 10-17 (43), 10-5 (45), 10-2 (51) |
| 9.slot | `W5,K17:B6,K9` | – | wit wint (5)  | – | wit wint (4)  | 5-1 (5) |

### Varianten

#### Sectie 1

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✏️ | 1.1 | 1. 17-9 2-10 2. 16-11 10-2 3. 11-13 2-10 4. 13-17 2-5 5. 9-12 15×8 6. 18-11 8×13 7. 17×2 | wit wint | – | *tekst: 4. … 2-5*<br>ONMOGELIJK: zet 8 (zwart) 2-5 is niet legaal in B:WK9,K17,K18:BK10,15; legale zetten: 10-7, 10-5, 10-2, 10-13 |
| ✅ | 1.1 | 1. 17-9 2-10 2. 16-11 10-2 3. 11-13 2-10 4. 13-17 10-5 5. 9-12 15×8 6. 18-11 8×13 7. 17×2 | wit wint | wit wint (0) | *gecorrigeerd: 4. … 10-5* |
| ✅ | 1.2 | 1. 5-2? 3-6 | remise | remise | *natuurlijk niet 1. 5-2 3-6, remise*<br>bevestigd: 5-2? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 12-14) |
| ✅ | 1.2 | 1. 12-14 16-13 2. 5-10 13-16 3. 1-15 16-6 4. 10-2 6-9 5. 14-17 | wit wint | wit wint (6) |  |

#### Sectie 2

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 2.1 | 1. 4-1 16-3 2. 9-6 3×15 3. 17-14 | wit wint | wit wint (4) |  |
| ✅ | 2.1 | 1. 4-1 16-11 2. 17-14 | wit wint | wit wint (6) |  |
| ✅ | 2.1b | 1. 4-18 3-16 2. 18-14 16-3 3. 14-4 3-16 4. 4-1 | wit wint | wit wint (8) | *eerst een paar tempo's, dan 4-1 enz.* |
| ✅ | 2.2 | 1. 4-1 6-16 2. 18-14 | wit wint | wit wint (36) | *schijf 3 kan niet van zijn plaats* |
| ✅ | 2.3 | 1. 17-12 8-1 2. 2-5 15×8 3. 5×15 | wit wint | wit wint (4) |  |
| ✅ | 2.3 | 1. 17-12 8-13 2. 2-10 | wit wint | wit wint (4) |  |
| ✅ | 2.3 | 1. 17-12 8-3 2. 18-7 15×8 3. 2-6 8-12 4. 6-16 12-15 5. 7-18 | wit wint | wit wint (2) |  |
| ✅ | 2.3 | 1. 17-12 8-16 2. 2-9 15×8 3. 18-11 8×13 4. 9-17 | wit wint | wit wint (0) |  |
| ✅ | 2.3 | 1. 17-12 8-16 2. 2-10 15×8 3. 10-13 | wit wint | wit wint (6) |  |

#### Sectie 3.1

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 2.2 | 1. 18-14 6-9 2. 14-11 9-6 3. 11-7 6-16 | remise | remise | *gevolgd door 3-6-9, remise*<br>zet 1 (wit) 18-14 geeft de winst weg (remise) (behoudend: 4-1) |
| ✅ | 2.2 | 1. 4-1[enige] | wit wint | wit wint (44) | *dus de enige zet is 1. 4-1*<br>bevestigd: 4-1 is inderdaad de enige behoudende zet |
| ✅ | 2.2 | 1. 4-1 6-16 2. 18-14 16-6 3. 1-15 6-16 | wit wint | wit wint (33) | *wit heeft zijn doel bereikt, diagram 3.3* |
| ✅ | 2.2 | 1. 4-1 6-11! 2. 1-15[enige] 11-16 3. 18-14 16-6 4. 15-5! 6-16 5. 5-1 16-6 6. 1-15 6-16 | wit wint | wit wint (33) | *zwart draait de tempo's om, wit ook*<br>bevestigd: 1-15 is inderdaad de enige behoudende zet |
| ✏️ | 2.2 | 1. 4-1 6-11 2. 1-15 11-6 3. 14-11 6-16 | wit wint | – | *tekst: op 11-6 komt 14-11 6-16 en wit is er weer*<br>ONMOGELIJK: zet 5 (wit) 14-11 is niet legaal in W:W10,K15,18:B3,K6; legale zetten: 18-14, 15-1, 10-7, 15-12, 15-5, 15-8 |
| ✅ | 2.2 | 1. 4-1 6-11 2. 1-15 11-6 3. 18-14 6-16 | wit wint | wit wint (33) | *gecorrigeerd: 3. 18-14 6-16 (diagram 3.3)* |
| ✅ | 2.2 | 1. 4-1 6-11 2. 1-15 11-16 3. 18-14 16-6 4. 15-5 6-16 5. 5-1 16-6 6. 1-15 6-9 7. 14-11 9-6 8. 15-8 6-2 9. 11-7 2-6 10. 8-16 6-8 11. 16×6 3×8 12. 7-5 8×1 13. 10-7 | wit wint | wit wint (4) | *allemaal gedwongen zetten voor wit, maar wel gewonnen* |

#### Sectie 3.2

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ⚠️ | 3.3 | 7. 10-7[enige] 16-13 8. 7-4[enige] 13-16 9. 4-1[enige] 16-6 10. 15-5 6-13 11. 5-10 13-16 12. 1-15 16-6 13. 10-2 6-9 14. 14-11 9-17 15. 15-5 17-9 16. 5-10 9-12 17. 2-6 | wit wint | wit wint (2) | zet 1 10-7 (winst in 33) "de enige": ook 15-1 (winst in 37) behoudt/behouden het resultaat<br>zet 5 4-1 (winst in 29) "de enige": ook 15-1 (winst in 33) behoudt/behouden het resultaat<br>bevestigd: 7-4 is inderdaad de enige behoudende zet |
| ✅ | 3.3 | 7. 10-7 16-13 8. 15-1 13-11 | remise | remise | *op 15-1 komt 13-11*<br>zet 3 (wit) 15-1 geeft de winst weg (remise) (behoudend: 7-4) |
| ✅ | 3.3 | 7. 10-7 16-13 8. 7-5 3-6 | remise | remise | *op 7-5 komt 3-6*<br>zet 3 (wit) 7-5 geeft de winst weg (remise) (behoudend: 7-4) |
| ℹ️ | 3.3 | 7. 10-7 16-13 8. 7-4 13-16 9. 15-1 16-13 | – | wit wint (31) | *op 15-1 moet wit na 16-13 terug naar 15 met herhaling* |
| ✏️ | 3.3 | 7. 10-7 16-13 8. 7-4 13-16 9. 4-1 16-6 10. 15-5 6-13 11. 5-10 13-6 12. 5-2 | wit wint | – | *tekst: of 11. … 13-6 12. 5-2*<br>ONMOGELIJK: zet 11 (wit) 5-2 is niet legaal in W:WK1,K10,14:B3,K6; legale zetten: 10-2, 1-15, 10-17, 1-5, 10-7, 10-5, 1-4, 1-12, 1-8, 10-13, 14-11, 14-12 |
| ✅ | 3.3 | 7. 10-7 16-13 8. 7-4 13-16 9. 4-1 16-6 10. 15-5 6-13 11. 5-10 13-6 12. 10-2 | wit wint | wit wint (18) | *gecorrigeerd: 12. 10-2* |
| ℹ️ | 3.3 | 7. 15-1 | – | wit wint (36) | *wit kan niet wachten (na 15-1 of 15-5 terug naar 3.3 met herhaling)* |
| ℹ️ | 3.3 | 7. 15-5 | – | remise | *idem*<br>zet 1 (wit) 15-5 geeft de winst weg (remise) (behoudend: 10-7, 15-1) |
| ✅ | 3.3 | 7. 10-7 16-13 8. 7-4 13-16 9. 4-1 16-6 10. 15-5 6-13 11. 5-10 13-16 12. 1-15 16-6 13. 10-2 6-9 14. 15-12 3-6 | remise | remise | *op 15-12 is 3-6 remise*<br>zet 15 (wit) 15-12 geeft de winst weg (remise) (behoudend: 14-11) |
| ✅ | 3.3 | 7. 10-7 16-13 8. 7-4 13-16 9. 4-1 16-6 10. 15-5 6-13 11. 5-10 13-16 12. 1-15 16-6 13. 10-2 6-9 14. 14-11 3-6 15. 15-8 | wit wint | wit wint (12) | *op 3-6 gewoon 15-8* |
| ✅ | 3.5 | 1. 18-14 | wit wint | wit wint (18) | *dan wint 18-14* |
| ✅ | 3.5 | 1. 18-4 | wit wint | wit wint (18) | *of 18-4* |
| ✅ | 3.5b | 1. 18-14 | wit wint | wit wint (14) | *het stuk op 9 mag (ook) een dam zijn* |

#### Sectie 3.3

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ❌ | 3.4 | 1. 10-7 6-11 2. 7-4 | wit wint | remise | *niet 1. … 6-11, want dan wint wit na 7-4*<br>zet 1 (wit) 10-7: geeft de winst weg (remise); behoudend waren: 15-5, 15-1<br>zet 3 (wit) 7-4: geeft de winst weg (remise); behoudend waren: 15-18<br>zet 2 (zwart) 6-11 verliest (behoudend: 6-2) |
| ⚠️ | 3.4 | 1. 10-7 6-13 2. 7-4 | wit wint | wit wint (30) | *evenmin 1. … 6-13*<br>zet 1 (wit) 10-7: geeft de winst weg (remise); behoudend waren: 15-5, 15-1<br>zet 2 (zwart) 6-13 verliest (behoudend: 6-2) |
| ⚠️ | 3.4 | 1. 10-7 6-16 2. 7-4 | wit wint | wit wint (30) | *of 6-16*<br>zet 1 (wit) 10-7: geeft de winst weg (remise); behoudend waren: 15-5, 15-1<br>zet 2 (zwart) 6-16 verliest (behoudend: 6-2) |
| ⚠️ | 3.4 | 1. 10-7 6-9 2. 14-11 9-6 3. 15-8 6-2 4. 7-4 2-7 5. 8-6 3×13 6. 4×16 | wit wint | wit wint (0) | *maar ook niet 1. … 6-9*<br>zet 1 (wit) 10-7: geeft de winst weg (remise); behoudend waren: 15-5, 15-1<br>zet 2 (zwart) 6-9 verliest (behoudend: 6-2) |
| ✅ | 3.4 | 1. 10-7 6-2 2. 7-4 3-6 3. 14-11 6-9 4. 4-1 2-6 5. 1-8 6-3 | remise | remise | *de remise*<br>zet 1 (wit) 10-7 geeft de winst weg (remise) (behoudend: 15-5, 15-1) |
| ✅ | 3.4 | 1. 10-7 6-2 2. 7-4 3-6 3. 14-11 6-9 4. 15-8 2-7 5. 8-3 7×14 6. 3-11 14×7 7. 4×11 9-12 | remise | remise | zet 1 (wit) 10-7 geeft de winst weg (remise) (behoudend: 15-5, 15-1) |
| ✅ | 3.4 | 1. 10-7 6-2 2. 15-5 3-6 3. 14-11 6-8 | remise | remise | *op 2. 15-5 is 3-6 3. 14-11 6-8 remise*<br>zet 1 (wit) 10-7 geeft de winst weg (remise) (behoudend: 15-5, 15-1) |
| ✅ | 3.5c | 1. 14-11 9-6 2. 10-7 6-2 3. 7-4 2-7 4. 8-6 3×13 5. 4×16 | wit wint | wit wint (0) | *variant 1* |
| ✅ | 3.5c | 1. 14-11 9-2 2. 11-7 2-6 3. 8-16 6-8 4. 16×6 3×8 5. 7-5 8×1 6. 10-7 1-4 7. 7-5 4-7 8. 5×10 | wit wint | wit wint (0) | *variant 2* |

#### Sectie 4.1

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✏️ | 1.1 | 1. 17-9 2-10 2. 16-11 10-2 3. 11-13 2-10 4. 13-17 2-5 5. 9-12 15×8 6. 18-11 8×13 7. 17×2 | wit wint | – | *slotfinesse*<br>ONMOGELIJK: zet 8 (zwart) 2-5 is niet legaal in B:WK9,K17,K18:BK10,15; legale zetten: 10-7, 10-5, 10-2, 10-13 |

#### Sectie 4.2

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 2.3 | 1. 17-12[enige] | wit wint | wit wint (10) | *de winnende zet is 1. 17-12*<br>bevestigd: 17-12 is inderdaad de enige behoudende zet |
| ✏️ | 2.3 | 1. 17-12 8-1 2. 2-4 | wit wint | – | *op 1. … 8-1 wint wit meteen door 2. 2-4 (vergelijk 2-5 in hoofdstuk 2)*<br>ONMOGELIJK: zet 3 (wit) 2-4 is niet legaal in W:WK2,K12,K18:BK1,15; legale zetten: 2-5, 12-9, 12-14, 12-17, 2-7, 2-10, 2-6, 2-9, 12-8, 12-5, 18-14, 18-11, 18-7, 18-4 |
| ✅ | 2.3 | 1. 17-12 8-3 2. 18-4 15×8 3. 2-6 8-12 4. 6-16 12-15 5. 4-18 | wit wint | wit wint (2) | *finesse 1* |
| ✅ | 2.3 | 1. 17-12 8-16 2. 2-10 15×8 3. 10-13 | wit wint | wit wint (6) | *finesse 2* |
| ✅ | 2.3 | 1. 17-12 8-16 2. 2-9 15×8 3. 18-11 8×13 4. 9-17 | wit wint | wit wint (0) | *finesse 3* |
| ✅ | 4.2b | 1. 17-12 15×8 2. 2-6 8-12 3. 6-16 12-15 | remise | remise | *met de zwarte dam op 3 is het remise* |
| ✅ | 4.4 | 1. 17-14 16-13 2. 14-9 13-16 3. 9-17 | remise | remise | *spoken* |
| ✅ | 4.4 | 1. 17-14 16-8 2. 14-12 | wit wint | wit wint (10) | *op 16-8 verliest 14-12 (echt)*<br>zet 2 (zwart) 16-8 verliest (behoudend: 16-13, 16-3) |
| ✅ | 4.4 | 1. 17-14 16-3 | remise | remise | *op 16-3 ziet zwart het spook* |
| ✅ | 4.4 | 1. 17-14 16-13 2. 14-9 13-8 | wit wint | wit wint (11) | *geen 13-8 (terecht)*<br>zet 4 (zwart) 13-8 verliest (behoudend: 13-3, 13-16) |
| ✅ | 4.4 | 1. 17-14 16-13 2. 14-9 13-3 | remise | remise | *en 13-3 (spook)* |

#### Sectie 4.3

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 4.10 | 1. 14-12 15×8 2. 9-6 | wit wint | wit wint (6) | *finesse 1* |
| ✅ | 4.10b | 1. 9-12 15×8 2. 17-13 | wit wint | wit wint (6) | *finesse 2* |
| ✅ | 4.10c | 1. 14-12 8-16 2. 17-14 15×8 3. 14-11 8×13 4. 9-17 | wit wint | wit wint (0) | *finesse 3* |
| ✅ | 4.10c | 1. 14-12 8-3 2. 9-2 15×8 3. 2-6 | wit wint | wit wint (6) | *finesse 1* |
| ✅ | 4.11b | 1. 1-12 15×8 2. 17-13 | wit wint | wit wint (4) |  |
| ✅ | 4.11 | 1. 1-5 3-16 2. 5-12 | wit wint | wit wint (6) |  |
| ✅ | 4.11 | 1. 9-6 3×16 2. 1-12 | wit wint | wit wint (8) | *(of 9-6 3x16 1-12)* |

#### Sectie 5.1

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 5.1b | 1. 17-13 16×15 2. 5-8 15×5 3. 1×8 | wit wint | wit wint (2) |  |
| ✅ | 5.1c | 1. 5-7 | wit wint | wit wint (2) |  |
| ✅ | 5.1d | 1. 5-7 | wit wint | wit wint (2) |  |
| ✅ | 5.1e | 1. 17-14 11×18 2. 5-8 | wit wint | wit wint (18) |  |
| ✅ | 5.1e | 1. 5-7 11×4 2. 12-8 | wit wint | wit wint (24) | *grappig* |
| ✅ | 5.1e | 1. 5-7 11×4 2. 1-8 4-1 | remise | remise | *en niet 2. 1-8 4-1 =*<br>zet 3 (wit) 1-8 geeft de winst weg (remise) (behoudend: 12-8) |
| ✅ | 5.1f | 1. 17-14 8×15 2. 5-8 | wit wint | wit wint (4) |  |
| ✅ | 5.1 | 1. 5-2 3-16 2. 1-4 16-3 3. 4-18 3-16 4. 18-14 16-3 5. 14-4 3-16 6. 4-1 16-3 7. 2-5 | wit wint | wit wint (22) | *oneven aantal wachtzetten* |
| ✅ | 5.1 | 1. 5-2 3-11 2. 17-14 | wit wint | wit wint (18) | *op 3-11 komt 17-14, 5-8* |
| ✅ | 5.2b | 1. 11-8 | wit wint | wit wint (12) |  |
| ✅ | 5.2c | 1. 11-8 | wit wint | wit wint (10) |  |
| ✅ | 5.2d | 1. 11-8 | wit wint | wit wint (6) |  |
| ✅ | 5.2e | 1. 4-1 | wit wint | wit wint (6) |  |
| ✅ | 5.2f | 1. 4-1 5-2 2. 1-12 | wit wint | wit wint (10) |  |
| ✅ | 5.2g | 1. 4-1 12-9 2. 1-5 | wit wint | wit wint (10) |  |
| ✅ | 5.2 | 1. 17-14 1-5 2. 4-1 | wit wint | wit wint (10) |  |

#### Sectie 5.2

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 5.3 | 1. … 16-3 2. 17-14 10-13 | remise | remise | *gewoon 2. … 10-13 is nu remise* |
| ✅ | 5.3 | 1. … 16-3 2. 17-14 3-16 3. 14-12 16-11 4. 5-7 11×4 5. 1-8 4-1 | remise | remise | *nu wel!* |
| ✅ | 5.3 | 1. 17-14 10-13 2. 5-8 13-17 3. 8-3 | wit wint | wit wint (4) | *dan is het meteen uit*<br>zet 2 (zwart) 10-13 verliest (behoudend: 16-3) |
| ✅ | 5.3 | 1. 17-14 16-3 2. 14-12 10-13 3. 9-6 | wit wint | wit wint (4) | *op 2. … 10-13 komt 9-6, 5-8*<br>zet 4 (zwart) 10-13 verliest (behoudend: 3-11, 3-16) |
| ✅ | 5.3 | 1. 17-14 16-3 2. 14-12 3-11 | remise | remise | *2. … 3-11 kan nu wel* |
| ✅ | 5.3 | 1. 17-14 16-3 2. 14-12 3-16 3. 1-4 10-13? 4. 5-8 13-17 5. 8-3 | wit wint | wit wint (12) | *met 12-8 W+*<br>bevestigd: 10-13? verliest, terecht als fout gemarkeerd (beter: 16-13, 16-3) |
| ✅ | 5.3 | 1. 17-14 16-3 2. 14-12 3-16 3. 1-4 16-3 4. 4-14 10-13 5. 14-17 13-16 6. 5-7! | wit wint | wit wint (18) | *zwart moet een dam weggeven*<br>zet 10 (zwart) 13-16 verliest (behoudend: 3-8) |
| ✅ | 5.3 | 1. 17-14 16-3 2. 14-12 3-16 3. 1-4 16-3 4. 4-14 10-13 5. 14-17 3-8 | remise | remise | *maar jammer: remise* |
| ✅ | 5.3 | 1. 17-14 16-3 2. 14-12 3-16 3. 1-4 16-3 4. 4-14 3-16 5. 14-17 16-3 | remise | remise | *zwart blijft heen en weer gaan* |

#### Sectie 5.3

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 5.5b | 1. 5-7 | wit wint | wit wint (18) |  |
| ✅ | 5.5b | 1. 17-14 | wit wint | wit wint (22) |  |
| ✅ | 5.5c | 1. 17-12 16-3 2. 9-6 3×15 3. 5-8 15×5 4. 1×8 | wit wint | wit wint (2) |  |
| ✅ | 5.5 | 1. 1-4 3-16 2. 4-18 16-3 3. 18-14 3-16 4. 14-4 16-3 5. 4-1 | wit wint | wit wint (28) | *oneven aantal stille zetten* |
| ✅ | 5.5 | 1. 17-14 3-16 2. 14-12 | wit wint | wit wint (26) | *het oneven aantal kan ook één zijn* |
| ✅ | 5.5 | 1. 17-14 10-13 2. 14-17 13-16 3. 1-4 | wit wint | wit wint (16) |  |
| ✅ | 5.5 | 1. 17-14 10-13 2. 14-17 13-16 3. 5-7 | wit wint | wit wint (16) |  |

#### Sectie 5.5

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 5.7 | 1. 12-15 7-10 2. 15-1 10-13 3. 1-8 | remise | remise | *daarmee bereikt wit niets*<br>zet 1 (wit) 12-15 geeft de winst weg (remise) (behoudend: 9-2) |
| ✅ | 5.7 | 1. 9-2 7-10 2. 12-9 10-13 3. 14-11 13×8 4. 9-6 | wit wint | wit wint (6) | *good old finesse 1* |
| ✅ | 5.7 | 1. 9-2 7-10 2. 12-9 3-8 3. 14-12 | wit wint | wit wint (18) |  |
| ✏️ | 5.7 | 1. 9-2 3-13 2. 14-12 | wit wint | – | *zie A*<br>ONMOGELIJK: zet 3 (wit) 14-12 is niet legaal in W:WK2,K12,14,K18:B7,K13; legale zetten: 2x17 |
| ✅ | 5.7 | 1. 9-2 7-10 2. 12-9 3-16 3. 14-12 10-13 4. 2-6 13-17 5. 12-8 17-13 6. 18-11 13-10 7. 6-3 10-13 8. 8-6 | wit wint | wit wint (4) |  |
| ✅ | 5.7 | 1. 9-2 7-10 2. 12-9 3-16 3. 14-12 10-13 4. 12-8 | wit wint | wit wint (12) | *(of 12-8)* |
| ✅ | 5.7 | 1. 9-2 7-10 2. 12-9 3-16 3. 14-12 10-13 4. 9-6? | wit wint niet | remise | *maar niet 9-6*<br>bevestigd: 9-6? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 2-6, 12-8) |
| ❌ | 5.7 | 1. 9-2 7-10 2. 12-9 3-16 3. 14-12 10-13 4. 2-6 13-17 5. 6-3? | wit wint niet | wit wint (10) | *niet 6-3*<br>zet 9 (wit) 6-3?: als fout gemarkeerd, maar verandert het resultaat niet (winst in 11 blijft winst in 11) |
| ✅ | 5.7 | 1. 9-2 7-10 2. 12-9 3-16 3. 14-12 16-3 4. 18-14 10-13 5. 14-17 13-16 6. 2-7 | wit wint | wit wint (16) | *finesse 5* |
| ✅ | 5.7 | 1. 9-2 7-10 2. 12-9 3-16 3. 14-12 16-3 4. 18-14 3-16 5. 14-17 | wit wint | wit wint (34) | *bouwt 5.2 of 5.5 op* |
| ✏️ | 5.7 | 1. 9-2 3-13 2. 14-12 13-3 3. 18-14 | wit wint | – | *A*<br>ONMOGELIJK: zet 3 (wit) 14-12 is niet legaal in W:WK2,K12,14,K18:B7,K13; legale zetten: 2x17 |
| ✏️ | 5.7 | 1. 9-2 3-13 2. 14-12 13-16 3. 18-14 16-3 4. 14-17 | wit wint | – | *A*<br>ONMOGELIJK: zet 3 (wit) 14-12 is niet legaal in W:WK2,K12,14,K18:B7,K13; legale zetten: 2x17 |
| ⚠️ | 5.7 | 1. 12-1 7-10 2. 1-4 10-13 | wit wint | wit wint (9) | *ook nu verliest 2. … 10-13*<br>zet 1 (wit) 12-1: geeft de winst weg (remise); behoudend waren: 9-2<br>zet 4 (zwart) 10-13 verliest (behoudend: 3-16) |
| ⚠️ | 5.7 | 1. 12-1 7-10 2. 1-4 3-16 3. 14-12 16-13 4. 18-14 | wit wint | wit wint (30) | zet 1 (wit) 12-1: geeft de winst weg (remise); behoudend waren: 9-2<br>zet 6 (zwart) 16-13 verliest (behoudend: 10-13) |
| ⚠️ | 5.7 | 1. 12-1 7-10 2. 1-4 3-16 3. 14-12 16-3 4. 18-14 | wit wint | wit wint (30) | zet 1 (wit) 12-1: geeft de winst weg (remise); behoudend waren: 9-2<br>zet 6 (zwart) 16-3 verliest (behoudend: 10-13) |
| ✅ | 5.7 | 1. 12-1 7-10 2. 1-4 3-16 3. 14-12 10-13 4. 12-8 13-17 5. 18-11 17-10 | remise | remise | *remise na 1. 12-1*<br>zet 1 (wit) 12-1 geeft de winst weg (remise) (behoudend: 9-2) |

#### Sectie 6.3

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 6.1 | 1. 10-7 11×4 2. 9-6 | wit wint niet | remise | *wint niet*<br>zet 1 (wit) 10-7 geeft de winst weg (remise) (behoudend: 9-6) |
| ✅ | 6.1 | 1. 9-6 8×3 2. 12-8 11×6 3. 10-7 5×10 4. 15-12!! 6-9 5. 12-8 3-6 6. 8×3 9-12 7. 3-11 10-13 8. 11-8 | wit wint | wit wint (4) |  |
| ✅ | 6.1 | 1. 9-6 8×3 2. 12-8 11×6 3. 10-7 5×10 4. 15-12 6-9 5. 12-8 3-6 6. 8×3 9-12 7. 3-11 12-15 8. 11-18 | wit wint | wit wint (2) |  |
| ✅ | 6.1 | 1. 9-6 8×3 2. 12-8 11×6 3. 10-7 5×10 4. 15-12 6-9 5. 12-8 9-12 | wit wint | wit wint (9) | *9-12 wint door oppositie* |

#### Sectie 6.1

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 6.2 | 7. 18-14 2-6 | remise | remise | *met 7. … 2-6 haalt zwart snel en elegant remise*<br>zet 1 (wit) 18-14 geeft de winst weg (remise) (behoudend: 17-14) |
| ⚠️ | 6.2 | 7. 18-14 2-5 8. 12-8 | wit wint | wit wint (12) | *(zie 7.19)*<br>zet 1 (wit) 18-14: geeft de winst weg (remise); behoudend waren: 17-14<br>zet 2 (zwart) 2-5 verliest (behoudend: 2-6) |
| ⚠️ | 6.2 | 7. 18-14 7-10 8. 12-8 | wit wint | wit wint (14) | *(zie 7.19)*<br>zet 1 (wit) 18-14: geeft de winst weg (remise); behoudend waren: 17-14<br>zet 2 (zwart) 7-10 verliest (behoudend: 2-6) |
| ⚠️ | 6.2 | 7. 18-14 3-6 8. 13-11 | wit wint | wit wint (16) | *(zie 7.19)*<br>zet 1 (wit) 18-14: geeft de winst weg (remise); behoudend waren: 17-14<br>zet 2 (zwart) 3-6 verliest (behoudend: 2-6) |
| ✏️ | 6.2 | 7. 17-14 2-7 | – | – | *tekst: "na 7. 17-14 2-7" (diagram 6.3 toont 2-5)*<br>ONMOGELIJK: zet 2 (zwart) 2-7 is niet legaal in B:W12,13,14,18:B2,3,4,7; legale zetten: 7-10, 2-6, 2-5, 3-6, 7-11 |
| ✅ | 6.2 | 7. 17-14 2-5 | wit wint | wit wint (21) | *diagram 6.3* |

#### Sectie 6.3

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 6.3 | 1. 13-10 3-6 2. 12-9 6-8 3. 18-15 8-11 4. 14-12 5-8 5. 10×5 8×1 6. 12-8 | wit wint | wit wint (8) |  |
| ✅ | 6.3 | 1. 13-10 5-8 2. 10×5 8×1 3. 12-8 | wit wint | wit wint (14) |  |
| ✅ | 6.3 | 1. 13-11 7-10 2. 11-7 4×11 3. 14×2 10-13 4. 12-8 13-17 5. 2-7 17-9 6. 7-11 9-14 7. 8-6 | wit wint | wit wint (2) |  |
| ✅ | 6.3 | 1. 13-11 7-10 2. 11-7 4×11 3. 14×2 10-13 4. 12-8 13-17 5. 2-7 17-9 6. 7-11 9-2 7. 18-15 2-9 8. 8-5 | wit wint | wit wint (32) |  |
| ✅ | 6.3 | 1. 13-11 7-10 2. 11-7 4×11 3. 14×2 10-13 4. 12-8 13-17 5. 2-7 17-9 6. 7-11 9-17 7. 8-5 | wit wint | wit wint (30) |  |
| ✅ | 6.3 | 1. 13-11 3-6 2. 12-9 5-8 3. 9×2 8×13 4. 2×17 | wit wint | wit wint (4) |  |

#### Sectie 6.1

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 6.3 | 1. 13-11 7-10 2. 11-8?? | remise | remise | *wit kan het nog makkelijk verprutsen*<br>bevestigd: 11-8?? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 11-7) |
| ✅ | 6.4 | 9. … 10-13 10. 8×1 3-6!! | remise | remise | *zwart maakt nu remise* |

#### Sectie 6.3

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 6.5 | 1. 18-15 7-11 2. 8×13 10×17 3. 12-9 | wit wint niet | remise | *wint niet*<br>zet 1 (wit) 18-15 geeft de winst weg (remise) (behoudend: 12-9) |
| ✅ | 6.5 | 1. 12-9 7-11 2. 8×13 10×12 3. 9×14 4-7 4. 18-15 3-6 5. 15-12 7-10 6. 14-11 | wit wint | wit wint (6) | *de oplossing* |

#### Sectie 6.2

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 6.4 | 9. … 10-13 10. 8×1 13-16?? | wit wint | wit wint (25) | *diagram 6.6*<br>bevestigd: 13-16?? verliest, terecht als fout gemarkeerd (beter: 3-6) |

#### Sectie 6.4

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 6.6 | 1. 12-9 3-6 2. 9×2 16-13 3. 1-5 13-3 4. 5-10 3-16 5. 10-17 16-3 6. 14-11 3×16 7. 2-7 4×11 8. 17-13 11-14 9. 18×11 | wit wint | wit wint (0) | *Manoury 1770* |
| ✅ | 6.6a | 1. 2-7 4×11 2. 10-13 | wit wint | wit wint (2) | *wit wint door 2-7, 10-13* |
| ✅ | 6.7 | 1. 13-10 6-8 2. 10×5 8×1 3. 12-8 | wit wint | wit wint (14) |  |
| ✅ | 6.7 | 1. 13-10 7-11 2. 12-9 6-8 3. 9-6 | wit wint | wit wint (10) |  |
| ✅ | 6.7 | 1. 13-10 7-11 2. 12-9 11-13 3. 9×2 13-16 4. 15-12 3-6 5. 2×9 16-3 6. 9-2 3-16 7. 2-5 16-6 8. 12-9 6-3 9. 5-1 3-16 10. 1-15 16-3 11. 10-7 4×11 12. 9-6 3×8 13. 15×5 | wit wint | wit wint (2) |  |
| ✅ | 6.7 | 1. 13-10 7-11 2. 12-9 11-13 3. 9×2 13-16 4. 15-12 3-6 5. 2×9 16-3 6. 9-2 3-16 7. 12-9? 16-8 | remise | remise | *12-9? 16-8 =*<br>bevestigd: 12-9? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 2-5, 2-9) |
| ✅ | 6.7 | 1. 13-10 7-11 2. 12-9 11-13 3. 9×2 13-16 4. 15-12 3-6 5. 2×9 16-3 6. 9-2 3-16 7. 2-5 16-8? 8. 5-1 8×15 9. 17-13! | wit wint | wit wint (18) | *16-8? 5-1 8x15 17-13! W+*<br>bevestigd: 16-8?: verliest sneller (verlies in 22 i.p.v. verlies in 26) |
| ✅ | 6.7a | 1. 10-7 4×11 2. 9-6 | wit wint | wit wint (4) | *wit wint door 10-7, 9-6* |

#### Sectie 6.2

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 6.8 | 7. … 7-10 8. 13-11 4-7 9. 11×4 10-13 10. 4-1 13-16 11. 12-9 | remise | remise | *diagram 6.9 met zwart aan zet*<br>zet 6 (wit) 4-1 geeft de winst weg (remise) (behoudend: 12-8) |
| ✅ | 6.9 | 11. … 16-13 12. 18-15 13-16 13. 1-4 3-6 14. 4-1 6-8 15. 1×12 2-5 16. 12×1 | remise | remise | *een vervelende variant, standaard-remise* |
| ✅ | 6.9 | 11. … 16-13 12. 18-15 13-16 13. 14-12 3-6 | remise | remise | *op 14-12 is 3-6 remise* |
| ✅ | 6.9 | 11. … 16-13 12. 18-15 13-16 13. 15-12 3-6 | remise | remise | *op 15-12 is 3-6 remise* |

#### Sectie 6.5

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 6.9 | 1. 14-11 16×6 2. 1-5 2×7 3. 9×2 7-10 | wit wint niet | remise | *niet meer dan remise*<br>zet 1 (wit) 14-11 geeft de winst weg (remise) (behoudend: 18-15, 1-15) |
| ✅ | 6.9 | 1. 1-4 3-6! 2. 4-1 6-8 3. 1×12 2-5 4. 12×1 | remise | remise | *de standaardremise van hoofdstuk 2*<br>zet 1 (wit) 1-4 geeft de winst weg (remise) (behoudend: 18-15, 1-15) |
| ✅ | 6.9 | 1. 14-12 3-6 | remise | remise | *helemaal meteen remise*<br>zet 1 (wit) 14-12 geeft de winst weg (remise) (behoudend: 18-15, 1-15) |
| ✅ | 6.9 | 1. 18-15 16-13 2. 14-12! 13-11 3. 1-4 11-16 4. 4-7 16-13 5. 7-5! 2×7 6. 9-6 3×8 7. 12×17 | wit wint | wit wint (0) | *diagram 6.9a, vondst van Hans Jansen* |
| ✅ | 6.9 | 1. 18-15 16-13 2. 14-12 3-6 3. 1-4 | wit wint | wit wint (30) | *op 3-6 komt 1-4 en 6-8 gaat niet* |
| ✏️ | 6.9 | 1. 18-15 16-13 2. 1-4 16-8 3. 14-12 | wit wint | – | *trouwens: 2. 1-4 16-8 3. 14-12 kon ook*<br>ONMOGELIJK: zet 4 (zwart) 16-8 is niet legaal in B:WK4,9,14,15:B2,3,K13; legale zetten: 13-8, 13-6, 13-16, 3-6, 13-10, 2-5, 2-6, 13-11, 13-17 |
| ✅ | 6.9 | 1. 18-15 16-13 2. 1-4 13-8 3. 14-12 | wit wint | wit wint (36) | *idem met 13-8 (de zwarte dam staat op 13)* |
| ✅ | 6.9 | 1. 18-15 16-13 2. 14-12 13-11 3. 1-4 11-6 4. 4-18 6-16 5. 18-7 | wit wint | wit wint (30) |  |
| ✅ | 6.9 | 1. 18-15 16-13 2. 14-12 13-11 3. 1-4 11-13 4. 4-14 13-16 5. 14-7 | wit wint | wit wint (30) |  |
| ✅ | 6.9 | 1. 18-15 16-13 2. 14-12 13-11 3. 1-4 11-13 4. 4-14 13-6 5. 14-18 | wit wint | wit wint (32) |  |
| ✅ | 6.9a | 1. … 16-13 2. 7-5 2×7 3. 9-6 | wit wint | wit wint (2) |  |
| ✅ | 6.9 | 1. … 16-13 2. 1-4 3-6 3. 4-1 6-8 4. 1×12 2-5 5. 12×1 | remise | remise | *zwart aan zet, remise* |
| ✅ | 6.9 | 1. … 16-13 2. 14-12 3-6 3. 1-4 13-16 4. 4-7 16-13 5. 18-15 2-5 6. 7×2 13-10! | remise | remise | *tweede poging* |
| ✅ | 6.9 | 1. … 16-13 2. 18-15 13-16 3. 1-12 16-13 | remise | remise | *wit komt niet verder* |
| ✅ | 6.9 | 1. … 16-13 2. 1-12 13-16?? 3. 12-15 16-13 4. 15-1 13-16 | wit wint | wit wint (41) | *diagram 6.9 met wit aan zet*<br>bevestigd: 13-16?? verliest, terecht als fout gemarkeerd (beter: 3-6) |
| ✅ | 6.9 | 1. … 16-13 2. 1-12 3-6 | remise | remise | *maar zwart speelt 2. … 3-6, remise* |

#### Sectie 6.6

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 6.10 | 1. 18-15 3-6 | remise | remise | *natuurlijk meteen remise*<br>zet 1 (wit) 18-15 geeft de winst weg (remise) (behoudend: 13-10) |
| ✅ | 6.10 | 1. 12-9 7-10 | remise | remise | *gaat snel*<br>zet 1 (wit) 12-9 geeft de winst weg (remise) (behoudend: 13-10) |
| ✅ | 6.10 | 1. 18-14 3-6! 2. 13-11 6-9! 3. 12-8 7-10 4. 8-5 4-7 5. 11×4 10-13 | remise | remise | *een aardige poging*<br>zet 1 (wit) 18-14 geeft de winst weg (remise) (behoudend: 13-10) |
| ✅ | 6.10 | 1. 13-10! 7-11 2. 18-15! 3-6 3. 12-8 11-13 4. 8×3 13-17 5. 3-11 | wit wint | wit wint (14) | *grappige winst, diagram 6.12* |
| ✏️ | 6.10 | 1. 13-10 7-11 2. 18-15 4-7 3. 10×5 10-13 4. 12-8 13-17 5. 5-2 | wit wint | – | *tekst: 3. … 10-13*<br>ONMOGELIJK: zet 6 (zwart) 10-13 is niet legaal in B:W5,12,15:B3,11; legale zetten: 11-13, 3-6, 11-14 |
| ✅ | 6.10 | 1. 13-10 7-11 2. 18-15 4-7 3. 10×5 11-13 4. 12-8 13-17 5. 5-2 | wit wint | wit wint (50) | *gecorrigeerd: 3. … 11-13 (naar diagram 6.13/6.14)* |
| ✅ | 6.13 | 6. 8-5 9-17 | remise | remise | *gevolgd door 3-6, 17-13*<br>zet 1 (wit) 8-5 geeft de winst weg (remise) (behoudend: 2-7) |
| ✅ | 6.13 | 6. 2-7 9-6 7. 7-11 6-2 8. 11-13 2-10 9. 13-17 10-2 10. 17-9 2-7 11. 9-2 7-14 | wit wint | wit wint (35) | *diagram 6.14* |
| ✏️ | 6.13 | 6. 2-7 9-6 7. 7-11 6-2 8. 11-13 2-10 9. 13-17 10-2 10. 17-9 2-7 11. 9-2 7-14 12. 2-13 14-17 13. 8-5 3-6 14. 15-12 17×9 15. 10-17 | wit wint | – | *tekst: 12. 2-13*<br>ONMOGELIJK: zet 13 (wit) 2-13 is niet legaal in W:WK2,8,15:B3,K14; legale zetten: 2-10, 2-9, 2-6, 2-5, 2-7, 8-5, 8-6, 15-12 |
| ✅ | 6.13 | 6. 2-7 9-6 7. 7-11 6-2 8. 11-13 2-10 9. 13-17 10-2 10. 17-9 2-7 11. 9-2 7-14 12. 2-10 14-17 13. 8-5 3-6 14. 15-12 17×9 15. 10-17 | wit wint | wit wint (4) | *gecorrigeerd: 12. 2-10 (diagram 6.15)* |
| ✅ | 6.13 | 6. 2-7 9-6 7. 7-11 6-2 8. 11-13 2-10 9. 13-17 10-2 10. 17-9 2-7 11. 9-2 7-14 12. 2-10 14-17 13. 8-5 3-6 14. 5-2 6-8! | remise | remise | *na 14. 5-2 6-8! is het remise*<br>zet 17 (wit) 5-2 geeft de winst weg (remise) (behoudend: 15-12) |

#### Sectie 7.1

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 7.1 | 3. 15-12! 3-6 4. 12-8! 5×12 5. 18-15 6-8 6. 14-11 8×13 7. 15×8 | wit wint | wit wint (10) | *uit* |
| ✅ | 7.1 | 3. 15-12 4-7 4. 12-8! 5×12 5. 18-15 | wit wint | wit wint (20) | *uit* |
| ✅ | 7.1 | 3. 15-12 5-7 4. 12-8 | wit wint | wit wint (20) | *uit* |
| ✅ | 7.1 | 3. 15-12 5-8 4. 12×5 1×8 5. 16-13 3-6 6. 18-15 | wit wint | wit wint (8) | *uit* |
| ℹ️ | 7.2 | 1. 11-7 5-8 2. 12×5 1×8 3. 7-4 8-11 4. 14×7 10×5 | – | remise | *wit neemt "voor de zekerheid" een remisecombinatie* |

#### Sectie 7.2

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 7.1 | 3. 14-12 | remise | remise | *met 3. 14-12 helemaal niets*<br>zet 1 (wit) 14-12 geeft de winst weg (remise) (behoudend: 15-12, 16-13) |
| ✅ | 7.1 | 3. 14-11 5-8 4. 11×6 3×8 5. 16-13 4-7! 6. 13-11 8×13 7. 17×5 1×8 8. 18-14 9-12 | remise | remise | *eieren voor zijn geld*<br>zet 1 (wit) 14-11 geeft de winst weg (remise) (behoudend: 15-12, 16-13) |
| ❌ | 7.1 | 3. 14-11 5-8 4. 11×6 3×8 5. 16-13 1-5 6. 17-14! | wit wint | remise | *op 5. … 1-5 verliest zwart*<br>zet 1 (wit) 14-11: geeft de winst weg (remise); behoudend waren: 15-12, 16-13<br>zet 7 (wit) 17-14: geeft de winst weg (remise); behoudend waren: 18-14<br>zet 6 (zwart) 1-5 verliest (behoudend: 4-7) |
| ❌ | 7.1 | 3. 14-11 5-8 4. 11×6 3×8 5. 16-13 4-7 6. 17-14 7-10 | zwart wint | remise | *tekst "9. 17-14 7-10": meteen gewonnen voor zwart*<br>zet 1 (wit) 14-11 geeft de winst weg (remise) (behoudend: 15-12, 16-13) |
| ✅ | 7.1 | 3. 14-11 5-8 4. 11×6 3×8 5. 16-13 4-7 6. 13-10 7-11 | zwart wint | zwart wint (14) | *tekst "9. 13-10 7-11": meteen gewonnen voor zwart*<br>zet 1 (wit) 14-11 geeft de winst weg (remise) (behoudend: 15-12, 16-13)<br>zet 7 (wit) 13-10 verliest (behoudend: 13-11, 17-14, 18-14) |
| ✅ | 7.1 | 3. 14-11 5-8 4. 11×6 3×8 5. 16-13 4-7 6. 18-14 9-12! 7. 14×9 7-11 8. 13-10 11-13 9. 10-7 13-16 10. 7-4 16-11 11. 17-14 11×18 12. 15-12 8×15 13. 9-6 18-11 14. 6-2 | remise | remise | *tekst "9. 18-14 ...": wit haalt nog net remise*<br>zet 1 (wit) 14-11 geeft de winst weg (remise) (behoudend: 15-12, 16-13) |
| ❌ | 7.3 | 3. … 5-8[enige] | remise | wit wint (51) | *de enige weg naar remise*<br>zet 1 5-8 (verlies in 52) "de enige": ook 3-6 (verlies in 22), 5-7 (verlies in 22), 4-7 (verlies in 18), 9-12 (verlies in 18) behoudt/behouden het resultaat |
| ✅ | 7.3 | 3. … 5-7 4. 13-11 3-6 5. 15-12 1-5 6. 12-8 5×12 7. 18-15 6-8 8. 11×6 9×2 9. 15×8 | wit wint | wit wint (10) |  |
| ✅ | 7.3 | 3. … 3-6 4. 13-11 6-8 | remise | remise | *13-11 6-8 =*<br>zet 2 (wit) 13-11 geeft de winst weg (remise) (behoudend: 13-10) |
| ✏️ | 7.3 | 3. … 3-6 4. 13-10!! 5-8 5. 10-7 4×11 6. 14×10 | wit wint | – | *tekst: 6. 14x10*<br>ONMOGELIJK: zet 6 (wit) 14×10 is niet legaal in W:W14,15,17,18:B1,6,8,9,11; legale zetten: 14x7 |
| ✅ | 7.3 | 3. … 3-6 4. 13-10 5-8 5. 10-7 4×11 6. 14×7 | wit wint | wit wint (14) | *gecorrigeerd: 6. 14x7* |
| ✅ | 7.3 | 3. … 3-6 4. 13-10!! 6-8!! | wit wint | wit wint (19) | *diagram 7.4 (kerstpuzzel)* |
| ✅ | 7.4j | 5. 14-12 9×14 6. 18×6 5-7 | remise | remise | zet 1 (wit) 14-12 geeft de winst weg (remise) (behoudend: 14-11) |
| ✅ | 7.4j | 5. 10-7?? 5×10 6. 14-12 9×14 7. 18×6 10-13! 8. 17×10 4-7 9. 10×5 1×3 | zwart wint | zwart wint (6) | *zwart wint zelfs*<br>bevestigd: 10-7?? verliest zelfs, terecht als fout gemarkeerd (beter: 14-11) |
| ✏️ | 7.4j | 5. 14-11 8×13 6. 15-12 9×14 7. 18×16 5-8 8. 17-13 1-5 9. 13-11 8×13 10. 16×13 | wit wint | – | ONMOGELIJK: zet 11 (wit) 16×13 is niet legaal in W:W10,16:B4,5,13; legale zetten: 16x11, 10x17 |
| ✏️ | 7.4j | 5. 14-11 8×13 6. 15-12 9×14 7. 18×16 4-7 8. 17-14 1-5 9. 16-13 | wit wint | – | ONMOGELIJK: zet 8 (zwart) 1-5 is niet legaal in B:W10,14,16:B1,5,7; legale zetten: 1-4, 5-8, 7-11 |
| ✅ | 7.4j | 5. 14-11 8×13 6. 15-12 9×14 7. 18×16 5-7 8. 10×5 1×8 9. 16-13! 8-12 10. 13-11 12-15 11. 17-13!! | wit wint | wit wint (6) | *tekst: "dus 8. 5-7 10x5 9. 1x8 16-13! 10. 8-12 13-11 11. 12-15 17-13!!"* |

#### Sectie 7.3

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✏️ | 7.1 | 3. 16-11 5-8 | – | – | *tekst: "3. 16-11 5-8" (bedoeld 16-13)*<br>ONMOGELIJK: zet 1 (wit) 16-11 is niet legaal in W:W14,15,16,17,18:B1,3,4,5,9; legale zetten: 15-12, 16-13, 14-11, 14-12, 17-13 |
| ✅ | 7.4 | 4. 14-12 | remise | remise | *loopt het snel remise*<br>zet 1 (wit) 14-12 geeft de winst weg (remise) (behoudend: 14-11) |
| ❌ | 7.4 | 4. 13-11 8×13 5. 17×10 4-7 6. 10×5 1×8 | zwart wint | remise | *verliest wit zelfs*<br>zet 1 (wit) 13-11 geeft de winst weg (remise) (behoudend: 14-11) |
| ✅ | 7.5 | 4. … 3-6 5. 10-7 | wit wint | wit wint (16) | *meteen uit*<br>zet 1 (zwart) 3-6 verliest (behoudend: 1-5, 8-11) |
| ✅ | 7.5 | 4. … 1-5 5. 17-13 3-6 6. 13-11 8×13 7. 10×17 5-7 8. 17-13 7-10 9. 14-11 10×17 10. 18-14 17×12 11. 15×3 | wit wint | wit wint (16) | *diagram 7.6*<br>zet 3 (zwart) 3-6 verliest (behoudend: 9-12) |
| ℹ️ | 7.5 | 4. … 8-11 5. 14×7 4×11 6. 15-12[enige] 9×14 7. 17×12 | – | remise | *het enige is 6. 15-12*<br>bevestigd: 15-12 is inderdaad de enige behoudende zet |
| ✅ | 7.7 | 6. 15-12[enige] | remise | remise | *het enige is 6. 15-12, en dat is meteen heel goed*<br>bevestigd: 15-12 is inderdaad de enige behoudende zet |
| ✅ | 7.8 | 7. … 3-6![enige] | remise | remise | *de enige remise*<br>bevestigd: 3-6 is inderdaad de enige behoudende zet |
| ✅ | 7.8 | 7. … 1-4 8. 18-15 3-6 9. 12-8 11-14 10. 8×3 14-17 11. 3-11 | wit wint | wit wint (14) | zet 1 (zwart) 1-4 verliest (behoudend: 3-6) |
| ✅ | 7.8 | 7. … 1-4 8. 18-15 4-7 | wit wint | wit wint (55) | *de taaie winst*<br>zet 1 (zwart) 1-4 verliest (behoudend: 3-6) |
| ✅ | 7.8 | 7. … 1-5 8. 18-15 5-7 | wit wint | wit wint (55) | *de taaie winst*<br>zet 1 (zwart) 1-5 verliest (behoudend: 3-6) |
| ✅ | 7.8 | 7. … 1-5 8. 18-15 3-6 9. 12-8 5×12 10. 15×3 11-14 11. 3-11 | wit wint | wit wint (2) | zet 1 (zwart) 1-5 verliest (behoudend: 3-6) |
| ✅ | 7.8 | 7. … 3-6 8. 12-8 11-13 9. 8×3 13-17 | remise | remise | *wit komt niet verder* |
| ✅ | 7.9 | 4. … 9-12 5. 11×6 3×8 6. 13-11 8×13 7. 17×10 | wit wint | wit wint (8) |  |
| ✅ | 7.9 | 4. … 3-6 5. 17-14 1-5 6. 13-10 8×13 7. 10×17 5-7 8. 17-13 7-10 9. 14-11 10×17 10. 18-14 17×12 11. 15×3 | wit wint | wit wint (16) |  |

#### Sectie 7.4

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 7.10 | 6. 18-14 9-12 7. 14×9 4-7 8. 13-11 8×13 9. 17×10 7-11 10. 15-12 | wit wint | wit wint (26) |  |
| ✅ | 7.10 | 6. 18-14 4-7 7. 13-11 8×13 8. 17×10 | wit wint | wit wint (4) |  |
| ✅ | 7.10 | 6. 18-14 9-12 7. 14×9 4-7 8. 17-14 | remise | remise | *17-14 is remise*<br>zet 5 (wit) 17-14 geeft de winst weg (remise) (behoudend: 13-11) |
| ❌ | 7.10 | 6. 17-14 9-12 7. 14×9 5-7 | remise | wit wint (13) | *diagram 7.13*<br>zet 4 (zwart) 5-7: verliest; behoudend waren: 4-7<br>zet 1 (wit) 17-14 geeft de winst weg (remise) (behoudend: 18-14) |
| ✅ | 7.10 | 6. 17-14 9-12 7. 14×9 4-7 | remise | remise | *diagram 7.14*<br>zet 1 (wit) 17-14 geeft de winst weg (remise) (behoudend: 18-14) |
| ✅ | 7.13b | 1. 13-11 | wit wint | wit wint (4) |  |
| ✅ | 7.14b | 1. 13-11 | wit wint | wit wint (30) |  |
| ✅ | 7.13 | 8. 13-10 7-11 9. 10-7?? 8-12 | zwart wint | zwart wint (12) | bevestigd: 10-7?? verliest, terecht als fout gemarkeerd (beter: 9-6, 15-12)<br>zet 1 (wit) 13-10 geeft de winst weg (remise) (behoudend: 9-6) |
| ✅ | 7.13 | 8. 15-12 8×15 9. 9-6 7-10 10. 13-11 4-7 | remise | remise | zet 1 (wit) 15-12 geeft de winst weg (remise) (behoudend: 9-6) |
| ✅ | 7.13 | 8. 15-12 8×15 9. 13-10 7-11 10. 9-6 4-7 | remise | remise | zet 1 (wit) 15-12 geeft de winst weg (remise) (behoudend: 9-6) |
| ✅ | 7.13 | 8. 9-6 8×3 9. 13-10 7-11 10. 15-12 3-6 11. 12-8 11-13 12. 8×3 13-17 | remise | remise | zet 3 (wit) 13-10 geeft de winst weg (remise) (behoudend: 13-11) |
| ⚠️ | 7.13 | 8. 9-6 8×3 9. 13-10 7-11 10. 15-12 4-7 | wit wint | wit wint (39) | *4-7 verliest*<br>zet 3 (wit) 13-10: geeft de winst weg (remise); behoudend waren: 13-11<br>zet 6 (zwart) 4-7 verliest (behoudend: 3-6) |
| ℹ️ | 7.14 | 8. 18-14 7-10 9. 14-11 10×17 10. 11×6 17-14!! 11. 6-3 14-18 12. 3-13 18-4 | – | remise | *diagram 7.15* |
| ❌ | 7.15 | 13. 15-12 4-1 | wit wint | remise | *puzzel 1: hoe wint wit na 13. 15-12 4-1?* |
| ✅ | 7.15 | 13. 15-12 | remise | remise | *puzzel 2: hoe maakt zwart remise na 13. 15-12?* |

#### Sectie 7.5

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 7.16 | 6. 13-11 5-8 7. 11×6 3×8 8. 18-14 8-11 | zwart wint | zwart wint (10) | zet 1 (wit) 13-11 verliest zelfs (behoudend: 15-12) |
| ✅ | 7.16 | 6. 13-11 5-8 7. 11×6 3×8 8. 17-14 8-11 | zwart wint | zwart wint (12) | zet 1 (wit) 13-11 verliest zelfs (behoudend: 15-12) |
| ✅ | 7.16 | 6. 13-11 5-8 7. 11×6 3×8 8. 17-13 4-7 | zwart wint | zwart wint (14) | zet 1 (wit) 13-11 verliest zelfs (behoudend: 15-12) |
| ✏️ | 7.16 | 6. 13-10 5-8 7. 17-14 8-11 8. 14×7 5×11 9. 15-12 3-6 10. 12-8 11-13 11. 8×3 13-16 | remise | – | *tekst: 8. 14x7 5x11*<br>ONMOGELIJK: zet 6 (zwart) 5×11 is niet legaal in B:W7,10,15,18:B2,3,4; legale zetten: 4x11<br>zet 1 (wit) 13-10 geeft de winst weg (remise) (behoudend: 15-12) |
| ✅ | 7.16 | 6. 13-10 5-8 7. 17-14 8-11 8. 14×7 4×11 9. 15-12 3-6 10. 12-8 11-13 11. 8×3 13-16 | remise | remise | *gecorrigeerd: 8. … 4x11*<br>zet 1 (wit) 13-10 geeft de winst weg (remise) (behoudend: 15-12) |
| ✏️ | 7.16 | 6. 13-10 5-8 7. 17-14 8-11 8. 14×7 4×11 9. 15-12 2-7 10. 18-15 | wit wint | – | *(* 9. … 2-7 10. 18-15 W+), met 8. … 4x11*<br>zet 1 (wit) 13-10: geeft de winst weg (remise); behoudend waren: 15-12<br>ONMOGELIJK: zet 8 (zwart) 2-7 is niet legaal in B:W10,12,18:B2,3,11; legale zetten: 3-6, 2-5, 2-6, 11-13, 11-14 |
| ⚠️ | 7.16 | 6. 13-10 5-8 7. 17-14 8-11 8. 14×7 4×11 9. 15-12 2-6 10. 12-9 6-8 11. 9-6 8-12 12. 6-2 | wit wint | wit wint (14) | *(* 9. … 2-6 ... W+), met 8. … 4x11*<br>zet 1 (wit) 13-10: geeft de winst weg (remise); behoudend waren: 15-12<br>zet 8 (zwart) 2-6 verliest (behoudend: 3-6) |
| ✅ | 7.16 | 6. 17-14 5-8 7. 14-11 2-5 | remise | remise | zet 1 (wit) 17-14 geeft de winst weg (remise) (behoudend: 15-12) |
| ✅ | 7.16 | 6. 17-14 5-8 7. 14-12 3-6 | remise | remise | zet 1 (wit) 17-14 geeft de winst weg (remise) (behoudend: 15-12) |
| ✅ | 7.16 | 6. 17-14 5-8 7. 13-10 8-11 | remise | remise | zet 1 (wit) 17-14 geeft de winst weg (remise) (behoudend: 15-12) |
| ✅ | 7.16 | 6. 18-14 5-8 | remise | remise | zet 1 (wit) 18-14 geeft de winst weg (remise) (behoudend: 15-12) |
| ✅ | 7.16 | 6. 15-12 | wit wint | wit wint (46) | *de beste zet* |
| ✅ | 7.17 | 6. … 3-6 7. 13-11!! | wit wint | wit wint (12) |  |
| ✅ | 7.17 | 6. … 2-6 7. 12-9 6-8 8. 9-6 | wit wint | wit wint (16) |  |
| ✅ | 7.17 | 6. … 4-7 7. 18-14 7-10 8. 13-11 3-6 9. 12-8 5×12 10. 14×9 | wit wint | wit wint (8) |  |
| ✅ | 7.17 | 6. … 4-7 7. 18-14 3-6 8. 13-11 6-8 9. 11×4 8×15 10. 14-12 15×8 11. 4-1 | wit wint | wit wint (12) |  |
| ✅ | 7.17 | 6. … 4-7 7. 18-14 2-6 8. 12-9 6-8 9. 13-11 8×13 10. 17×10 | wit wint | wit wint (8) |  |
| ✏️ | 7.17 | 6. … 5-8 7. 12×5 2×10 8. 18-14 3-6 9. 13-11 | wit wint | – | *tekst: 7. … 2x10*<br>ONMOGELIJK: zet 3 (zwart) 2×10 is niet legaal in B:W5,13,17,18:B2,3,4; legale zetten: 2x7 |
| ✅ | 7.17 | 6. … 5-8 7. 12×5 2×7 8. 18-14 3-6 9. 13-11 | wit wint | wit wint (12) | *gecorrigeerd: 7. … 2x7* |

#### Sectie 7.6

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✏️ | 7.18 | 7. 13-10 7-11 8. 17-14 4-7 9. 10×5 2×10 10. 12-9 10-13 11. 14×7 10×5 12. 18-15 | remise | – | ONMOGELIJK: zet 6 (zwart) 2×10 is niet legaal in B:W5,12,14,18:B2,3,11; legale zetten: 2x7<br>zet 1 (wit) 13-10 geeft de winst weg (remise) (behoudend: 17-14) |
| ✏️ | 7.18 | 7. 12-9 2-7 8. 13-10 7-11 9. 17-14 4-7 10. 14-12 5-8 11. 10×5 8×15 | remise | – | *tekst: 7. 12-9 2-7*<br>ONMOGELIJK: zet 2 (zwart) 2-7 is niet legaal in B:W9,13,17,18:B2,3,4,7; legale zetten: 2-5, 7-10, 3-6, 7-11, 2-6<br>zet 1 (wit) 12-9 geeft de winst weg (remise) (behoudend: 17-14) |
| ✅ | 7.19 | 7. … 2-5 8. 12-8 | wit wint | wit wint (12) | zet 1 (zwart) 2-5 verliest (behoudend: 2-6) |
| ✅ | 7.19 | 7. … 7-10 8. 12-8 | wit wint | wit wint (14) | zet 1 (zwart) 7-10 verliest (behoudend: 2-6) |
| ✅ | 7.19 | 7. … 3-6 8. 13-11 | wit wint | wit wint (16) | zet 1 (zwart) 3-6 verliest (behoudend: 2-6) |
| ✅ | 7.19 | 7. … 2-6!! 8. 13-11 7-10!! | remise | remise | *een dijk van een stand* |
| ✅ | 7.21 | 7. … 7-10 8. 12-8 | wit wint | wit wint (18) | zet 1 (zwart) 7-10 verliest (behoudend: 2-5, 3-6) |
| ✅ | 7.21 | 7. … 2-6 | wit wint | wit wint (39) | *wit wint, zie diagram 6.7*<br>zet 1 (zwart) 2-6 verliest (behoudend: 2-5, 3-6) |
| ✅ | 7.21 | 7. … 3-6 | remise | remise |  |
| ✅ | 7.21 | 7. … 2-5 | remise | remise |  |
| ✅ | 7.21 | 7. … 3-6 8. 17-14 2-5 9. 13-10 6-9 10. 14-11 7×14 11. 12×17 5-8 12. 17-13 4-7 | remise | remise |  |

#### Sectie 7.7

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 7.22 | 7. … 2-6 8. 13-10! 6-8 9. 10×5 8×1 10. 12-8 | wit wint | wit wint (14) |  |
| ✅ | 7.22 | 7. … 2-6 8. 13-10 7-11 9. 14×7 4×11 10. 12-9 6-8 11. 9-6 | wit wint | wit wint (16) |  |
| ✅ | 7.22 | 7. … 3-6 8. 13-11 2-5 9. 12-9 | wit wint | wit wint (16) |  |
| ✅ | 7.22 | 7. … 3-6 8. 13-11 6-9 9. 12-8 7-10 10. 8-6 4-7 11. 11×4 10-13 12. 6-3 | wit wint | wit wint (10) |  |
| ✅ | 7.22 | 7. … 3-6 8. 13-11 7-10 9. 12-8 6-9 | wit wint | wit wint (17) |  |
| ✅ | 7.22 | 7. … 2-5 | wit wint | wit wint (21) | *wit kan op twee mooie manieren winnen* |
| ✅ | 7.22 | 7. … 7-10 8. 13-11 4-7 9. 11×4 10-13 10. 4-1 13-16 11. 12-9 16-13 12. 14-12 3-6 13. 1-4 13-16 14. 4-7 16-13 15. 18-15 2-5!! 16. 7×2 13-10 | remise | remise | *de hoofdvariant, diagram 7.23*<br>zet 6 (wit) 4-1 geeft de winst weg (remise) (behoudend: 12-8) |
| ✅ | 7.22 | 7. … 7-10 8. 13-11 4-7 9. 11×4 10-13 10. 12-8! 13-16 11. 14-11 2-5 12. 8×1 16×6 13. 18-14 | wit wint | wit wint (32) | *database: 10. 12-8! in plaats van 10. 4-1* |
| ❌ | 7.1 | 3. 16-13 5-8 4. 14-11 1-5 5. 11×6 9×2 6. 15-12 5-7 7. 17-14 7-10 | remise | wit wint (43) | *kansrijkste spel voor wit, beste verdediging voor zwart* |
| ✏️ | 7.1 | 3. 16-13 5-7 4. 14-11 1-5 | – | – | *tekst sectie 7.7: "3. … 5-7 4. 14-11 1-5"*<br>ONMOGELIJK: zet 4 (zwart) 1-5 is niet legaal in B:W11,13,15,17,18:B1,3,4,7,9; legale zetten: 7x14 |

#### Sectie 8

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ⚠️ | 7.1 | 3. 16-13 5-8 4. 14-11 1-5 5. 11×6 3×8 6. 17-14 9-12 7. 14×9 5-7 8. 9-6 8×3 9. 13-10 7-11 10. 15-12 3-6 11. 12-8 11-13 12. 8×3 13-17 | remise | remise | *diagram 8.1*<br>zet 10 (zwart) 5-7: verliest; behoudend waren: 4-7<br>zet 7 (wit) 17-14 geeft de winst weg (remise) (behoudend: 18-14)<br>zet 13 (wit) 13-10 geeft de winst weg (remise) (behoudend: 13-11) |
| ✅ | 8.1 | 1. 18-15 17-9 2. 3-8 9-12 | remise | remise | *wit komt niet verder* |
| ✅ | 8.1 | 1. 18-15 17-9 2. 3-8 9-17? 3. 8-11 | wit wint | wit wint (14) | *9-17? 8-11 W+*<br>bevestigd: 9-17? verliest, terecht als fout gemarkeerd (beter: 9-12) |
| ✏️ | 8.1 | 1. 3-8 17-9 2. 8-16 9-2 3. 3-11 | – | – | *tekst: 3. 3-11*<br>ONMOGELIJK: zet 5 (wit) 3-11 is niet legaal in W:W10,K16,18:BK2,4; legale zetten: 10-7, 16-11, 16-8, 16-3, 18-14, 18-15, 16-13, 16-6 |
| ✅ | 8.1 | 1. 3-8 17-9 2. 8-16 9-2 3. 16-11 2-9 4. 10-7 9-17 5. 7-5 17-10! 6. 5-2 4-7 7. 11×4 10-13 | remise | remise | *gecorrigeerd: 3. 16-11* |
| ✅ | 8.1 | 1. 3-8 17-9 2. 8-16 9-17 | remise | remise | *9-17 is ook remise* |
| ✅ | 8.2 | 1. … 16-8 2. 14-11 8×16 3. 10-7 4×11 4. 17-13 11-14 5. 18×11 | wit wint | wit wint (0) | *Manoury* |
| ✅ | 8.2 | 1. … 16-6 2. 14-11 6×16 3. 10-7 4×11 4. 17-13 11-14 5. 18×11 | wit wint | wit wint (0) | *(of 6)* |
| ✅ | 8.2 | 1. … 16-3 2. 14-11 3×16 3. 10-7 4×11 4. 17-13 11-14 5. 18×11 | wit wint | wit wint (0) | *(of 3)* |
| ✅ | 8.3 | 1. … 16-3 2. 10-7 4×11 3. 9-6 | wit wint | wit wint (4) |  |
| ✅ | 8.3 | 1. … 16-11 2. 17-14 | wit wint | wit wint (18) | *16-11 17-14, 9-6* |
| ✅ | 8.3 | 1. 15-1 | wit wint | wit wint (22) | *wit aan zet speelt 15-1 met winst* |
| ✅ | 8.4b | 1. 10-7 4×11 2. 17-13 | wit wint | wit wint (2) | *Manoury* |
| ✅ | 8.5 | 1. 2-7 4×11 2. 9-6 | wit wint | wit wint (4) |  |
| ✅ | 8.5 | 1. … 3-11 2. 17-14 | wit wint | wit wint (18) |  |
| ✅ | 8.5 | 1. … 3-16 2. 2-5 | wit wint | wit wint (20) |  |
| ✅ | 8.6 | 1. … 3-8 2. 17-12 | wit wint | wit wint (24) |  |
| ✅ | 8.6 | 1. … 3-11 2. 18-14 | wit wint | wit wint (20) |  |
| ✅ | 8.6 | 1. … 3-16 2. 2-7 4×11 3. 17-13 | wit wint | wit wint (2) |  |
| ✅ | 8.6 | 1. 2-5 | wit wint | wit wint (22) | *als wit aan zet is speelt hij 2-5* |

#### Sectie 9.1

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✅ | 9.1 | 1. 2-10? 9-6 | wit wint niet | remise | *zwart verovert de lange lijn*<br>bevestigd: 2-10? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 2-7) |
| ✏️ | 9.1 | 1. 8-5? 9-17 2. 5-2 3-6 | wit wint niet | – | *tekst: 9-17 8-2 3-6, 17-13*<br>ONMOGELIJK: zet 3 (wit) 5-2 is niet legaal in W:WK2,5,15:B3,K17; legale zetten: 2-9, 5-1, 15-12, 2-6<br>bevestigd: 8-5? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 2-7) |
| ✅ | 9.1 | 1. 8-5? 9-17 | wit wint niet | remise | *zwart verovert de lange lijn*<br>bevestigd: 8-5? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 2-7) |
| ✅ | 9.1 | 1. 2-7 9-6 2. 7-11 6-9? 3. 8-5 | wit wint | wit wint (32) | *wint snel*<br>bevestigd: 6-9?: verliest sneller (verlies in 34 i.p.v. verlies in 46) |
| ✅ | 9.1 | 1. 2-7 9-2? 2. 7-11 | wit wint | wit wint (34) | *diagram 9.2*<br>bevestigd: 9-2?: verliest sneller (verlies in 36 i.p.v. verlies in 48) |
| ✅ | 9.1 | 1. 2-7 9-17? 2. 7-11 | wit wint | wit wint (34) | *diagram 9.3*<br>bevestigd: 9-17?: verliest sneller (verlies in 36 i.p.v. verlies in 48) |
| ✅ | 9.4 | 1. 10-13 9-2 2. 13-11 | wit wint | wit wint (34) |  |
| ✅ | 9.4 | 1. 10-13 9-17 2. 13-11 | wit wint | wit wint (34) |  |
| ✅ | 9.5 | 1. 9-14 10-2 2. 14-11 | wit wint | wit wint (34) |  |
| ✅ | 9.5 | 1. 9-14 10-17 2. 14-11 | wit wint | wit wint (34) |  |
| ℹ️ | 9.6 | 1. 2-9 17-13 | – | wit wint (5) | *tekst: "2-9 17-13 met 9.5"* |
| ✅ | 9.6 | 1. 2-9 17-10 | wit wint | wit wint (37) | *naar diagram 9.5* |
| ✅ | 9.1 | 1. 2-7 9-6 2. 7-11 6-2 3. 11-16? 2-7 4. 15-12 7-4 | remise | remise | *er is niets te doen tegen 4-1*<br>bevestigd: 11-16? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 11-13, 11-4, 11-14, 11-18) |
| ✅ | 9.1 | 1. 2-7 9-6 2. 7-11 6-2 3. 11-13 2-7? 4. 15-12 7-18 5. 13-10 18-15 6. 10-2 | wit wint | wit wint (30) | *diagram 9.7*<br>bevestigd: 2-7?: verliest sneller (verlies in 36 i.p.v. verlies in 44) |
| ✅ | 9.7 | 6. … 15-18 7. 2-5 18-15 8. 12-9 15-18 9. 5-1 18-7 10. 1-4 7-10 11. 4-18 10-2 12. 18-14 2-10 13. 9-6 10-2 14. 14-9 2-7 15. 6-2 | wit wint | wit wint (6) | *routinematig* |
| ✏️ | 9.7 | 1. 2-9 18-15 | – | – | *tekst bij 9.7 met wit aan zet*<br>ONMOGELIJK: zet 2 (zwart) 18-15 is niet legaal in B:W8,K9,12:B3,K15; legale zetten: 15-18, 3-6 |

#### Sectie 9.2

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✏️ | 7.1 | 3. 16-13 5-8 4. 13-10 8-11 5. 14×7 4×11 6. 15-12 9×14 7. 17×12 1-4 8. 18-15 4-7 9. 10×5 10-13 | – | – | *tekst: 9. … 10-13*<br>ONMOGELIJK: zet 14 (zwart) 10-13 is niet legaal in B:W5,12,15:B3,11; legale zetten: 11-13, 3-6, 11-14<br>zet 3 (wit) 13-10 geeft de winst weg (remise) (behoudend: 14-11)<br>zet 10 (zwart) 1-4 verliest (behoudend: 3-6) |
| ⚠️ | 7.1 | 3. 16-13 5-8 4. 13-10 8-11 5. 14×7 4×11 6. 15-12 9×14 7. 17×12 1-4 8. 18-15 4-7 9. 10×5 11-13 10. 12-8 13-17 11. 5-2 17-9 | wit wint | wit wint (49) | *gecorrigeerd: 9. … 11-13, diagram 9.1*<br>zet 3 (wit) 13-10: geeft de winst weg (remise); behoudend waren: 14-11<br>zet 10 (zwart) 1-4 verliest (behoudend: 3-6) |
| ✅ | 9.1 | 12. 2-5 9-6 | remise | remise | zet 1 (wit) 2-5 geeft de winst weg (remise) (behoudend: 2-7) |
| ✅ | 9.1 | 12. 2-10 9-6 | remise | remise | zet 1 (wit) 2-10 geeft de winst weg (remise) (behoudend: 2-7) |
| ✏️ | 9.1 | 12. 8-5 9-17 13. 5-2 3-6 | remise | – | ONMOGELIJK: zet 3 (wit) 5-2 is niet legaal in W:WK2,5,15:B3,K17; legale zetten: 2-9, 5-1, 15-12, 2-6<br>zet 1 (wit) 8-5 geeft de winst weg (remise) (behoudend: 2-7) |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-9 14. 8-5 | wit wint | wit wint (32) |  |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-16 2-7 15. 15-12 7-4 | remise | remise | zet 5 (wit) 11-16 geeft de winst weg (remise) (behoudend: 11-13, 11-4, 11-14, 11-18) |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-13 2-7 15. 15-12 7-18 16. 13-10 | wit wint | wit wint (32) |  |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-13 2-7 15. 15-12 7-4 16. 13-10 | wit wint | wit wint (32) |  |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-13 2-10 15. 13-16 10-7 | remise | remise | zet 7 (wit) 13-16 geeft de winst weg (remise) (behoudend: 13-17, 13-11) |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-13 2-10 15. 13-17 10-7 16. 15-12 7-4 17. 17-10 | wit wint | wit wint (32) |  |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-13 2-10 15. 13-17 10-7 16. 15-12 7-18 17. 17-10 | wit wint | wit wint (32) |  |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-13 2-10 15. 13-17 10-2 16. 17-10 2-6 | remise | remise | zet 9 (wit) 17-10 geeft de winst weg (remise) (behoudend: 17-9, 17-13, 17-14) |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-13 2-10 15. 13-17 10-2 16. 17-12 2-6 | remise | remise | zet 9 (wit) 17-12 geeft de winst weg (remise) (behoudend: 17-9, 17-13, 17-14) |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-13 2-10 15. 13-17 10-2 16. 15-12 2-6 | remise | remise | zet 9 (wit) 15-12 geeft de winst weg (remise) (behoudend: 17-9, 17-13, 17-14) |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-13 2-10 15. 13-17 10-2 16. 17-9 2-10 17. 9-14 | wit wint | wit wint (36) |  |
| ✅ | 9.1 | 12. 2-7 9-6 13. 7-11 6-2 14. 11-13 2-10 15. 13-17 10-2 16. 17-9 2-7 17. 9-2 | wit wint | wit wint (38) | *diagram 9.8* |

#### Sectie 9.3

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ⚠️ | 9.8 | 17. … 7-14 18. 2-10[enige] | wit wint | wit wint (34) | *de enige winnende zet*<br>zet 2 2-10 (winst in 35) "de enige": ook 2-9 (winst in 41), 2-6 (winst in 43), 2-5 (winst in 45) behoudt/behouden het resultaat |
| ✅ | 9.8 | 17. … 7-18 18. 15-12? 18-15 | remise | remise | *verbazend genoeg remise*<br>bevestigd: 15-12? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 2-5, 2-9, 2-6, 2-10) |
| ✅ | 9.8 | 17. … 7-18 18. 2-5 18-14 19. 5-10 | wit wint | wit wint (34) | *de snelste winst* |
| ✅ | 9.8 | 17. … 7-18 18. 2-5 18-4 19. 15-12 | wit wint | wit wint (28) | *op 18-4 wint 15-12 wel* |
| ✅ | 9.8 | 17. … 7-18 18. 2-10 18-14 19. 10-5 14-17 20. 5-7 17-9 21. 7-2 9-14 22. 2-10 | wit wint | wit wint (34) | *ook 18. 2-10 is winnend* |
| ✅ | 9.8 | 17. … 7-4 18. 15-12? 4-1 | remise | remise | bevestigd: 15-12? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 2-5, 2-9, 2-6, 2-10) |
| ✅ | 9.8 | 17. … 7-4 18. 2-5 4-14 19. 5-10 | wit wint | wit wint (34) |  |
| ✅ | 9.8 | 17. … 7-4 18. 2-5 4-18 19. 15-12 | wit wint | wit wint (28) |  |
| ✅ | 9.8 | 17. … 7-4 18. 2-10 | wit wint | wit wint (44) | *ook 18. 2-10 is winnend* |

#### Sectie 9.4

| | vanaf | variant | boek | database | toelichting |
|---|---|---|---|---|---|
| ✏️ | 9.12 | 19. … 14-17 20. 8-5 3-6! 21. 15-12! 17×9 22. 13-17 | – | – | *tekst: 22. 13-17*<br>ONMOGELIJK: zet 6 (wit) 13-17 is niet legaal in W:W5,K10:B6,K9; legale zetten: 10-17, 5-1, 5-2, 10-13, 10-7 |
| ✅ | 9.12 | 19. … 14-17 20. 8-5 3-6 21. 15-12 17×9 22. 10-17 | wit wint | wit wint (4) | *gecorrigeerd: 22. 10-17 (slotstand)* |
| ✅ | 9.12 | 19. … 14-4 20. 15-12 | wit wint | wit wint (32) |  |
| ✅ | 9.12 | 19. … 14-18 20. 15-12 | wit wint | wit wint (32) |  |
| ✅ | 9.12 | 19. … 14-9 20. 10-13 | wit wint | wit wint (32) |  |
| ✅ | 9.12 | 19. … 14-12 20. 10-13 | wit wint | wit wint (6) |  |
| ✅ | 9.12 | 19. … 14-12 20. 8-6 | wit wint | wit wint (8) |  |
| ⚠️ | 9.12 | 19. … 14-17 20. 8-5[enige] | wit wint | wit wint (30) | *wit moet wel verder*<br>zet 2 8-5 (winst in 31) "de enige": ook 10-7 (winst in 43), 10-5 (winst in 43), 10-2 (winst in 51) behoudt/behouden het resultaat |
| ✅ | 9.12 | 19. … 14-17 20. 8-5 3-6 21. 5-2? 6-8 | remise | remise | bevestigd: 5-2? geeft de winst weg (remise), terecht als fout gemarkeerd (beter: 15-12) |
| ✅ | 9.12 | 19. … 14-17 20. 8-5 3-6 21. 15-12[enige] | wit wint | wit wint (6) | bevestigd: 15-12 is inderdaad de enige behoudende zet |

Database-statistiek: `built 9 slices 11394573 positions 7342710904 extra slices 0 positions 0 tt 1371 nodes 1690`
