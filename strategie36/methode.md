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
  op 4 threads) en tot en met 9 stukken (7,3 miljard stellingen) worden vooraf gebouwd, met
  kleursymmetrie (180° draaien en de kleuren wisselen), en opgeslagen als één byte-array met een
  offsettabel.
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
