# Projekat
Radili Jelena Karan (EE48/2018) i Marko Milović (EE31/2018).

## UVOD

Cilj ovog projekta je kreiranje sistema za praćenje temperature u automobilu, korišćenjem freeRTOS sistema.

## TEKST ZADATKA

Napisati softver za simulaciju sistema za merenje temperature u automobilu. Napraviti minimum 4 taska, jedan task za prijem podataka od PC-ja, jedan task za slanje podataka PC-iju, drugi za merenje temperature, i treci za prikaz na displeju. Sihronizaciju izmedju tajmera i taskova, kao i izmedju taskova ako je potrebno, realizovati pomocu semafora (ili task notifikacija) ili mutexa, zavisno od potrebe. Podatke izmedju taskova slati preko redova (queue).
Trenutne vrednosti temperature simulirati pomoću UniCom, simulatora “serijske” komunikacije. Računati da se informacije o trenutnoj temperaturi dobijaju preko UniCom softvera svake sekunde na kanalu 0. Temperatura se dobija kao vrednost otpornosti, maksimalne vrednosti 32 oma. Komunikaciju sa PC-ijem ostvariti isto preko simulatora serijske veze, ali na kanalu 1. Za simulaciju displeja koristiti Seg7Mux, a za simulaciju logičih ulaza i izlaza koristiti LED_bar.

1.	Pratiti temperaturu. Posmatrati  vrednosti koje se dobijaju iz UniCom softvera  kao vrednosti otpornosti, koje imaju opseg od 0 do 32 oma. Uzimati zadnjih 5 ocitavanja i usrednjavati ih. Ne vršiti nikakve proračune u interapt funkciji, samo proslediti podatke tasku za merenje temperature. Treba da se primaju 1 podatak za unutrasnju temperaturu (u kabini) i 1 podatak za spoljasnju temperaturu.
2.	Podesavanje min i maksimalne vrednosti za temperaturu, tj kalibrisati vrednosti otpornosti za ta dva stanja temperature. Procedura: Poslati preko PC karaktere  MINTEMP  a onda maksimalnu vrednost otpornosti (koji mora biti manji od 32 oma), ili karaktere MAXTEMP a onda minimalnu vrednost otpornosti (koja mora biti veci od 0 oma) .  Preračunati vrednosti koje se dobijaju iz vrednosti otpornosti (izmedju MINTEMP I MAXTEMP) tako da se dobije vrednost temperature u stepenima celzijusa. Racunati da je karakteristika priblizno linearna.

3.	Realizovati komunikaciju sa simuliranim sistemom. Slati naredbe preko simulirane serijske komunikacije. Naredbe i poruke koje se salju  preko serijske veze treba da sadrze samo ascii slova i brojeve, i trebaju se zavrsavati sa carriage return (CR),  tj brojem 13 (decimalno), čime se detektuje kraj poruke.  Naredbe su:
a. Automatsko obavestavanje o prevelikoj i premaloj temperaturi:
Zadati granicne temperature slicno kao u stavci 2 za THIGH i TLOW.
Ako temperatura ispadne iz opsega koji se zadaje ovom naredbom, da jedan ceo stubac u LED bar krene da blinka periodom od 500ms.
3. Na LCD displeju prikazati izmerene podatke, brzina osvezavanja podataka 100ms. Zavisno koji je “taster”  pritisnut na LED baru, prikazati trenutnu temperaturu i vrednost ocitanje otpornosti. A ako je pritisnut drugi taster, prikazivati spoljasnju temperaturu.

4.  Misra pravila. Kod koji vi budete pisali mora poštovati MISRA pravila, ako je to moguće (ako nije dodati komentar zašto to nije moguće). Nije potrebno ispravljati kod koji niste pisali, tj. sve one biblioteke FreeRTOSa i simulatora hardvera.

5. Obavezno je projekat postaviti na GitHub i na gitu je obavezno da ima barem jedan issue i pull request. Takođe obavezan je Readme.md fajl kao i .gitignore fajl sa odgovarajućim sadržajima. Navesti u Readme fajlu kako testirati projekat.

## PERIFERIJE

1. Led bar cine tri stupca, prva dva su diode, a treći je taster . Pozivanje se vrsi preko naredbe LED_bars_plus.exe BBb

2. Sedmosegmentni displej sadrzi 7 cifara, a poziva se naredbom Seg7_Mux.exe 5

3. Za serijsku komunikaciju neophodna su tri kanala cije se otvaranje vrsi pozivanjem sledecih naredbi: AdvUniCom.exe 0, AdvUniCom.exe 1 i AdvUniCom.exe 2

## OPIS TASKOVA

### SerialSend_Task0

Svakih 1000ms šalje se karakter "u" ka kanalu 0 i vrednost otpornosti se automatski ažurira.

### SerialReceive_Task0

Prihvata vrednosti otpornosti sa kanala 0 i šalje u redove.

### SerialReceiveTask_1

Uzima vrednosti minimalne i maksimalne temperature i otpornosti sa kanala 1, potrebnih za kalibraciju. 

### SerialSend_Task2

Svakih 1000ms šalje karakter "t" ka kanalu 2 i vrednosti za graničnu temperaturu se automatski ažuriraju.

### SerialReceiveTask_2

Prihvata vrednosti granične temperature, smešta ih u promenljive thigh i tlow, nakon čega ih šalje u red.

### prosecna_temp_un

Računavanje prosečne unutrašnje temperature na osnovu 5 uzastopnih merenja.

### prosecna_temp_sp

Računavanje prosečne spoljašnje temperature na osnovu 5 uzastopnih merenja.

### timer_seg7

Tajmerska funkcija koja se poziva svakih 100ms i proverava stanje tastera na led baru. Informacija sa led bara daje odgovarajući semafor kako bi se ta informacija kasnije obradila.

### mux_seg7_un

Vrednosti unutrašnje temperature i otpornosti se ispisuju na seg7_mux.

### mux_seg7_sp

Vrednosti spoljasnje temperature i otpornosti se ispisuju na seg7_mux.

### kalibracija

Kalibracija temperature na osnovu otpornosti dobijene sa kanala 0, i minimalnih i maksimalnih vrednosti temperature i otpornosti dobijenih sa kanala 1. U tasku je implementirana signalizacija ukoliko se kalibrisana temperatura ne nalazi u željenom opsegu, tako što se uključuje odgovarajući stubac led bara.

### main_demo

Funkcija u kojoj se vrši inicijalizacija svih periferija koje se koriste, kreiraju se taskovi, semafori, redovi i tajmer. Takođe se definiše interrupt za serijsku komunikaciju i poziva vTaskStartScheduler() funkcija potrebna za raspoređivanje taskova.

## NAČIN TESTIRANJA




