# **Architettura del Progetto ZPac:**  
**Ingegneria Retro-Computing e sviluppo AI-Orchestrated**

L'ingegneria del software su architetture retro-computing richiede un bilanciamento rigoroso tra l'ottimizzazione spinta delle risorse e la fedeltà comportamentale. Il progetto ZPac emerge come un'implementazione "white-room" di un iconico videogioco arcade degli anni '80, progettato specificamente per lo Zeal 8-bit Computer, un ecosistema hardware homebrew basato sul processore Zilog Z80.1 L'obiettivo fondamentale dell'opera non è una mera transcodifica (porting) del codice ROM originale, bensì una completa riscrittura basata sull'osservazione comportamentale e sulla documentazione pubblica.1 Questo approccio garantisce che ogni struttura dati, algoritmo di intelligenza artificiale e routine di rendering sia nativamente concepita per sfruttare e rispettare i vincoli della piattaforma di destinazione.1

Oltre alla valenza tecnica, il progetto si propone come un esperimento formale di un ecosistema di sviluppo all'avanguardia definito "AI-Orchestrated Development".1 L'intento è dimostrare la fattibilità della creazione di un software embedded complesso per una piattaforma di nicchia da parte di figure professionali prive di un background ultra-specialistico in Assembly Z80, delegando l'implementazione tecnica a un'intelligenza artificiale, a valle di una solida orchestrazione architetturale umana.1 L'esame del codice sorgente fornito e della documentazione architetturale rivela un prodotto di straordinaria maturità tecnica, in cui le limitazioni hardware vengono eluse attraverso pattern di programmazione avanzati e un uso enciclopedico delle specificità dello Zeal 8-bit OS e della Zeal Video Board (ZVB).

## **Sviluppo AI-Orchestrated: un'analisi metodologica**

L'industria dello sviluppo software sta assistendo a una transizione paradigmatica dall'uso dell'intelligenza artificiale come semplice strumento di autocompletamento (spesso associato al concetto informale di "vibe coding") verso veri e propri flussi di lavoro di "AI-Orchestrated Development".2 In quest'ultimo modello, l'AI gestisce l'implementazione del codice sotto una rigorosa supervisione umana, un approccio essenziale per mantenere la qualità, la sicurezza e la scalabilità del software.2 Il progetto ZPac codifica magistralmente questo flusso di lavoro simbiotico tra l'Architetto umano (il creatore del progetto, identificato come Azuech) e il Technical Driver basato su Intelligenza Artificiale (nello specifico, Claude di Anthropic).1

Questa dicotomia dei ruoli si rivela cruciale per aggirare i limiti intrinseci di entrambe le entità. L'operatore umano delega la difficoltà della manipolazione mnemonica di vasti set di registri hardware e la tediosità della sintassi del compilatore SDCC.1 Contestualmente, l'orchestrazione umana previene la tendenza dell'AI a perdere coesione strutturale su progetti a lungo termine o a proporre soluzioni non idonee per l'hardware specifico.1 Il feedback primario su questa metodologia è che essa rappresenta lo stato dell'arte per lo sviluppo assistito, superando i limiti del coding generativo non strutturato.

### **Dinamica dei ruoli e partizionamento delle responsabilità**

L'allocazione delle responsabilità segue un modello top-down rigoroso che si allinea perfettamente con le moderne teorie dei sistemi multi-agente in cui la specializzazione batte la generalizzazione.3 L'Architetto umano agisce come Project Manager e unico detentore della "big picture". Le sue responsabilità primarie includono la definizione della strategia di sviluppo suddivisa per fasi, l'impostazione delle priorità e l'identificazione precoce dei vicoli ciechi architetturali indotti dai vincoli hardware.1 Spetta unicamente all'Architetto la decisione di modificare l'allocazione degli asset in memoria o di transitare da una modalità grafica all'altra per soddisfare il "tile budget".1 Questo ruolo richiede una profonda comprensione dei vincoli di prodotto e del ciclo di vita del software, ma svincola il professionista dalla necessità di dominare il codice macchina.1

Il Technical Driver (Claude) esegue l'ingegneria di dettaglio. Analizza la manualistica dell'hardware, decodifica le specifiche del Software Development Kit (come lo ZVB SDK per grafica e audio) e modella le architetture software proposte.1 La sua responsabilità si estende alla stesura effettiva del codice sorgente C (conforme allo standard C89 per massimizzare la compatibilità con SDCC) e dell'eventuale Assembly Z80, nonché all'identificazione di alternative tecniche durante il debugging.1

### **Il ciclo di esecuzione e la generazione di prompt autoconclusivi**

Il flusso di lavoro iterativo documentato per ZPac è strutturato in cinque fasi sequenziali che garantiscono il controllo qualità e la convergenza verso gli obiettivi finali, una prassi che previene il degrado del codice tipico dei cicli di sviluppo AI prolungati 1:

1. **Analisi dello stato e definizione dei vincoli:** L'Architetto analizza lo stato della build corrente e impone parametri non negoziabili per lo sprint di sviluppo. Un esempio notevole di questa fase è l'imposizione di un accumulatore di velocità a 32 bit per garantire l'autenticità del movimento, respingendo scorciatoie algoritmiche (come il semplice salto di frame o "tick-skipping") che l'AI potrebbe proporre per semplificare il codice a discapito della fedeltà fisica.1  
2. **Discussione strategica:** Si valuta la fattibilità dell'approccio prima di scrivere alcuna riga di codice. Il Technical Driver propone opzioni architetturali valutando i trade-off, come il consumo di Video RAM rispetto ai cicli di clock della CPU.1  
3. **Generazione del prompt (incapsulamento del contesto):** Questa fase risolve uno dei problemi più complessi nello sviluppo assistito: la frammentazione del contesto. L'AI genera un prompt "autoconclusivo" (self-contained) che incorpora tutto lo scibile tecnico necessario per l'implementazione del task: frammenti di codice preesistente, macro di registri hardware, costanti matematiche e pattern di riferimento.1 Questo incapsulamento è vitale poiché l'esecuzione avverrà in un ambiente CLI separato, privo della memoria storica delle conversazioni precedenti.1  
4. **Implementazione in ambiente a carattere:** Il prompt autoconclusivo viene passato a "Claude Code", un agente a riga di comando con accesso diretto all'ambiente di sviluppo locale (Ubuntu 24.04 LTS).1 L'agente interagisce con il filesystem, scrive i moduli .c e .h, invoca il sistema di build (CMake) e risolve autonomamente gli inevitabili errori di compilazione e linking sollevati dal toolchain SDCC.1  
5. **Validazione empirica:** L'Architetto testa il binario risultante (.bin) sull'emulatore zeal-native.1 A differenza del software enterprise, la validazione di un clone arcade non si basa su unit test automatizzati, ma su feedback audiovisivo e comportamentale, un requisito umano essenziale per un'ingegneria del software incentrata sull'autenticità visiva e sonora.1

Il feedback su questo framework metodologico è estremamente positivo. L'isolamento dell'ideazione strategica (gestita tipicamente tramite interfaccia web) dall'implementazione pragmatica (gestita in ambiente terminale CLI) dimostra una comprensione avanzata di come orchestrare strumenti di intelligenza artificiale disparati. L'AI funge efficacemente da "compilatore intermedio", traducendo l'intento logico-architetturale umano in chiamate API hardware-specifiche e direttive di allocazione della memoria.1

## **Hardware e vincoli di piattaforma: Lo Zeal 8-bit Computer**

Per valutare adeguatamente il codice e le scelte di design di ZPac, è imprescindibile un'analisi delle restrizioni e delle capacità dell'hardware target. Lo Zeal 8-bit Computer è un ambiente di calcolo a 8 bit moderno, basato sul processore Zilog Z80 nella sua variante capace di operare a 10 MHz.4 A questa frequenza di clock, il processore è in grado di erogare un throughput di circa 1.45 MIPS (Million Instructions Per Second).5 Rispetto all'hardware arcade originale degli anni '80, che tipicamente impiegava processori Z80A cloccati a circa 3.072 MHz e disponeva di appena 2 KB di RAM di lavoro, lo Zeal offre un margine di calcolo significativamente superiore e un'abbondanza di memoria fisica pari a 512 KB di SRAM statica.1

Tuttavia, l'abbondanza di memoria fisica si scontra con il limite invalicabile dell'architettura Z80: uno spazio di indirizzamento logico a 16 bit, che consente alla CPU di "vedere" solo 64 KB di memoria in qualsiasi dato momento.4 Lo Zeal 8-bit Computer risolve questo problema hardware tramite una Memory Management Unit (MMU) a 22 bit 6, mappando banchi di memoria da 16 KB all'interno dello spazio di indirizzamento. Il sistema operativo nativo, Zeal 8-bit OS, riserva tipicamente i primi 16 KB per la ROM del kernel, lasciando all'utente uno spazio utile visibile di 48 KB per contenere il codice eseguibile, lo stack e le variabili globali del programma.1 Questo vincolo di 48 KB è il singolo parametro architetturale che ha influenzato maggiormente lo sviluppo di ZPac.

A completare l'ecosistema hardware interviene la Zeal Video Board (ZVB), una scheda video opzionale pilotata da un FPGA Lattice ECP5, che intercetta i segnali di bus del processore e genera un output grafico VGA a colori con supporto hardware per text mode, tilemap bitmap stratificati e sprite indipendenti.4 Le routine grafiche del progetto devono interfacciarsi con questa scheda attraverso un bus di I/O, scrivendo dati nei registri della VRAM senza sovraccaricare la CPU.

Il feedback architetturale sul progetto ZPac è che esso dimostra una consapevolezza eccezionale di questi colli di bottiglia hardware. Tutte le scelte di design discusse nelle sezioni seguenti (streaming I/O, formati fixed-point, deduplicazione grafica) derivano dalla necessità di eludere l'ostacolo dello spazio logico di 64 KB garantendo al contempo un rendering fluido a 60 frame per secondo.

## **Sottosistema Grafico: Transizione e Strutturazione in Mode 6**

L'implementazione grafica rappresenta l'area in cui le decisioni strategiche prese dall'Architetto umano hanno avuto il maggiore impatto sulla fattibilità del progetto. Il log di sviluppo evidenzia un intervento architetturale fondamentale: la transizione forzata dalla modalità video "Mode 5" alla "Mode 6" offerta dallo ZVB SDK.1

### **Il "Tile Budget" e le Complessità dello Scaling**

La riproduzione in alta fedeltà di un hardware concepito originariamente per monitor CRT con orientamento verticale (risoluzione 224x288 pixel) su uno standard display orizzontale moderno come il VGA pone severi problemi di proporzione e aspect ratio. Per mitigare le distorsioni visive e garantire una corretta leggibilità sul monitor VGA dello Zeal, l'Architetto ha imposto uno scaling lineare degli asset grafici originali con un fattore di 1.5x.1 In pratica, i tile logici originali del gioco arcade (griglie da 8x8 pixel) sono stati ingranditi per occupare matrici da 12x12 pixel, le quali vengono poi allineate e centrate all'interno del formato tile hardware standard a 16x16 pixel gestito dall'FPGA dello Zeal.1

Questo necessario fattore di scala ha generato una frammentazione geometrica che ha portato a un'esplosione del "Tile Budget", ovvero il conteggio dei tile grafici unici richiesti per ricostruire il labirinto, le intersezioni, la UI e i frame di animazione dei personaggi.1 Le proiezioni iniziali indicavano una necessità di ben 438 tile unici.1 Grazie allo sviluppo di una pipeline custom di deduplicazione in Python (utilizzando la libreria Pillow per l'analisi delle immagini), il team ha identificato simmetrie perfette nei quadranti degli sprite e riutilizzato porzioni grafiche, riducendo il fabbisogno a un valore altamente ottimizzato di 329 tile unici.1

Nonostante l'ottimizzazione, il budget di 329 tile si scontrava con le limitazioni fisiche del "Mode 5" della scheda video. Il Mode 5 opera a una risoluzione di 320x240 pixel con una profondità di colore a 8 bit per pixel (256 colori simultanei per singolo tile).1 Questa ricchezza cromatica richiede 256 byte di memoria VRAM per ogni singolo tile, ma soprattutto il controller FPGA in questa modalità supporta un massimo hardware di 256 tile contemporanei, un limite insufficiente per accogliere i 329 tile richiesti da ZPac.1 La persistenza sul Mode 5 avrebbe richiesto un downgrade visivo inaccettabile o compromessi catastrofici sulle animazioni.

### **I Benefici Architetturali del Mode 6**

L'Architetto ha conseguentemente guidato una massiccia refattorizzazione per migrare il motore grafico al **Mode 6**. Questa modalità video offre caratteristiche strutturalmente dissimili, che si sono rivelate provvidenziali. La tabella seguente illustra la comparazione tecnica tra le due modalità e le implicazioni per l'ingegneria del software di ZPac:

| Specifica Tecnica della Video Board | Mode 5 | Mode 6 | Impatto Architetturale sul Codice ZPac |
| :---- | :---- | :---- | :---- |
| **Risoluzione Attiva** | 320 x 240 pixel | 640 x 480 pixel | L'alta risoluzione permette di alloggiare l'intero labirinto scalato (una griglia da 21x24 tile all'interno di una matrice visibile di 40x30 tile) lasciando generoso spazio nero per gli elementi Head-Up Display (HUD).1 |
| **Profondità Colore e Dimensione Dati** | 8 bit per pixel (256 byte/tile) | 4 bit per pixel (128 byte/tile) | Raddoppia la densità di informazione per byte (due pixel in ogni byte). Questo dimezza istantaneamente l'occupazione VRAM dei tile, abbassando il peso totale del tileset da circa 84 KB a 43 KB.1 |
| **Budget Massimo Tile Hardware** | 256 tile | 512 tile | Innalza il limite hardware oltre la soglia necessaria, accogliendo comodamente i 329 tile attuali e lasciando 171 slot vuoti per l'implementazione futura delle cutscene.1 |
| **Gestione delle Palette Hardware** | Singola palette globale a 256 colori | 16 Gruppi Palette indipendenti (da 16 colori ciascuno) | Funzionalità cruciale: abilita il ricoloramento a runtime. I fantasmi e i loro stati ("vulnerabile", "occhi") possono condividere gli stessi dati tile in VRAM; il motore grafico altera semplicemente l'indice della palette nel byte di attributo per cambiare il colore, risparmiando massicce quantità di spazio VRAM.1 |
| **Flipping Grafico Spaziale** | Non nativamente ottimizzato | Supporto flag X/Y (SPRITE\_FLIP\_X, SPRITE\_FLIP\_Y) | Permette al motore di riutilizzare i frame di animazione specchiandoli orizzontalmente o verticalmente direttamente nel controller video FPGA, senza richiedere copie duplicate dei tile bitmap.1 |

Il feedback su questa decisione strategica è di totale encomio. Abbandonare la profondità di colore a 8 bit (totalmente superflua per un gioco arcade del 1980 che vanta sprite con massimo 2 o 3 toni) in favore dello spazio di immagazzinamento a 4 bit del Mode 6 dimostra un "design thinking" brillante, plasmato attorno alla realtà dell'hardware FPGA.

### **Renderizzazione Dinamica degli Sprite Compositi e del Loop Grafico**

Nel contesto della gestione visiva implementata in main.c e game\_loop.c, si osserva come l'intero layout venga gestito tramite layer hardware. La funzione gfx\_initialize viene chiamata con la macro SDK ZVB\_CTRL\_VID\_MODE\_GFX\_640\_4BIT.1 L'infrastruttura di Zeal prevede una distinzione tra sfondi statici (tilemaps) ed entità in movimento libero (hardware sprites).

Le entità attive (il protagonista Pac-Man e i quattro fantasmi avversari) non sono mappate nel background layer. A causa dello scaling applicato, un personaggio necessita di una canvas di 24x24 pixel. Dato che l'hardware dello Zeal gestisce nativamente solo sprite fisici di dimensioni fisse a 16x16 pixel, ZPac introduce una logica software per assemblare "Sprite Compositi 2x2".1

Come si evince dall'analisi di game\_loop.c, la routine update\_zpac\_sprites() orchestra la renderizzazione di quattro sprite hardware adiacenti per formare una singola identità visiva di 32x32 pixel.1 Questa architettura sottrae 20 sprite fisici ai 128 resi disponibili dall'FPGA per gestire simultaneamente le cinque entità di gioco.1

La complessità sorge nell'animazione direzionale. La macchina a stati dell'animazione utilizza un ciclo fluido in 4 fasi (Wide → Medium → Closed → Medium), mappato da un array anim\_phase\_map basato sui tick del game loop (zpac.anim\_tick).1 Invece di conservare in memoria i tile pre-ruotati per le direzioni Sinistra e Su, il motore legge la variabile di stato zpac.dir\_current. Se la direzione richiesta è DIR\_LEFT, il software utilizza il set di tile di base (destra) e invia l'attributo SPRITE\_FLIP\_X alla scheda video.1 Questa operazione di specchiatura di uno sprite composito impone la necessità di riordinare matematicamente l'indice dei quattro quadranti hardware per evitare che la porzione frontale del personaggio finisca sul lato sbagliato dello schermo, un calcolo gestito in tempo reale dalle routine in game\_loop.c.1

## **Architettura della Memoria e Streaming I/O** 

Come introdotto in precedenza, l'ingombro logico di 48 KB visibili allo Z80 pone un grave rischio per la fattibilità dell'eseguibile, che deve includere logica di gioco, librerie SDK, stack e asset. Il file main.c documenta l'implementazione di una pipeline di caricamento avanzata e asincrona che risolve radicalmente questo problema di "Out of Memory".1

### **Streaming in Chunk dei Dati Binari**

La pipeline grafica di ZPac genera un file di asset binari (zpac\_tiles.bin) la cui dimensione si attesta sui 49 KB.1 Includere staticamente questo array nel codice sorgente C causerebbe il fallimento immediato in fase di linking, in quanto supererebbe lo spazio di RAM allocabile da ZealOS.

La funzione load\_tileset\_from\_file() in main.c introduce un paradigma di I/O streaming.1 All'avvio del programma, il motore disabilita il rendering grafico (gfx\_enable\_screen(0)) e richiede al virtual file system di ZealOS (zos\_vfs.h) di aprire il file degli asset.1 Il file può risiedere nel filesystem locale della scheda SD oppure essere servito dal computer di sviluppo tramite il protocollo HostFS (mappato come disco H:/), uno strumento vitale per lo sviluppo iterativo rapido sull'emulatore zeal-native senza dover costantemente trasferire file su supporto fisico.1

La lettura avviene in "Pass 1": il file viene assorbito in blocchi di 512 byte (chunking). Ogni chunk, appena allocato in un buffer temporaneo in RAM, viene immediatamente trasmesso alla Video RAM della ZVB tramite la funzione SDK gfx\_tileset\_load e il buffer viene sovrascritto dal chunk successivo.1 In questo modo, l'ingombro di picco in memoria centrale causato dagli asset grafici è di soli 512 byte, trasferendo l'intero onere di stoccaggio dei 49 KB sulla RAM dedicata della scheda video FPGA.1 Questo trucco ingegneristico mantiene il binario compilato al sicuro, con una dimensione agile di circa \~45 KB.1

### **Generazione Procedurale in VRAM e Sostituzione "No-Dot" Differita**

Nel videogioco arcade, il giocatore percorre il labirinto consumando piccole "pillole" (dot). Su hardware a bassa potenza, la logica di rasterizzazione pone un dilemma prestazionale severo. Un tile in Mode 6 è composto da 128 byte, ciascuno contenente due pixel a 4 bit (nibble). Modificare l'aspetto di un singolo tile a runtime per "cancellare" una pillola imporrebbe alla CPU Z80 di leggere i 128 byte del tile dalla VRAM, eseguire mascherature bit a bit per azzerare i nibble corrispondenti al colore della pillola e riscrivere i byte in VRAM.1 L'invocazione di cicli di lettura/scrittura così intensi sul bus I/O della video board durante il game loop violerebbe sistematicamente la finestra di timing dei 60 FPS.

La genialità della soluzione implementata in main.c risiede nel pre-calcolo (o "generazione procedurale differita") di queste variazioni durante l'inizializzazione statica, trasformando un collo di bottiglia computazionale in un semplice accesso di memoria in O(1).1

Durante il "Pass 2" di load\_tileset\_from\_file(), il sistema riesamina le intestazioni grafiche dei primi 178 tile, che compongono l'ambiente del labirinto.1 La routine tile\_has\_dots() sonda i byte per intercettare il valore 0x1, assegnato in palette al pigmento delle pillole.1 Se un tile contiene tale pigmento, la funzione tile\_remove\_dots() genera una copia perfetta in un buffer di lavoro e sovrascrive i nibble 0x1 con il valore 0x0 (trasparenza o nero).1

Questa variante purificata viene quindi iniettata nella VRAM a una coordinata di index sfalsata, a partire dall'indice tile 341 (TOTAL\_TILES).1 A corollario, l'array globale tile\_nodot\_map traccia una corrispondenza diretta tra l'indice del tile originale (es. tile 15\) e il suo clone immacolato in fondo alla VRAM (es. tile 341).1 A runtime, quando il loop fisico rileva l'ingestione della pillola, non avviene alcuna alterazione bitmap; il programma altera meramente il puntatore a livello di tilemap (un'operazione singola su un singolo byte), invocando istantaneamente la visualizzazione del tile "No-Dot".1

Il feedback su questo blocco di codice in main.c è di eccellenza ingegneristica. Il compromesso tra utilizzo spinto della spaziosa VRAM e conservazione aggressiva dei cicli di esecuzione della limitata CPU Z80 è calibrato perfettamente per le idiosincrasie dell'ecosistema retro-computing.

## **Motore fisico e algoritmica di movimento:**  **L’accumulatore di Bresenham** 

L'ottenimento di un comportamento software "arcade-accurate" richiede un motore fisico in grado di replicare al decimo di secondo la cinematica del cabinato del 1980\.1 La sfida principale posta dallo Z80 risiede nel totale divieto di utilizzo di hardware in virgola mobile (FPU) e nelle scarse prestazioni relative all'esecuzione di divisioni numeriche software.1

Le specifiche tecniche originali ("Pac-Man Dossier") prescrivono che i personaggi viaggino a frazioni complesse di una velocità di base (ad esempio, l'80% in stato di caccia normale o il 75% in stato di vulnerabilità).1 Calcolare vettori sub-pixel iterando routine di divisione logica ostacolerebbe irreparabilmente il throughput di esecuzione, mentre arrotondare rozzamente le velocità all'intero più vicino genererebbe il fenomeno del "Jitter Problem" (vibrazione visiva asincrona e difetti d'interpolazione in corrispondenza del centro geometrico del tile).1

L'Architetto ha esplicitamente bloccato l'impiego di palliativi algoritmici moderni (come il "tick-skipping") imponendo al Technical Driver la trasposizione di un'autentica meccanica di scorrimento frazionario compatibile con i processori a 8 bit.1 La soluzione descritta in Bresenham\_accumulator.md e codificata in game\_loop.c applica l'algoritmo di accumulo dell'errore formulato da Jack Bresenham nel 1962, solitamente impiegato per la rasterizzazione di linee geometriche inclinate, declinandolo però sul tracciato cronologico dei vettori di movimento.1

L'algoritmo aggira il requisito della frazione allocando la grandezza dello spostamento come una progressione dell'errore intero che innesca uno step discreto unicamente all'evento di overflow.1 Nel motore ZPac, questa speculazione matematica si converte in un **Accumulatore Fixed-Point formato 8.8**.1

A livello strutturale, ogni personaggio alloca nel suo strato logico un registro senza segno a 16 bit denominato uint16\_t speed\_acc 1:

* L'**High Byte** (gli 8 bit superiori) contiene il quantitativo discreto (i pixel interi) da avanzare nel frame di refresh corrente.1  
* Il **Low Byte** (gli 8 bit inferiori) funge da cisterla per il gradiente dell'errore (la frazione residuale dello scorrimento).1

La calibratura dei ratei attinge a parametri costanti. Assumendo dal livello logico un valore di spinta precalcolato come 433 (che algebricamente decodifica il rapporto 433/256, equivalente a un avanzamento di circa 1.691 pixel a frame), il throughput su 60 Hz assicura al personaggio un moto a \~101 pixel al secondo, scartando con una discrepanza millimetrica (\< 0.5%) dal timing arcade autentico.1

La logica operativa esegue questo calcolo in game\_loop.c con una parsimonia di cicli clock eccezionale, servendosi solo di tre istruzioni logiche Z80 native 1:

1. **Addizione a 16-bit:** zpac.speed\_acc \+= pac\_speed; (Incremento dell'accumulatore con la costante calibrata).  
2. **Estrazione per Right-Shift:** steps \= (uint8\_t)(zpac.speed\_acc \>\> 8); (Scorrimento logico a destra di 8 bit che fa defluire i pixel di avanzamento intero, di solito 1 o occasionalmente 2, isolandoli nell'identificativo steps adibito a foraggiare il ciclo subordinato di controllo delle intersezioni).  
3. **Preservazione Residuale con Bitmask AND:** zpac.speed\_acc &= 0xFF; (Un'operazione logica AND annulla il byte superiore, assicurando la conservazione perpetua della porzione di errore accumulato senza ricorrere a sottrazioni o divisioni).

Il feedback per questa architettura matematica è eccellente. Risolve alla radice l'idiosincrasia della virgola mobile su hardware limitato, assicurando che lo smistamento dei pixel in eccesso nel tempo avvenga con perfetta equidistanza, elidendo ogni fenomeno di jitter visivo, una pietra miliare nello sviluppo homebrew.

## **Logica Ambientale e Constraint Matematici: Implementazione del Labirinto**

L'interazione tra i personaggi e il labirinto è interamente decodificata nel file maze\_logic.c. Qui si rileva l'utilizzo sapiente dell'approccio a "bitmask" e la segregazione dell'ambito logico da quello prettamente visivo.1

La mappa è orchestrata attorno all'array matrice uint8\_t maze\_logic, tracciante un reticolo in 31 righe e 28 colonne corrispondenti alle caselle spaziali di percorrenza.1 Le transizioni vettoriali fanno uso degli array statici dir\_dx e dir\_dy, i quali normalizzano i costrutti per il movimento nelle direttrici di base (Destra, Giù, Sinistra, Su).1

La matrice è instanziata in O(n) durante la fase procedurale dell'engine init\_maze\_logic(), la quale funge da interprete (parser) dell'array ASCII costante maze\_ascii fornito in hardcode.1 L'adozione del template visivo ASCII eleva massimamente la leggibilità e l'auditabilità del codice C rispetto a matrici binarie incomprensibili per l'uomo.

Il parser decodifica il dominio assegnando flag bit a bit ai settori del reticolo 1:

* **Caratteri di Percorrenza (. e o):** Oltre ad applicare il tag di casella transitabile con dot (CELL\_WD) o energizer (CELL\_WE), questa scansione inizializza per sommatoria iterativa la variabile globale dot\_count.1 Il gioco decreterà la pulizia del livello (level clear) unicamente all'azzeramento algoritmico di questa variabile.  
* **Costrutti Fisici (\# e ):** Instanziano i muri impenetrabili (CELL\_WALL) e le corridoie sgombre (CELL\_W).1

La fedeltà assoluta al coin-op del 1980 richiede l'implementazione di trigger invisibili ("Logic Overlays") che inibiscono l'AI ma esulano dalla conformazione grafica del livello. Questi overlay sono applicati dinamicamente mediante algebra booleana (Bitwise OR |=) al termine della scansione ASCII 1:

1. **Overlay di Tunnel (CELL\_TUNNEL):** Iniettati esplicitamente nelle colonne \[0-5\] e \[22-27\] della riga 14\.1 In sede di loop, all'intercettazione di questo flag, lo ZPac o i nemici subiscono un wrap-around cosmico, in base a un controllo algoritmico della coordinata X (se nx \< 0 wrap a 27; se nx \> 27 wrap a 0), permettendo la transizione topologica laterale dello schermo.1  
2. **Cella di Isolamento (CELL\_GHOST\_HOUSE):** Le matrici comprese tra riga \[13-15\] e colonna \[11-16\] ricevono questo vincolo, negato fisicamente alla locomozione di base del giocatore e adibito esclusivamente ai meccanismi di respawn spettrale e temporizzazione di uscita.1  
3. **Vettori di Esclusione Direzionale (CELL\_NO\_UP):** Il vincolo comportamentale più sottile e accurato. Nelle righe \[9-11\] e colonne \[12-15\] insistenti superiormente alla casa dei fantasmi, ogni casella già marcata come CELL\_WALKABLE viene integrata con il flag CELL\_NO\_UP.1 Questo riproduce una peculiare restrizione algoritmica storica per cui i fantasmi in queste esatte intersezioni adiacenti alla tana sono banditi dal poter scartare in direzione Nord, influenzando drasticamente i calcoli e le traiettorie dell'inseguimento.1

## **Macchina a Stati e Architettura del Loop di Esecuzione**

L'evoluzione dell'ambiente di gioco e dei vincoli contestuali è orchestrata dal costrutto game\_loop.c. A differenza di progetti accademici rudimentali, ZPac separa efficacemente l'update del display rasterizzato dalle decisioni temporali.1

L'applicazione globale è controllata dal parametro identificativo game\_state, integrato da registri di track come demo\_mode e il contatore inattivo demo\_frame\_count, che gestiscono in autonomia il timing per il trigger ciclico dell'Attract Mode dopo una latenza costante (DEMO\_DURATION di 3000 frame, circa 50 secondi a 60 fps).1

L'architettura della curva di difficoltà elude la parametrizzazione algebrica continua a favore di una stratificazione precompilata (Lookup Tiers), minimizzando i calcoli floating e proteggendo i cicli clock Z80.1 La routine update\_level\_speeds() estrapola e aggancia un riferimento struct (speed\_set\_t) desunto dall'array speed\_tiers categorizzato in quattro fasi di gioco globale: Tier 0 (L1), Tier 1 (L2-L4), Tier 2 (L5-L20) e Tier 3 (Massimale da L21 in poi).1 La granularità della struct disaccoppia con esattezza le costanti numeriche di scorrimento normali, accelerate e vulnerabili per ciascun archetipo di attore.

I trigger di vulnerabilità (le Power Pellet, o Energizer) rispondono analogamente a strutture non lineari. L'estrazione dell'arco cronologico dello stato vulnerabile attinge alla tabella fright\_dur\_table tramite indice intero. Delineando un drop vertiginoso dai 360 tick ammessi nel primo scenario fino ai micidiali zero tick allocati forzosamente ai livelli 17 e 19 e al superamento del livello 20\.1

Durante i momenti cruciali dell'applicativo (Eventi di Blocco) — ad esempio il completamento della scansione del labirinto — si evince un eccellente uso di routine asincrone temporizzate sull'hardware. La sequenza documentata da level\_complete\_sequence() sospende le letture I/O delegando le pause al blocco della sincronizzazione verticale hardware (gfx\_wait\_vblank), con soppressione immediata dei flag di renderizzazione fantasma tramite la proiezione fuori dal confine visibile raster (coordinata y \= 480 implementata da hide\_all\_ghosts()).1

Il rinomato lampeggiamento (Flashing) delle pareti post-completamento fornisce un'ulteriore masterclass nell'uso dell'architettura hardware 4-bit Mode 6\.1 In luogo del ridisegno costoso dei tilemap CELL\_WALL, l'engine produce un alias di memoria limitata della palette in uso (white\_pal), infettando forzatamente con il colore primario 0xFFFF gli slot d'indice del muro.1 Una rapida interpolazione circolare che sovrascrive a loop i banchi dei registri fisici della palette per 8 iterazioni (in uno span di 3 secondi) regala un feedback visivo immediato a consumo O(1).

Il feedback su quest'area del codice loda la perfetta aderenza a pratiche di robusta architettura C ANSI; si osserva l'uso rigoroso e sicuro di array costanti statici (static const uint16\_t) a discapito di cicli operativi massicci calcolati iterativamente.

## **Sottosistema Audio e Ingegneria del Programmable Sound Generator (PSG)**

L'analisi del file sound.c svela come l'ambiente acustico di ZPac aggiri in modo inventivo la mancanza di un coprocessore dedicato al mixaggio audio. Il firmware istruisce il chip sintetizzatore PSG (Programmable Sound Generator) integrato nel nucleo dell'FPGA servendosi degli header nativi dello ZVB SDK (zvb\_sound.h).1

### **Architettura di Salvaguardia del Bank Switching**

La limitata estensione di banda di comunicazione tra la CPU Z80 e il modulo video/audio espone il sistema al pericoloso conflitto del memory mapping I/O. L'allocazione topografica dei registri sonori siede logicamente sul **Bank 3** delle periferiche hardware della scheda, divergendo dal **Bank 0** requisito per l'ordinario invio di bitmap e rendering grafico.1 Impartire una sollecitazione diretta per modulare un inviluppo audio di background durante la scrittura ciclica del refresh del display equivarrebbe a iniettare rumore di entropia (glitch a schermo o suoni non corretti), dato il cortocircuito tra banchi hardware.1

L'ingegnerizzazione di sound.c disinnesca il potenziale di collasso incapsulando irrevocabilmente ogni comando I/O di natura acustica entro un protocollo di backup contestuale di esecuzione controllato dalle sub-routine snd\_bank\_save() e snd\_bank\_restore().1 Il flusso prescrive:

1. Isolamento logico dell'identificativo in corso di periferica, esfiltrato e decodificato da zvb\_config\_dev\_idx.  
2. Associazione temporanea della memoria tramite direttiva zvb\_map\_peripheral() sul settore acustico.  
3. Immissione di carico per sweep e forme d'onda.  
4. Ripristino coatto della segmentazione del banco precedente (solitamente video) prima che il processore riattivi il loop ciclico per evitare la perdita del context o del pointer di stack hardware.1

### **Allocazione Vocale e Dinamiche Waveform**

Il modulo delinea l'assegnazione rigida ma ottimizzata di tre oscillatori distinti (VOICE0, VOICE1 e VOICE2), prevenendo le dissonanze cacofoniche di sovrapposizione armonica.1 L'orchestrazione definisce gli archetipi parametrici dei timbri:

* **La Sirena Oscillante (VOICE0):** Operante con una modulazione a dente di sega o quadra. Esegue un andirivieni algoritmico in un delta esadecimale tra limiti 0x022E e 0x0575, processando uno scatto tonale di ampiezza 87 ad ogni refresh rate.1 Un periodo sinusoidale pieno si conclude matematicamente ogni 20 frame (circa 0.33 sec) esitando nel tipico battito "uauauaua" propulsivo del gioco.1 Tale suono diviene innesco d'allerta sostituito celermente nella modalità Fright da un rapido dente di sega ascendente che riposiziona i propri assi al valore di base 0x00D0 di 8 cicli in 8 cicli.1  
* **La Mandibola Waka (VOICE1):** Adotta nativamente l'indice di una quadra al 50% di Duty Cycle che l'autore indica come foriera di un'intrinseca pastosità "crunchy" sul processore FPGA Zeal. Si alterna programmaticamente su due matrici inverse (una di ascesa l'altra di caduta timbrica) temporizzate rigorosamente su lotti di 5 frame (eatdot1 e eatdot2) che spaziano da vertici a \~496 Hz fino alle profondità a \~164 Hz per dare volume ritmico al pasto.1  
* **Modulatori One-Shot (VOICE2):** Riservata alle instanze singolari, adotta curve incrementali aspre, quali il consumo fantasma (un incremento costante con origine 0x0030 fino ad un picco esasperato a 0x0C00 protraendosi per precisi 32 frame d'ascolto), assicurando la leggibilità audio essenziale al videogioco.1

### **Serializzazione e Ottimizzazione Acustica: Il Preludio**

Il culmine della compressione è toccato dalla cutscene d'apertura (il "Preludio"), uno stralcio acustico a doppia voce esteso su 245 tick di ciclo a 60 fps (4.08 s approssimativi).1 Data la penuria asfissiante della memoria residente Z80, un campionamento in virgola mobile per spartito è pura utopia teorica.

Lo sblocco avviene introducendo un paradigma denso Byte/Tick supportato da LUT (Look-Up Tables) incrociate. Un array statico, il prelude\_data, immagazzina i 245 istanti temporali collassati ad 1 byte per record.1 In esecuzione il sistema scinde logicamente l'identificatore in due semi-informazioni (nibble). L'**High Nibble** traccia il segnale direzionale su prelude\_v0\_lut per istruire il Synth del Basso (VOICE0), mentre il **Low Nibble** punta al corrispettivo indice in prelude\_v1\_lut per innescare l'armonia della Melodia (VOICE1) in un range espanso da \~340Hz a 1143Hz.1 Valori indicizzati allo Zero indicano il silenzio armonico.1

Questo artificio è esaltato dall'uso eccezionale di un tracciatore di pseudo-decadimento del volume (fading analogico). Lo script sfrutta la tabella prelude\_v0\_vol per interpretare 16 livelli arbitrari di decadenza naturale del volume all'interno del singolo colpo musicale del basso, decodificando in progressione questi identificativi nei quattro rudi layer di saturazione elettrica forniti nativamente dall'API ZVB (VOL\_75, VOL\_50, VOL\_25, VOL\_0) creando l'illusione psicacustica di una nota sfumata su hardware di tre decenni fa.1 Il feedback decreta la gestione della pipeline sonora come uno degli asset tecnologici di maggior prestigio e raffinatezza dell'intera codebase prodotta dall'AI.

## **Ingegneria della toolchain di build e approvvigionamento fonti**

Il processo di deployment del progetto su base CMakeLists.txt esegue il bridging tra architettura x86/ARM moderna a scopo sviluppo e il compilato grezzo indirizzato alla CPU Zilog Z80 dell'ambiente Zeal.1

Richiedendo CMake versione \>= 3.16, definisce rigorosamente sdcc (Small Device C Compiler) come toolchain proprietario principale.1 Selezionare l'SDCC è una strategia obbligatoria ma complessa in campo retro; questo cross-compilatore ANSI C genera internamente assembly ottimizzati Z80 agendo pesantemente sui registri.1 L'infrastruttura di linking però integra le librerie precompilate (come zvb\_gfx e zvb\_sound mappate con target "PUBLIC" da ZVB\_SDK) facendo ponte con l'integrazione di z88dk / z80asm per assicurare che direttive native macro, offset del sistema operativo ZealOS e routine assembly specializzate non sfaldino il costrutto.1 La macro personalizzata zos\_add\_outputs(zpac) sancisce la firma del file in conformità formale e strutturale (headers binari) idonea a innescare il caricamento sul sistema OS della piattaforma target.1

I sorgenti enumerati nel parametro add\_executable manifestano un disaccoppiamento dei domini superlativo 1: main.c (gestore del core e delegato alle inizializzazioni grafiche globali), maze\_logic.c (topologia), game\_loop.c (coordinamento di thread temporali), unit logiche per classi istanziabili (dots.c, ghost.c, fruit.c, sound.c).1 Si eleva la presenza del sorgente dedito level256.c, inserito programmaticamente per riproporre il famigerato Split Screen Bug (Kilcreen).

Quest'ultima considerazione apre il capitolo della fedeltà comportamentale documentato in CREDITS.md. Assicurando che non vi sia alcun furto o inclusione speculare di array ROM dell'originale Namco dell'epoca, l'opera trae l'algoritmica fondante dallo studio de "Il Dossier Pac-Man" (guida universalmente riconosciuta da Jamey Pittman inerente i pattern periscopici, target logico di deviazione, la matematica della follia "Cruise Elroy") incrociato allo scrutinio del port di natura C99 fornito da A. Weissflog.1 Le meccaniche tecniche relative all'intricato guasto del kilcreen al 256° labirinto attingono dalla vivisezione tecnica operata da D. Hodges per iniettare le medesime alterazioni del buffer in esadecimale e ottenere le stesse manifestazioni asettiche del collasso software.1 Tutte le calibrazioni di timing si sono misurate empiricamente e a span di frame per frame su ambienti di simulazione MAME per convergere alla latenza della macchina fisica con margine sub 0.5%.1

## **Conclusioni e implicazioni di sistema**

L'analisi esaustiva dell'architettura del software, della topologia logica e della metodologia di approvvigionamento di ZPac convalida la portata dell'opera oltre la pura sfera ludica. L'implementazione costituisce un artefatto ingegneristico pregevole, in cui l'ostilità di un ambiente di sviluppo Z80 gravato da limitazioni asfissianti di indirizzamento (48 KB di user space logico) a fronte di ambizioni audio/grafiche complesse (scaler a 1.5x, palette hardware, animazione multi-frame) viene sistematicamente neutralizzata attraverso l'impiego elegante delle peculiarità del firmware target (Zeal 8-bit Computer e ZVB SDK).

Il paradigma "AI-Orchestrated Development" promosso dal progetto in cui si affida a un'entità intelligente (Claude) l'esecuzione a macro di livello macchina, relegando al programmatore umano lo screening tattico a volo d'uccello e la validazione dei task, disinnesca il paradosso dei colli di bottiglia del programmatore generalista moderno. Mostra la via applicabile e vincente del nuovo approccio allo sviluppo "Spec-Driven Development": l'utente è un Project Architect che elabora l'infrastruttura (vincoli algoritmici precisi come l'imposizione di un'architettura accumulator-based per debellare le approssimazioni di velocità), mentre l'Intelligenza Artificiale opera i calcoli crudi e tesse i legami delle librerie C SDK, integrando la sintassi di destinazione su un livello introvabile in uno skill set generico odierno.

L'ingegnosità matematica e gestionale si palesa tangibilmente nella pre-renderizzazione in I/O "No-Dot Map", nell'utilizzo delle lookup tables acustiche incrociate di prelude\_data per simulare polifonie evolute e fade temporizzati su sensori hardware essenziali, e soprattutto nella stesura algoritmica di Bresenham applicato alla cinetica, permettendo una replicazione simulata della frazione dei float senza l'oneroso dispendio dei cicli macchina che un'approssimazione al decimale avrebbe inflitto allo Z80. In ultima analisi, il codice di ZPac assurge a manifesto di best-practice, offrendo sia un paradigma evoluto nello sviluppo software AI assistito sia un test eccellente delle capacità del nascente ecosistema Zeal 8-bit.

#### **Bibliografia**

1. READMEv2.md  
2. Transforming software development: The Vibe Coding approach every CIO needs to know, accesso eseguito il giorno marzo 13, 2026, [https://our-thinking.nashtechglobal.com/insights/transforming-software-development-the-vibe-coding-approach-every-cio-needs-to-know](https://our-thinking.nashtechglobal.com/insights/transforming-software-development-the-vibe-coding-approach-every-cio-needs-to-know)  
3. How I Built a Multi-Agent AI System That Changed My Development Workflow Forever | by Vedantparmarsingh | Medium, accesso eseguito il giorno marzo 13, 2026, [https://medium.com/@vedantparmarsingh/how-i-built-a-multi-agent-ai-system-that-changed-my-development-workflow-forever-2fede7780d0f](https://medium.com/@vedantparmarsingh/how-i-built-a-multi-agent-ai-system-that-changed-my-development-workflow-forever-2fede7780d0f)  
4. Zeal Documentation \- Specifications \- Zeal 8-bit Computer, accesso eseguito il giorno marzo 13, 2026, [https://zeal8bit.com/docs/specifications/](https://zeal8bit.com/docs/specifications/)  
5. Zeal Documentation \- Zilog Z80 Processor \- Zeal 8-bit Computer, accesso eseguito il giorno marzo 13, 2026, [https://zeal8bit.com/docs/cpu/](https://zeal8bit.com/docs/cpu/)  
6. The Zeal 8-Bit Computer Complete Edition Is Everything You Need for Some Retro Zilog Z80 Fun \- Hackster.io, accesso eseguito il giorno marzo 13, 2026, [https://www.hackster.io/news/the-zeal-8-bit-computer-complete-edition-is-everything-you-need-for-some-retro-zilog-z80-fun-3f299c2fba40](https://www.hackster.io/news/the-zeal-8-bit-computer-complete-edition-is-everything-you-need-for-some-retro-zilog-z80-fun-3f299c2fba40)  
7. Getting started with Zeal Video Board \- Zeal 8-bit Computer, accesso eseguito il giorno marzo 13, 2026, [https://zeal8bit.com/getting-started-zvb/](https://zeal8bit.com/getting-started-zvb/)  
8. SDCC Compiler User Guide, accesso eseguito il giorno marzo 13, 2026, [https://sdcc.sourceforge.net/doc/sdccman.pdf](https://sdcc.sourceforge.net/doc/sdccman.pdf)  
9. Why do C to Z80 compilers produce poor code? \- Retrocomputing Stack Exchange, accesso eseguito il giorno marzo 13, 2026, [https://retrocomputing.stackexchange.com/questions/6095/why-do-c-to-z80-compilers-produce-poor-code](https://retrocomputing.stackexchange.com/questions/6095/why-do-c-to-z80-compilers-produce-poor-code)  
10. Compile options to optimize program size? \- z88dk forums, accesso eseguito il giorno marzo 13, 2026, [https://www.z88dk.org/forum/viewtopic.php?t=9159](https://www.z88dk.org/forum/viewtopic.php?t=9159)
