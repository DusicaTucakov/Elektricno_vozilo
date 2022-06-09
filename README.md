# Procena autonomije električnog vozila 
## Uvod
Zadatak ovog projekta je da simulira sistem za procenu autonomije električnog vozila. Kod je napisan po MISRA standardu u okruženju *VisualStudio2022*.
## Zadaci
1. Praćenje stanja baterije. Trenutna vrednost baterije se dobija svaki sekund sa kanala 0 serijske komunikacije u opsegu od 0 do 5000mV.
2. Uzimati zadnja tri očitavanja vrednosti baterije i usrednjavati ih.
3. Podesiti maksimalnu (4900mV) i minimalnu (10mV) vrednost za nivo baterije tj. kalibrisati vrednost napona za ta dva stanja nivoa baterije. Ove vrednosti se šalju preko kanala 1 serijske komunikacije.
4. Izračunati vrednost napona u procentima. Formula: 100*(trenutni_napon - MINBAT)/(MAXBAT - MINBAT) 
5. Proračunati autonomiju vozila. Poslati preko PC-a karaktere PP a onda broj koji označava prosečnu potrošnju baterije u kWh i prema tome izračunati koliko se još kilometara auto može kretati sa trenutnom napunjenošću baterije.
6. Realizovati komande START i STOP kojima se meri koliko je potrošeno energije iz baterije (u procentima) u vremenu koje je proteklo između slanja ove dve naredbe. Tokom ovog merenja potrebno je uključiti jednu led diodu na simuliranom sistemu.
7. Slanje trenutne količine baterije prema PC-u u procentima. Kontinualno slati podatke periodom od 1000ms.
8. Uključiti jednu LED diodu na simuliranom sistemu kada nivo napunjenosti baterije padne ispod 10%.
9. Na LCD displeju prikazati izmerene podatke, brzina osvežavanja podataka je 100ms. Zavisno koji je "taster" pritisunt na LED baru, prikazati trenutni procenat napunjenosti baterije ili autonomiju vozila.
10. Napisati kod po MISRA standardu.
## Simulacija sistema
Periferije koje je potrebno koristiti u ovom projektu su:

1.*AdvUniCom* softver za simulaciju serijske komunikacije, 

2.*LED_bar_plus* i 

3.*seg7_mux.*


Prilikom pokretanja seg7_mux.exe navesti broj 3 kao argument kako bismo dobili displej sa 3 cifre. 

Pokretanjem led_bar_plus.exe navesti argument GGrr, kako bismo dobili led bar sa dva izlazna (led diode) i dva ulazna stupca (tasteri). 

Prilikom pokretanja AdvUniCom softvera potrebno je otvoriti kanal 0 i kanal 1. Kanal 0 se automatski otvara i u njemu upisati da kada stigne karakter 'U', kanal 0 šalje naredbu npr. V4200m. Potrebno je čekirati auto i ok1. Ovim smo postigli da se trenutni napon šalje svaki sekund ka FreeRTOS-u.
Menjanjem te vrednosti simulira se pristizanje različitih vrednosti napona. 

Kanal 1 otvoriti dodavanjem broja 1 kao argument: AdvUniCom.exe 1. Poslati naredbe za MINBAT, MAXBAT i PP(potrošnju). 
Na primer: \00MINBAT10\0d\00MAXBAT4900\0d\00PP15\0d. 
Početak poruke označava hex 0x00 a kraj poruke hex 0x0d, odnosno CR(Carriage Return).
Minimalna vrednost baterije tada je 10mV, maksimalna je 4900mV a potrošnja 15kWh.
U prozoru kanala 1 se tada pojavljuje nivo goriva u procentima. Ukoliko je vrednost nivoa baterije u procentima manja od 10% na LED baru će zasvetleti četvrta dioda u prvom stupcu.

Simulacija rada START-STOP tastera:
Pritiskom na START taster (prvi taster od gore u četvrtom stupcu) upali se prva dioda od dole u drgom stupcu i ona označava da je merenje aktivno.Potom je potrebno isklučiti START taster i promeniti vrednost napona koja se šalje (smanjiti ga) i pritisnuti STOP taster (drugi taster od gore u četvrtom stupcu).
Dioda koja označava da je merenje aktivno se gasi.
Merenje je tada završeno i vrednost potrošene količine baterije u procentima se prikazuje na seg7_mux displeju pritiskom na treći taster od dole u trećem stupcu.

Pritiskom na prvi taster od dole u trećem stupcu, na displeju će se prikazati vrednost baterije u procentima, a pritiskom na drugi taster od dole u trećem stupcu, prikazaće se autonomija vozila, odnosno koliko još kilometara automobil može da pređe sa trenutnom količinom baterije.



