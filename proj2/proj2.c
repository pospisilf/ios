// IOS PROJEKT č. 2
// Filip Pospíšil
// xpospi0f@stud.fit.vutbr.cz

//includy
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <ctype.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>

//struktury
typedef struct argumenty{
  int p; //počet osob generovaných v každé kategorii
  int h; //maximální hodnota doby, po kterou je generován proces HACKERS
  int s; //maximální hodnota doby, po kterou je generován proces SERFS
  int r; //maximálná doba plavby
  int w; //maximální doba, po které se osoba vrací na molo
  int c; //kapacita mola
} Targy;

typedef struct{
  int cisloAkce; //aktuální číslo prováděné akce
  int cekajiciHack; //čekající hacker na nastoupení
  int cekajiciSerf; //čekající serf na nastoupení
  int lideNaPalube; //lidé na palubě
  int spici; //uspané procesy
} shared;

//globální proměnné
FILE *vystup = NULL;
int shm_pamet;
int shm_argy;
shared *pametovastruktura;
Targy *argumenty;
pid_t GenHackerPID;
pid_t GenSerfPID;
int *cisloAkce;
sem_t *zavadeci;
sem_t *poradi;
sem_t *nastoupeniHack;
sem_t *nastoupeniSerf;
sem_t *plavba;
int jeKapitan = 0;

//prototypy funkcí
bool check_args();
void turnoff();
void maincontrol();
bool nactisemafory();
void zavrisemafory();
bool hacker(int HackerID);
bool serf(int SerfID);

//fce
bool check_args(int argc, char *argv[]){
	char *error = NULL;
	if (argc!=7){ //pokuď je počet argumentý jiný než 6
 		fprintf(stderr, "Špatný počet parametrů!\n"); //vytiskni na stderr zprávu
    exit (1); //ukonči s kódem 1
 		return 1; //návratová hodnota 1
 	}
 	else { //když je správný počet argumentů
    //převedení celých čísel v řetězci znaků na int
		argumenty->p = strtol(argv[1], &error, 10);
		argumenty->h = strtol(argv[2], &error, 10);
		argumenty->s = strtol(argv[3], &error, 10);
		argumenty->r = strtol(argv[4], &error, 10);
		argumenty->w = strtol(argv[5], &error, 10);
		argumenty->c = strtol(argv[6], &error, 10);
    //ošetření správnosti
		if(*error != 0 || !(argumenty->p >= 2 && argumenty->p % 2 == 0 ) || !(argumenty->h >= 0 && argumenty->h <= 2000) ||
    !(argumenty->s >= 0 && argumenty->s <= 2000) || !(argumenty->r >= 0 && argumenty->r <= 2000) || !(argumenty->w >= 20 && argumenty->w <= 2000) || !(argumenty->c >= 5)){
      //turnoff(); //když se nezadaří, vypni
 			fprintf(stderr, "Špatně zadaný parametr!\n"); //vypiš na stderr chybovou zprávu
      exit (1); //ukonči s kódem 1 viz zadání
		}
	}
	return 0;
}


void turnoff(){
    //odlinknutí všech semaforů
    {sem_unlink("sem_xpospi0f_zavadeci");} //odlinkuntí semaforů
    {sem_unlink("sem_xpospi0f_poradi");} //odlinkuntí semaforů
    {sem_unlink("sem_xpospi0f_nastoupeniHack");} //odlinkuntí semaforů
    {sem_unlink("sem_xpospi0f_nastoupeniSerf");} //odlinkuntí semaforů
    {sem_unlink("sem_xpospi0f_plavba");} //odlinkuntí semaforů

    //unmapnutí struktury ze sdílené paměti
    munmap(pametovastruktura, sizeof(int)); //odmapování paměťové struktury ze sdílené paměti

    //uzavření sharedPamet
    shm_unlink("/sharedPamet"); //unlinknutí sdílené paměti pro sdílené věci ze struktury
    close(shm_pamet);//zavření sdílené paměti pro sdílené věci ze struktury

    //uzavření sharedArgumenty
    shm_unlink("/sharedArgumenty"); //unlinknutí sdílené paměti pro argumenty
    close(shm_argy);//zavření sdílené paměti pro argumenty

    //Uzavření výstupu do kterého zapisuji
    if(vystup != NULL){
      fclose(vystup);
    }
    else{
      //když se nezadaří, neprovádím nic
    }
}

bool nactisemafory(){
  zavadeci = sem_open("sem_xpospi0f_zavadeci", O_CREAT | O_EXCL, 0664, 1); //otevřít semafor
  if(zavadeci == SEM_FAILED){ //kontrola otevření
    return -1;
  }
  poradi = sem_open("sem_xpospi0f_poradi", O_CREAT | O_EXCL, 0664, 1);//otevřít semafor
  if(poradi == SEM_FAILED){//kontrola otevření
    return -1;
  }
  nastoupeniHack = sem_open("sem_xpospi0f_nastoupeniHack", O_CREAT | O_EXCL, 0664, 0);//otevřít semafor
  if(nastoupeniHack == SEM_FAILED){//kontrola otevření
    return -1;
  }
  nastoupeniSerf = sem_open("sem_xpospi0f_nastoupeniSerf", O_CREAT | O_EXCL, 0664, 0);//otevřít semafor
  if(nastoupeniSerf == SEM_FAILED){//kontrola otevření
    return -1;
  }
  plavba = sem_open("sem_xpospi0f_plavba", O_CREAT | O_EXCL, 0664, 0);//otevřít semafor
  if(plavba == SEM_FAILED){//kontrola otevření
    return -1;
  }


  shm_pamet = shm_open("/sharedPamet", O_CREAT | O_EXCL | O_RDWR, 0664);//otevření sdílené paměti
  if(shm_pamet == -1){//kontrola otevření
    return -1;
  }
  shm_argy = shm_open("/sharedArgumenty", O_CREAT | O_EXCL | O_RDWR, 0644);//otevření sdílené paměti
  if(shm_argy == -1){//kontrola otevření
    return -1;
  }

  ftruncate(shm_pamet, sizeof(shared)); //zkrať na délku sizeof shared
  pametovastruktura = mmap(NULL, sizeof(shared), PROT_READ | PROT_WRITE, MAP_SHARED, shm_pamet, 0); //mapování sdílené paměti
	pametovastruktura->cisloAkce = 1; //nastavuji číslo akce na 1, začínáme totiž od jedničky
  pametovastruktura->cekajiciHack = 0; //nastavuji na 0, začínáme od nuly
  pametovastruktura->cekajiciSerf = 0;//nastavuji na 0, začínáme od nuly
  pametovastruktura->lideNaPalube = 0;//nastavuji na 0, začínáme od nuly
  pametovastruktura->spici = 0;//nastavuji na 0, začínáme od nuly

  ftruncate(shm_argy, sizeof(argumenty));//zkrať na délku sizeof argumenty
  argumenty = mmap(NULL, sizeof(Targy), PROT_READ | PROT_WRITE, MAP_SHARED, shm_argy, 0); //mapování sdílené paměti
  argumenty->p = 0; //nastavuji na 0, začínáme od nuly
  argumenty->h = 0; //nastavuji na 0, začínáme od nuly
  argumenty->s = 0; //nastavuji na 0, začínáme od nuly
  argumenty->r = 0; //nastavuji na 0, začínáme od nuly
  argumenty->w = 0; //nastavuji na 0, začínáme od nuly
  argumenty->c = 0; //nastavuji na 0, začínáme od nuly

  return 0;
}

void zavrisemafory(){
  //zavře každý semafory
  sem_close(zavadeci); //zavři zaváděcí
  sem_close(poradi); // etc.
  sem_close(nastoupeniSerf);// etc.
  sem_close(nastoupeniHack);// etc.
  sem_close(plavba);// etc.
}

bool hacker(int HackerID){
  srand(time(0) * getpid()); //Vygenerujeme PID procesu
  sem_wait(zavadeci); //čekáme
  fprintf(vystup,"%d\t: HACK %d\t: starts\n", pametovastruktura->cisloAkce, HackerID); //vypíšeme do výstupního souboru aktuální krok
  //printf("%d\t: HACK %d\t: starts\n", pametovastruktura->cisloAkce, HackerID); //po odkomentování lze využít k výpisu do konzole, stejné pro všechny zakomentované printf -> dále nekomentuji
  pametovastruktura->cisloAkce++;//zvětším číslo akce
  sem_post(zavadeci);//zvyšujeme hodnotu
  sem_wait(poradi);//čekáme
  sem_wait(zavadeci);//čekáme
  pametovastruktura->cekajiciHack++; //přidáme osobu do čekajících z kategorie Hack
  if((pametovastruktura->cekajiciHack+pametovastruktura->cekajiciSerf) > argumenty->c){ //jestliže je počet čekajících (sečtu cekajiciHack a cekajiciSerf) větší, než kapacita mola, proces odchází a uspíš se na dobu mezi20 aW
    fprintf(vystup,"%d\t: HACK %d\t: leaves queue \t: %d\t: %d\n", pametovastruktura->cisloAkce, HackerID, pametovastruktura->cekajiciHack, pametovastruktura->cekajiciSerf);
    //printf("%d\t: HACK %d\t: leaves queue \t: %d\t: %d\n", pametovastruktura->cisloAkce, HackerID, pametovastruktura->cekajiciHack, pametovastruktura->cekajiciSerf);
    usleep(20 + (argumenty->w-20)*(double) rand()/RAND_MAX); //upsívíme proces na random dobu mezi 20 a argumentem w
    pametovastruktura->cisloAkce++;
  }
  else{ //když není víc lidí čekajících než je max kapacita mola
    fprintf(vystup,"%d\t: HACK %d\t: waits \t: %d\t: %d\n", pametovastruktura->cisloAkce, HackerID, pametovastruktura->cekajiciHack, pametovastruktura->cekajiciSerf);
    //printf("%d\t: HACK %d\t: waits \t: %d\t: %d\n", pametovastruktura->cisloAkce, HackerID, pametovastruktura->cekajiciHack, pametovastruktura->cekajiciSerf);
    pametovastruktura->cisloAkce++;
  }
  sem_post(zavadeci);//zyvšujeme hodnotu
  sem_wait(zavadeci);//zyvšujeme hodnotu
  //ověřujeme, zda nám už nečeká dostatek lidí na nalodění
  if(pametovastruktura->cekajiciHack == 4){ //když máme 4 hackery, pro 4 serfery viz bool serf()
    pametovastruktura->cekajiciHack -= 4; //snížíme počet čekajících hackerů o 4
    jeKapitan = 1; //proměnná je kapitán jde na jedna, dle zadání je kapitánem kdokoliv
    sem_post(nastoupeniHack);//nastupuje hack
    sem_post(nastoupeniHack);//nastupuje hack
    sem_post(nastoupeniHack);//nastupuje hack
    sem_post(nastoupeniHack);//nastupuje hack
  }
  else if ((pametovastruktura->cekajiciHack == 2) && (pametovastruktura->cekajiciSerf >= 2)){ //když čekají 2 hacekři a dva serfeři
    pametovastruktura->cekajiciHack -= 2; //snížíme počet čekajících hack o 2
    pametovastruktura->cekajiciSerf -= 2; //snížíme počet čekajících serfs o 2
    jeKapitan = 1;
    sem_post(nastoupeniHack);//nastupuje hack
    sem_post(nastoupeniHack);//nastupuje hack
    sem_post(nastoupeniSerf);//nastupuje serf
    sem_post(nastoupeniSerf);//nastupuje serf
    }
  else //když není dost lidí
    {
    sem_post(poradi);//zvyšujeme hodnotu
    }
  sem_post(zavadeci);//zvyšujeme hodnotu
  sem_wait(nastoupeniHack);//čekáme na dostatek lidí na utvoření skupiny
  //nastupujeme
  sem_wait(zavadeci);//čekáme
  fprintf(vystup,"%d\t: HACK %d\t: boards \t: %d\t: %d\n", pametovastruktura->cisloAkce, HackerID, pametovastruktura->cekajiciHack, pametovastruktura->cekajiciSerf);
  //printf("%d\t: HACK %d\t: boards \t: %d\t: %d\n", pametovastruktura->cisloAkce, HackerID, pametovastruktura->cekajiciHack, pametovastruktura->cekajiciSerf);
  pametovastruktura->lideNaPalube++;
  pametovastruktura->cisloAkce++;
  sem_post(zavadeci);

  sem_wait(zavadeci);
  if(pametovastruktura->lideNaPalube == 4){
    pametovastruktura->lideNaPalube -= 4;
    sem_post(plavba);
    sem_post(plavba);
    sem_post(plavba);
    sem_post(plavba);
  }
  sem_post(zavadeci);

  sem_wait(plavba);

  sem_wait(zavadeci);
  if (jeKapitan == 1) {
    fprintf(vystup, "%d\t: HACK %d\t: boards \t: %d\t: %d\n", pametovastruktura->cisloAkce,HackerID, pametovastruktura->cekajiciHack, pametovastruktura->cekajiciSerf);
    //printf("%d\t: HACK %d\t: boards \t: %d\t: %d\n", pametovastruktura->cisloAkce,HackerID, pametovastruktura->cekajiciHack, pametovastruktura->cekajiciSerf);
    jeKapitan = 0;
    pametovastruktura->cisloAkce++;
  }
  else{
    // fprintf(vystup, "%d\t: HACK %d\t: member \t: %d\t: %d\n", pametovastruktura->cisloAkce,HackerID, pametovastruktura->cekajiciHack, pametovastruktura->cekajiciSerf);
    // printf("%d\t: HACK %d\t: member \t: %d\t: %d\n", pametovastruktura->cisloAkce,HackerID, pametovastruktura->cekajiciHack, pametovastruktura->cekajiciSerf);
    // pametovastruktura->cisloAkce++;
  }
  sem_post(zavadeci);

  return 0;
}

bool serf(int SerfID){ // V zásadě se děje to stejné co v hacker, pro komentáře tedy pohlédněte do bool hacker(int HackerID);
  srand(time(0) * getpid());
  fprintf(vystup,"%d\t: SERF %d\t: starts\n", pametovastruktura->cisloAkce, SerfID);
  //printf("%d\t: SERF %d\t: starts\n", pametovastruktura->cisloAkce, SerfID);
  sem_wait(zavadeci);
  pametovastruktura->cisloAkce++;
  sem_post(zavadeci);
  sem_wait(poradi);
  sem_wait(zavadeci);
  pametovastruktura->cekajiciSerf++;
  if((pametovastruktura->cekajiciHack+pametovastruktura->cekajiciSerf) > argumenty->c){
    fprintf(vystup,"%d\t: SERF %d\t: leaves queue \t: %d\t: %d\n", pametovastruktura->cisloAkce, SerfID, pametovastruktura->cekajiciHack, pametovastruktura->cekajiciSerf);
    //printf("%d\t: SERF %d\t: leaves queue \t: %d\t: %d\n", pametovastruktura->cisloAkce, SerfID, pametovastruktura->cekajiciHack, pametovastruktura->cekajiciSerf);
    usleep(20 + (argumenty->w-20)*(double) rand()/RAND_MAX);
    pametovastruktura->cisloAkce++;
  }
  else{
    fprintf(vystup,"%d\t: SERF %d\t: waits \t: %d\t: %d\n", pametovastruktura->cisloAkce, SerfID, pametovastruktura->cekajiciHack, pametovastruktura->cekajiciSerf);
    //printf("%d\t: SERF %d\t: waits \t: %d\t: %d\n", pametovastruktura->cisloAkce, SerfID, pametovastruktura->cekajiciHack, pametovastruktura->cekajiciSerf);
    pametovastruktura->cisloAkce++;
  }
  sem_post(zavadeci);
  sem_wait(zavadeci);
  if(pametovastruktura->cekajiciSerf == 4){
    pametovastruktura->cekajiciSerf -= 4;
    jeKapitan = 1;
    sem_post(nastoupeniHack);
    sem_post(nastoupeniHack);
    sem_post(nastoupeniHack);
    sem_post(nastoupeniHack);
  }
  else if ((pametovastruktura->cekajiciHack == 2) && (pametovastruktura->cekajiciSerf >= 2)){
    pametovastruktura->cekajiciHack -= 2;
    pametovastruktura->cekajiciSerf -= 2;
    jeKapitan = 1;
    sem_post(nastoupeniHack);
    sem_post(nastoupeniHack);
    sem_post(nastoupeniSerf);
    sem_post(nastoupeniSerf);
    }
  else
    {
    sem_post(poradi);
    }
  sem_post(zavadeci);
  sem_wait(nastoupeniSerf);
  sem_wait(zavadeci);
  fprintf(vystup,"%d\t: SERF %d\t: boards \t: %d\t: %d\n", pametovastruktura->cisloAkce, SerfID, pametovastruktura->cekajiciHack, pametovastruktura->cekajiciSerf);
  //printf("%d\t: SERF %d\t: boards \t: %d\t: %d\n", pametovastruktura->cisloAkce, SerfID, pametovastruktura->cekajiciHack, pametovastruktura->cekajiciSerf);
  pametovastruktura->lideNaPalube++;
  pametovastruktura->cisloAkce++;
  sem_post(zavadeci);
  sem_wait(zavadeci);
  if(pametovastruktura->lideNaPalube == 4){
    pametovastruktura->lideNaPalube -= 4;
    sem_post(plavba);
    sem_post(plavba);
    sem_post(plavba);
    sem_post(plavba);
  }
  sem_post(zavadeci);
  sem_wait(plavba);
  sem_wait(zavadeci);
  if (jeKapitan == 1) {
    fprintf(vystup, "%d\t: SERF %d\t: boards \t: %d\t: %d\n", pametovastruktura->cisloAkce,SerfID, pametovastruktura->cekajiciHack, pametovastruktura->cekajiciSerf);
    jeKapitan = 0;
    //printf("%d\t: SERF %d\t: boards \t: %d\t: %d\n", pametovastruktura->cisloAkce,SerfID, pametovastruktura->cekajiciHack, pametovastruktura->cekajiciSerf);
    pametovastruktura->cisloAkce++;
  }
  else{
    // fprintf(vystup, "%d\t: HACK %d\t: member \t: %d\t: %d\n", pametovastruktura->cisloAkce,HackerID, pametovastruktura->cekajiciHack, pametovastruktura->cekajiciSerf);
    // printf("%d\t: HACK %d\t: member \t: %d\t: %d\n", pametovastruktura->cisloAkce,HackerID, pametovastruktura->cekajiciHack, pametovastruktura->cekajiciSerf);
    // pametovastruktura->cisloAkce++;
  }
  sem_post(zavadeci);
  return 0;
}


int main(int argc, char *argv[]){
  turnoff(); //pro jistotu vypneme vše co mohlo zůstat
  zavrisemafory(); //pro jistotu zavřeme semafory

  if(nactisemafory() != 0){ //načteme semafory, když se nepovede tak ukončíme.
    fprintf(stderr, "Problém se semafory.\n");
    turnoff(); //vypneme vše.
    return 1; //návratová hodnota jedna dle zadání
    }

  check_args(argc,argv); //kontrola argumentů

  signal(SIGTERM, maincontrol); //dáme signál na vykonání SIGTERM a SIGINT funkcni maincontrol();
  signal(SIGINT, maincontrol);

  if( (vystup = fopen("proj2.out","w+") ) == NULL){ //pokusíme se otevřít soubor
      fprintf(stderr,"Chyba během otevírání souboru!\n"); //když nastane chyba, píšeme na stderr
      turnoff(); //bezpečně vypínáme
      return 1; //návratová hodnota viz zadání
    }
  fflush(vystup); //pro lepší zapisování
  setbuf(vystup, NULL); //pro lepší zapisování

  //vytvoř procesy hacker
  GenHackerPID = fork(); //fork procesu
  if(GenHackerPID >= 0){ //proces tvořící Hackers!
    if(GenHackerPID == 0){ //child proces
      signal(SIGINT, maincontrol); //SIGINT a SIGTERM funkcí maincontrol(())
      signal(SIGTERM, maincontrol);
      srand(time(0) * getpid()); //vytvoři PID
      for(int j = 1; j < argumenty->p+1; j++){ //tvoř proces dokuď nedosáhneš max. počtu lidí v kategorii
        usleep(rand() % ((argumenty->s)+1)); //prodleva mezi tvorbou
        pid_t HackerPID = fork(); //fork procesu
        if (HackerPID >= 0) { //child
          if(HackerPID == 0){ //když se povede vše vytvořit
            signal(SIGTERM, maincontrol);
            signal(SIGINT, maincontrol);
            hacker(j); //volám hacker() s hodnotou viz cyklus
            zavrisemafory(); //nakonec uzávírám semafory
            exit(0); //vše ok, exit s hodnotou 0
          }
        }
        else{//parent
          fprintf(stderr, "Chyba během vytváření procesu hacker.\n"); //chyba na stderr
          kill(0,SIGSTOP); //kill procesů
          kill(0,SIGTERM);
          exit(1); //návratová hodnota viz zadání
        }
      }
      for (int j = 1; j < argumenty->p+1; j++) {
        wait(NULL);
      }
      exit(0);
    }
    else{
    }
  }
  //proces serfs -> stejný jako hackers!
  GenSerfPID = fork();
  if(GenSerfPID >= 0){
    if(GenSerfPID == 0){ //child proces
      signal(SIGINT, maincontrol);
      signal(SIGTERM, maincontrol);
      srand(time(0) * getpid());
      for(int j = 1; j <= argumenty->p; j++){
        usleep(rand() % ((argumenty->s-0)+1));
        pid_t SerfPID = fork();
        if (SerfPID >= 0) {
          if(SerfPID == 0){
            signal(SIGTERM, maincontrol);
            signal(SIGINT, maincontrol);
            serf(j);
            zavrisemafory();
            exit(0);
          }
        }
        else{
          fprintf(stderr, "Chyba během vytváření procesu serf.\n");
          kill(0,SIGSTOP);
          kill(0,SIGTERM);
          exit(2);
        }
      }
      for (int j = 1; j <= argumenty->p; j++) {
        wait(NULL);
      }
      exit(0);
    }
    else{
    }
  }
}

void maincontrol(){
  kill(0,SIGTERM);
  turnoff();
  zavrisemafory();
  exit(1);
}
