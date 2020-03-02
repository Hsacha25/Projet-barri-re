//07/06/2019 : intégration capteurs actionneurs lecteurs et afficheur
//      BARRIERE DE PARKING DECMA-REP
//      - afficheur I2C Midas 3V3
//      - classe MX2ModBus
//      - gestion lecteur RFID PN532

/*
?augmenter le nb #define NBPLACES 100 et #define NBRFID 200 ?
*/
#include "mbed.h"
#include "EthernetInterface.h"
//JF
//#include "mbed.h"
#include "MX2ModBus.h"
#include "PN532.h"
#include "PN532_I2C.h"
#include "SB1602E.h"
//!JF

#define NBPLACES 100
#define NBRFID 200
LocalFileSystem local("local");
DigitalOut laled1(LED1),laled2(LED2),laled3(LED3),laled4(LED4);
Serial pc(USBTX, USBRX); // tx, rx
EthernetInterface eth;
TCPSocketServer server;
//JF
I2C i2c(p9, p10);
PN532_I2C pn532_if(i2c);
PN532 nfc(pn532_if);
SB1602E lcd( i2c );
//Serial pc(USBTX, USBRX); // tx, rx
Serial barCode(p13,p14);
DigitalOut barCodeTrig(p15), barCodeRst(p16);
MX2ModBus variateur;
Ticker scanQR;
void activationQR();
//!JF
//JF
//char etat=0;
char tagAutorise[4]={0x0A,0x08,0x2C,0x83};
//void receptionPC();
void receptionBarCode();
bool demandeMontee();
bool demandeDescente();
//bool identification();
void initLecteurRFID(void);
bool identRFID(char code[9]);
int ouvrirB=2; //0 fermer, 1 ouvrir, 2 aucune commande
//!JF
char code[30]="00000000";
int icode=0;
typedef struct
{ char code[9];
  time_t deb,fin;  //UNIX time
} AccesRFID;
int lport;
char lip[20],lnom[10],lES[10],lmode[10];//lmode : 'C'=clavier 'R'=rfid 'Q'=qrcode
char presents[NBPLACES][9]; //uniquement en E/S
int nbPresents;             //positif uniquement en E/S
int compteur;               //positif ou négatif
char letat;int afficheTrames;
AccesRFID aRFID[NBRFID];
int nbARFID,lplaces;
char ASCIIpoidsFort(char caractere)
{   char c=caractere>>4; if(c>9)c+='A'-10; else c+='0'; return c;
}
char ASCIIpoidsFaible(char caractere)  
{   char c=caractere&0xF; if(c>9)c+='A'-10; else c+='0'; return c;      
}
char Caractere(char aSCIIpoidsFort,char aSCIIpoidsFaible)
{   char c; if(aSCIIpoidsFort>='A') c=(aSCIIpoidsFort-'A'+10)<<4; else c=(aSCIIpoidsFort-'0')<<4;
    if(aSCIIpoidsFaible>='A') c+=aSCIIpoidsFaible-'A'+10; else c+=aSCIIpoidsFaible-'0';
    return c;
}
time_t convertitEnSecondesUNIX(char ldate[15],char lheure[15])
{   struct tm t;
    t.tm_sec = (lheure[6]-'0')*10+lheure[7]-'0';    // 0-59
    t.tm_min = (lheure[3]-'0')*10+lheure[4]-'0';    // 0-59
    t.tm_hour = (lheure[0]-'0')*10+lheure[1]-'0';   // 0-23
    t.tm_mday = (ldate[0]-'0')*10+ldate[1]-'0';   // 1-31
    t.tm_mon = ((ldate[3]-'0')*10+ldate[4]-'0')-1;     // 0-11
    t.tm_year = ((ldate[6]-'0')*1000+(ldate[7]-'0')*100+(ldate[8]-'0')*10+ldate[9]-'0')-1900;  // year since 1900
    return mktime(&t);
}
void AnneeEnCours(char an[5], int &annee)
{   time_t seconds = time(NULL);
    strftime(an, 5, "%Y", localtime(&seconds));
    annee=(an[0]-'0')*1000+(an[1]-'0')*100+(an[2]-'0')*10+an[3]-'0';
}
void LitEtat()
{   FILE *fc = fopen("/local/etat.txt", "r");
    fscanf(fc,"%d",&letat); 
    fclose(fc);
}
void ChargeConfig()
{   FILE *fc = fopen("/local/config.txt", "r");
    char text[15],ldate[15],lheure[15];
    fscanf(fc,"%s %s %s",text,ldate,lheure);
    fscanf(fc,"%s %s",text,lip);    
    fscanf(fc,"%s %d",text,&lport);
    fscanf(fc,"%s %s",text,lnom);
    fscanf(fc,"%s %d",text,&lplaces);    
    fscanf(fc,"%s %s",text,lES);    
    fscanf(fc,"%s %d",text,&compteur);
    fscanf(fc,"%s %d",text,&afficheTrames);
    fscanf(fc,"%s %s",text,lmode);    
    nbPresents=compteur; if(nbPresents<0) nbPresents=0;
    pc.printf("\r\n[%s %s] %s [%s] %d/%d IP : %s PORT : %d",ldate,lheure,lnom,lES,nbPresents,lplaces,lip,lport);
    fclose(fc);
    //mise à l'heure MBED
    //  0123456789 01234567
    //  17/01/2019 08:14:07
    time_t seconds=convertitEnSecondesUNIX(ldate,lheure);
    set_time(seconds);
}
void SauveConfig()
{   FILE *fc = fopen("/local/config.txt", "w");
    char dateheure[32];
    time_t seconds = time(NULL);
    strftime(dateheure, 32, "%d/%m/%Y %H:%M:%S", localtime(&seconds));
    fprintf(fc,"DATEHEURE\t%s\r\n",dateheure);
    fprintf(fc,"IP\t\t%s\r\n",lip);    
    fprintf(fc,"PORT\t\t%d\r\n",lport);
    fprintf(fc,"NOM\t\t%s\r\n",lnom);
    fprintf(fc,"NBPLACES\t%d\r\n",lplaces);
    fprintf(fc,"ENTREE/SORTIE\t%s\r\n",lES);      
    fprintf(fc,"COMPTEUR\t%d\r\n",compteur); 
    fprintf(fc,"AFFICHETRAMES\t%d\r\n",afficheTrames);     
    fprintf(fc,"LECTURE(C/R/Q)\t%s\r\n",lmode);       
    fclose(fc);
}
void ChargeAccesRFID()
{   nbARFID=0;
    FILE *far = fopen("/local/acces.txt", "r");
    char code[15],ldateheuredeb[25],ldateheurefin[25];
    //ici lire fichier et stocker dans structure
    do
    {   fscanf(far,"%s %s %s",code,ldateheuredeb,ldateheurefin);
        strcpy(aRFID[nbARFID].code,code);
        //convertit dates et heures et UNIX time
        aRFID[nbARFID].deb=convertitEnSecondesUNIX(ldateheuredeb,"00:00:00");
        aRFID[nbARFID].fin=convertitEnSecondesUNIX(ldateheurefin,"23:59:59");
        nbARFID++;
    }while(!feof(far)); nbARFID--;
    fclose(far);
}
void EnregistreAccesRFID()
{   FILE *far = fopen("/local/acces.txt", "w");
    for(int i=0;i<nbARFID;i++)
    {   char dateheure[32];
        fprintf(far,"%s\t",aRFID[i].code);
        strftime(dateheure, 32, "%d/%m/%Y", localtime(&aRFID[i].deb));
        fprintf(far,"%s\t", dateheure);
        strftime(dateheure, 32, "%d/%m/%Y", localtime(&aRFID[i].fin));
        fprintf(far,"%s\r\n", dateheure);
    }
    fclose(far);    
}    
void TestDateHeure()
{   char dateheure[32];
    while(1)
    {   time_t seconds = time(NULL);
        strftime(dateheure, 32, "\r\n%d/%m/%Y %H:%M:%S", localtime(&seconds));
        pc.printf("%s", dateheure);
        wait(1);
    }
}
bool AccesRFIDautorise(char code[9]) //vérifie date et heure d'autorisations
{   int indice=-1;
    for(int i=0;i<nbARFID;i++) if(!strcmp(code,aRFID[i].code)){indice=i; i=nbARFID;}
    if(indice!=-1)
    {   time_t seconds = time(NULL);
        if(seconds>aRFID[indice].deb && seconds<aRFID[indice].fin) return true;
    }
    return false;
}
bool EstPresentDansLeParking(char code[9])
{   for(int i=0;i<nbPresents;i++) if(!strcmp(code,presents[i])) return true;
    return false;
}
void SupprimePresent(char code[9])
{   int indice=-1;
    for(int i=0;i<nbPresents;i++) if(!strcmp(code,presents[i])) {indice=i; i=nbPresents;}
    if(indice!=-1) 
    {   for(int i=indice;i<nbPresents-1;i++) strcpy(presents[i],presents[i+1]);
        nbPresents--;
    }
    compteur--;SauveConfig();
}
void AffichePresents()
{   pc.printf("\r\n"); for(int i=0;i<nbPresents;i++) pc.printf("%s ",presents[i]);
}
void AjoutePresent(char code[9])
{   strcpy(presents[nbPresents],code); 
    nbPresents++; compteur++;SauveConfig();
}
void Bienvenue(time_t seconds,char dateheure[32],char code[50])
{   pc.printf("\r\nBIENVENUE");
    AjoutePresent(code);
    seconds = time(NULL);strftime(dateheure, 32, "%d/%m/%Y\t%H:%M", localtime(&seconds));
    FILE *fh = fopen("/local/histo.txt", "a");  
    fprintf(fh,"%s\t%s\t>\r\n",dateheure,code);
    fclose(fh);
}
void Aurevoir(time_t seconds,char dateheure[32],char code[50])
{   pc.printf("\r\nAU REVOIR");
    SupprimePresent(code);
    seconds = time(NULL);strftime(dateheure, 32, "%d/%m/%Y\t%H:%M", localtime(&seconds));
    FILE *fh = fopen("/local/histo.txt", "a");  
    fprintf(fh,"%s\t%s\t<\r\n",dateheure,code);
    fclose(fh); 
}
void OuvrirBarriere()
{   laled1 = 1;laled2 = 1;laled3 = 1;laled4 = 1;
    pc.printf("erreur:%d ",demandeMontee());
}
void FermerBarriere()
{   laled1 = 0;laled2 = 0;laled3 = 0;laled4 = 0;
    demandeDescente(); 
}

void AttenteClient(const void *context)
{   while (true) 
    {   //printf("\n\rAttente de connexion...\n\r");
        TCPSocketConnection client;
        server.accept(client);
        //printf("\n\rClient connecte : %s\n\r", client.get_address());
        char buffer[100];
        int n=client.receive(buffer, sizeof(buffer));
        buffer[30]=0;//message de 30 caractères sans \n\r
        char entete[30],requeteLigne[30]; strcpy(entete,buffer); strcpy(requeteLigne,buffer);entete[14]=0;requeteLigne[5]=0;
        char donnee[100]="nack";
        if(afficheTrames==1) printf("\n\rMessage du client : %s\n\r",buffer);
        if(!strcmp(buffer,"I00000100R0L1:0100000000000000"))    //ouvrir
        {   ouvrirB=1;
            //OuvrirBarriere();
            strcpy(donnee,"ouverte");
            
        }
        else if(!strcmp(buffer,"I00000100R0L1:0200000000000000"))    //fermer
        {   ouvrirB=0;
            //FermerBarriere();
            strcpy(donnee,"fermee");        
                   
        }
        else if(!strcmp(buffer,"I00000300R1L8:0000000000000000"))    //nom ?
        {   strcpy(donnee,"I00000300R0L8:0000000000000000");
            int j=14;//début donnée
            lnom[8]=0;//8 caractères max
            for(int i=0;i<strlen(lnom);i++)
            {   donnee[j]=ASCIIpoidsFort(lnom[i]);
                donnee[j+1]=ASCIIpoidsFaible(lnom[i]);
                j+=2;
            }
        }
        else if(!strcmp(entete,"I00000400R0L8:"))    //modif nom
        {   int j=14;
            for(int i=0;i<8;i++) {lnom[i]=Caractere(buffer[j],buffer[j+1]);j+=2;}
            lnom[8]=0;
            strcpy(donnee,"nom modifie : ");strcat(donnee,lnom);
            SauveConfig();
        }   
        else if(!strcmp(entete,"I00000500R0L8:"))    //modif ip
        {   int j=14; int cip[4];
            for(int i=0;i<4;i++) {cip[i]=Caractere(buffer[j],buffer[j+1]);j+=2;}
            sprintf(lip,"%d.%d.%d.%d",cip[0],cip[1],cip[2],cip[3]);
            strcpy(donnee,"ip modifie : ");strcat(donnee,lip);
            SauveConfig();
        }         
        else if(!strcmp(buffer,"I00000600R1L5:0000000000000000"))    //date heure ?
        {   strcpy(donnee,"I00000600R0L5:0000000000000000");
            time_t seconds = time(NULL);
            struct tm *t=NULL;
            t=localtime(&seconds);
            sprintf(donnee+14,"%02X%02X%02X%02X%02X000000",t->tm_mday,t->tm_mon+1,t->tm_year-100,t->tm_hour,t->tm_min);
        }
        else if(!strcmp(entete,"I00000700R0L5:"))    //modif j-m-a-h-mn
        {   int j=14; char ldt[15]; 
            struct tm t;
            t.tm_mday=Caractere(buffer[j],buffer[j+1]);j+=2;
            t.tm_mon=Caractere(buffer[j],buffer[j+1])-1;j+=2;
            t.tm_year=Caractere(buffer[j],buffer[j+1])+100;j+=2;
            t.tm_hour=Caractere(buffer[j],buffer[j+1]);j+=2;
            t.tm_min=Caractere(buffer[j],buffer[j+1]);j+=2;
            t.tm_sec=0;
            time_t seconds=mktime(&t);
            set_time(seconds);
            sprintf(ldt,"%02d/%02d/%02d %02d:%02d:%02d",t.tm_mday,t.tm_mon+1,t.tm_year-100,t.tm_hour,t.tm_min,t.tm_sec);
            strcpy(donnee,"heure modifie : ");strcat(donnee,ldt);
            SauveConfig();
        } 
        else if(!strcmp(entete,"I00000A00R0L8:"))    //ajout RFID (ID 4 octets puis j-m début, j-m fin)
        {   int j=14;
            for(int i=0;i<8;i++) aRFID[nbARFID].code[i]=buffer[i+j]; aRFID[nbARFID].code[8]=0;
            j+=8;
            struct tm t;
            t.tm_sec = 0;   t.tm_min = 0;   t.tm_hour = 0;
            t.tm_mday = Caractere(buffer[j],buffer[j+1]);j+=2;
            t.tm_mon = Caractere(buffer[j],buffer[j+1])-1;j+=2;
            int moisDeb=t.tm_mon;
            int a;char an[5]; AnneeEnCours(an,a);
            t.tm_year = a-1900;
            aRFID[nbARFID].deb=mktime(&t);
            t.tm_mday = Caractere(buffer[j],buffer[j+1]);j+=2;
            t.tm_mon = Caractere(buffer[j],buffer[j+1])-1;j+=2;
            if(t.tm_mon<moisDeb) t.tm_year++;
            aRFID[nbARFID].fin=mktime(&t);
            nbARFID++;
            EnregistreAccesRFID();
            char datedeb[32],datefin[32];
            strftime(datedeb, 32, "%d/%m/%Y", localtime(&aRFID[nbARFID-1].deb));
            strftime(datefin, 32, "%d/%m/%Y", localtime(&aRFID[nbARFID-1].fin));
            sprintf(donnee,"%s %s : %s",aRFID[nbARFID-1].code,datedeb,datefin);
            //I00000A00R0L8:ABCDEF010C01090C : 12/01 09/12
            //I00000A00R0L8:000000010A0C090A : 10/12 09/10            
        }
        else if(!strcmp(entete,"I00000B00R0L4:"))    //Retirer autorisation RFID (ID 4 octets)
        {   int j=14,indice=-1;
            char code[9];
            for(int i=0;i<8;i++) code[i]=buffer[i+j]; code[8]=0;
            for(int i=0;i<nbARFID;i++)
            {   if(!strcmp(code,aRFID[i].code)) indice=i;
            }
            if(indice!=-1)
            {   for(int i=indice;i<nbARFID-1;i++)
                    aRFID[i]=aRFID[i+1];
                nbARFID--;
                EnregistreAccesRFID();
                sprintf(donnee,"suppression %s",code);
            }
        }    
        else if(!strcmp(entete,"I00000C00R0L0:"))  //Effacer toutes les autorisations
        {  nbARFID=0;
           EnregistreAccesRFID();
           sprintf(donnee,"suppression autorisations");
        }     
        else if(!strcmp(buffer,"I00000D00R1L2:0000000000000000"))//Requête nb lignes historique
        {  int nb=0;
           FILE *fh = fopen("/local/histo.txt","r");
           char ldate[32],lheure[32],lcode[9],ldirection=0;
           //printf("%s\t%s\t%s\t%c\r\n",ldate,lheure,lcode,ldirection);
           if(fh)
           do
           {   fscanf(fh,"%s\t%s\t%s\t%c\r\n",ldate,lheure,lcode,&ldirection);
               nb++;
           }while(!feof(fh));
           sprintf(donnee,"I00000D00R0L2:%04X000000000000",nb);
           fclose(fh);   
        }    
        else if(!strcmp(buffer,"I00000E00R0L0:0000000000000000"))//Effacer Historique complet
        {  FILE *fh = fopen("/local/histo.txt","w");
           fclose(fh);
           sprintf(donnee,"suppression historique");
        }
        else if(!strcmp(requeteLigne,"I0001"))//Requête ligne Historique I00010001R1L8:0000000000000000
        {  int nb=0;
           int numLigne=Caractere(buffer[5],buffer[6])*256+Caractere(buffer[7],buffer[8]);
           FILE *fh = fopen("/local/histo.txt","r");
           char ldate[32],lheure[32],lcode[9],ldirection=0;
           if(fh)
           do
           {   ldirection=' ';
               fscanf(fh,"%s\t%s\t%s\t%c\r\n",ldate,lheure,lcode,&ldirection);
               nb++;
           }while(!feof(fh) && nb!=numLigne);
           //printf("%s\t%s\t%s\t%c\r\n",ldate,lheure,lcode,ldirection);
           char t_min=(lheure[3]-'0')*10+lheure[4]-'0';  
           char t_hour=(lheure[0]-'0')*10+lheure[1]-'0'; 
           char t_day=(ldate[0]-'0')*10+ldate[1]-'0';   
           char t_mon=((ldate[3]-'0')*10+ldate[4]-'0');   
           if(ldirection=='>') t_day=-t_day;
           if(ldirection!=' ')
                sprintf(donnee,"I0001%04XR0L8:%02X%02X%02X%02X%s",numLigne,t_day,t_mon,t_hour,t_min,lcode);
           //I00010001R0L8:0000000000000000  < Ligne numéro 0001 j-m-h-mn+ID : j en complément à 2 pour une sortie
           fclose(fh);   
        }
        else if(!strcmp(requeteLigne,"I0002"))//Effacer ligne Historique numéro 0001
        {  int nb=0;
           char ligne[40];
           int numLigne=Caractere(buffer[5],buffer[6])*256+Caractere(buffer[7],buffer[8]);
           FILE *fh = fopen("/local/histo.txt","r");
           FILE *fhw = fopen("/local/tmp.txt","w");
           if(fh)
           {   do
               {   nb++;
                   fgets(ligne,40,fh);                
                   if(nb!=numLigne)
                   if(!feof(fh)) 
                   fprintf(fhw,"%s",ligne);
               }while(!feof(fh));
           }  
           fclose(fh);            
           fclose(fhw);  
           FILE *fh2 = fopen("/local/histo.txt","w");
           FILE *fhw2 = fopen("/local/tmp.txt","r");
           if(fh)
           {   do
               {   fgets(ligne,40,fhw2);                
                   if(!feof(fhw2)) fprintf(fh2,"%s",ligne);
               }while(!feof(fhw2));
           }  
           fclose(fh2);            
           fclose(fhw2);                       
           sprintf(donnee,"ligne %d effacee",numLigne);
           //I00020001R0L2:0000000000000000  > Effacer ligne Historique numéro 0001
        }
        else if(!strcmp(buffer,"I00000200R1L1:0000000000000000"))//Requête état barrière (0-0-PésenceAval-PésenceAmont-FinCourseBas-FinCourseHaut-ExtérieurDescente-ExtérieurMontée)
        {  sprintf(donnee,"I00000200R0L1:%02X00000000000000",letat);
        }
        else if(!strcmp(entete,"I00000F00R0L2:"))    //Nombre de places
        {   int j=14; 
            lplaces=(Caractere(buffer[j],buffer[j+1])<<8)+(unsigned char)Caractere(buffer[j+2],buffer[j+3]);
            sprintf(donnee,"%d places",lplaces);
            SauveConfig();
        }
        else if(!strcmp(entete,"I00001000R0L1:"))    //Entrée=0, Sortie=1, E/S=2
        {   int j=14; 
            int n=Caractere(buffer[j],buffer[j+1]);
            if(n==0) strcpy(lES,"E");
            else if(n==1) strcpy(lES,"S");
            else if(n==2) strcpy(lES,"ES");
            sprintf(donnee,"%s",lES);
            SauveConfig();
        }            
        else if(!strcmp(buffer,"I00001100R1L2:0000000000000000"))//Requête valeur compteur
        {  sprintf(donnee,"I00000200R0L2:%04X000000000000",compteur&0xffff);
           SauveConfig();
        }
        else if(!strcmp(buffer,"I00001200R0L0:0000000000000000"))//RAZ compteur
        {  sprintf(donnee,"RAZ compteur");
           compteur=0;
           SauveConfig();
        }         
        client.send_all(donnee, strlen(donnee));
        client.close();
    }
}

int main (void) 
{   ChargeConfig();
    ChargeAccesRFID();
    char dateheure[32];
    time_t seconds = time(NULL);
    strftime(dateheure, 32, "%d/%m/%Y %H:%M:%S", localtime(&seconds));
//    FILE *fp = fopen("/local/histo.txt", "a");  
//    fprintf(fp,"Demarrage %s:%d %s\r\n",lip,lport,dateheure);
//    fclose(fp);
    eth.init(lip, "255.255.0.0", "10.10.10.10"); 
    eth.connect();
    pc.printf("\n\r\n\rAdresse IP du MBED %s, Port %d\n\r", eth.getIPAddress(),lport);
    server.bind(lport);
    server.listen();
    Thread thread(AttenteClient,0);
//JF
    //pc.attach(&receptionPC);
    wait(1);
    if(lmode[0]=='R')initLecteurRFID();
    //pc.printf("\n\rDebut du programme, variateur numero %d\n\r",variateur.getAdresse());
    //pc.printf("Etape 0\n\r");
    wait(2);
    barCodeTrig=1;
    barCodeRst=1;
    wait(0.1);
    //if(lmode[0]=='Q')barCode.attach(&receptionBarCode);
    wait(2);
//!JF
    while(1) 
    {   //lcd.printf( 0, "Bonjour \r" );
        if(ouvrirB==1) {OuvrirBarriere();ouvrirB=2;}        //ouverture distante
        else if(ouvrirB==0) {FermerBarriere();ouvrirB=2;}   //fermeture distante
        char aff[20];    strcpy(aff,lnom);    strcat(aff," \r");    lcd.printf( 0, aff);    
        strcpy(code,"--------"); 
        //LitEtat();//état barrière dans un fichier pour les tests
        if(variateur.lectureBoucles(letat)) pc.printf("etat=%X\n\r",letat);// lecture sans erreur
        //else pc.printf("Erreur lecture boucle\n\r");
if((letat&0x10)==0x10) 
{       if(lmode[0]=='R')
            if(identRFID(code))//pc.printf("OK\n\r");else pc.printf("KO\n\r"); 
            {    pc.printf("Code=%s\n\r",code);
                 wait(1);
            }
        if(lmode[0]=='Q')//le mbed plante lors du déclanchement de l'interruption BarCode
        {   //pc.printf("Attente vehicule (A) : ");
            //char arriv[50];
            //pc.scanf("%s",arriv);
            //barCodeTrig=1;
//            wait(0.05);
//            barCodeTrig=0;
            scanQR.attach(&activationQR,2);
            //wait(10);
            barCode.scanf("%s",code);
            pc.printf("%s\r\n",code); 
            scanQR.detach();
        }
        if(lmode[0]=='C')
        {   //arrivée d'une voiture : si accès et non présent enregistrer dans présents, si présent : supprimer de la liste
            //bienvenue, au revoir, accès interdit...
            pc.printf("\r\nCode ? ");
            pc.scanf("%s",code);
        }
        if(strcmp(code,"--------"))
        {   if(AccesRFIDautorise(code))
            {   if(!strcmp(lES,"ES"))
                {   if(!EstPresentDansLeParking(code)) 
                    {   Bienvenue(seconds,dateheure,code);
                        lcd.printf( 0, "Bienvenue \r" );
                    }
                    else
                    {   Aurevoir(seconds,dateheure,code);
                        lcd.printf( 0, "Au revoir \r" );                  
                    }
                }
                else if(!strcmp(lES,"E")) {Bienvenue(seconds,dateheure,code); lcd.printf( 0, "Bienvenue \r" );}
                else if(!strcmp(lES,"S")) {Aurevoir(seconds,dateheure,code);  lcd.printf( 0, "Au revoir \r" );}                
                OuvrirBarriere();
                //wait(2);
                while(letat&0x30){variateur.lectureBoucles(letat);wait(1);}
                wait(1);
                FermerBarriere();
            }
            else {pc.printf("\r\nACCES INTERDIT");lcd.printf( 0, "Acces interdit \r" );}
            if(!strcmp(lES,"ES"))
            {   //pc.printf("\r\n%d places disponibles",lplaces-nbPresents);
                AffichePresents();
            }
            pc.printf("\r\n Compteur : %d\r\n",compteur);
        }
}
        wait(2);
        //EnregistreAccesRFID();
    }
} 
//JF

bool demandeMontee(){
    return variateur.ecritureRegistre(0x1665,0x0001);
}
bool demandeDescente(){
    return variateur.ecritureRegistre(0x1666,0x0001);
}
/*
void receptionPC(){
    char c=pc.getc();
    pc.printf("Caractere recu : %c\n\r",c);
    if((c=='m')||(c=='M')) {
        bool rep=demandeMontee();
        if(rep) pc.printf("Commande de montee prise en compte !\n\r");
        else pc.printf("Probleme de commande !\n\r");
    }
    if((c=='d')||(c=='D')) {
        bool rep=demandeDescente();
        if(rep) pc.printf("Commande de descente prise en compte !\n\r");
        else pc.printf("Probleme de commande !\n\r");
    }
}*/
void receptionBarCode()
{   barCode.scanf("%s",code);
    pc.printf("%s\r\n",code);    
}
/*bool identification(){
    if(identRFID()) return true;
    else return false;
}*/

void initLecteurRFID(void){
    uint32_t versiondata = 0;
    pc.printf ("Recherche lecteur RFID !\r\n");
    while (1) {
        nfc.begin();
        versiondata = nfc.getFirmwareVersion();
        if (! versiondata) {
            pc.printf("Pas de lecteur RFID trouve !\r\n");
            wait_ms(500);
        } else {
            break;
        }
    }
        // Got ok data, print it out!
    pc.printf ("Lecteur RFID detecte : PN5%02X , Firmware ver. %d.%d\n\r",
               (versiondata>>24) & 0xFF,
               (versiondata>>16) & 0xFF,
               (versiondata>>8) & 0xFF);
        // Set the max number of retry attempts to read from a card
        // This prevents us from waiting forever for a card, which is
        // the default behaviour of the PN532.
    nfc.setPassiveActivationRetries(0xFF);
        // configure board to read RFID tags
    nfc.SAMConfig();
}

bool identRFID(char code[9]) //AABBCCD+'\0'
{
    bool success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;  // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
        // configure board to read RFID tags
    nfc.SAMConfig();
        // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
        // 'uid' will be populated with the UID, and uidLength will indicate
        // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
    //printf ("\n\r");
    if (success) {
        //pc.printf("Tag detecte !\n\r");
        //pc.printf("Taille identifiant : %d bytes\n\r", uidLength);
        //pc.printf("Identifiant du Tag : ");
        //for (uint8_t i=0; i < uidLength; i++) pc.printf(" 0x%02X", uid[i]);
        //pc.printf("\n\r");
        sprintf(code,"%02X%02X%02X%02X", uid[0],uid[1],uid[2],uid[3]);
        //if((uid[0]==tagAutorise[0])&&(uid[1]==tagAutorise[1])&&(uid[2]==tagAutorise[2])&&(uid[3]==tagAutorise[3])) return 1;
        //else return 0;
        return 1;
    } 
    else {
        // PN532 probably timed out waiting for a card
        //pc.printf("Attente d'un Tag ...\n\r");
        return 0;
    } 
}
//!JF
void activationQR()
{
    barCodeTrig=1;
    wait(0.05);
    barCodeTrig=0;

}
/*
Du point de vue du client distant : envoi >, reception <
Messagerie identique communication USB (terminal) ou TCP
Identifiants et données en Hexadécimal codé ASCII

I00000100R0L1:0100000000000000  > Ouvrir barrière
I00000100R0L1:0200000000000000  > Fermer barrière
I00000200R1L1:0000000000000000  > Requête état barrière (0-0-PésenceAval-PésenceAmont-FinCourseBas-FinCourseHaut-ExtérieurDescente-ExtérieurMontée)
I00000200R0L1:0000000000000000  < Etat capteurs barrière
I00000300R1L8:0000000000000000  > Requête Nom barrière 8 caractères
I00000300R0L8:0000000000000000  < Nom barrière 8 caractères
I00000400R0L8:0000000000000000  > Modification Nom barrière 8 caractères
I00000500R0L8:0000000000000000  > Modification IP (ipV4 : 4 premiers octets seulement)
I00000600R1L5:0000000000000000  > Requête date et heure
I00000600R0L5:0000000000000000  < Date et h Heure j-m-a-h-mn
I00000700R0L5:0000000000000000  > Mise à la date et l'heure j-m-a-h-mn
I00000A00R0L8:0000000000000000  > Ajouter autorisation (ID 4 octets puis j-m début, j-m fin)
I00000B00R0L4:0000000000000000  > Retirer autorisation (ID 4 octets)
I00000C00R0L0:0000000000000000  > Effacer toutes les autorisations
I00000D00R1L2:0000000000000000  > Requête nb lignes historique
I00000D00R0L2:0000000000000000  < Nb lignes historique max 3000/j (2 octets)
I00000E00R0L0:0000000000000000  > Effacer Historique complet
I00010001R1L8:0000000000000000  > Requête ligne Historique numéro 0001
I00010001R0L8:0000000000000000  < Ligne numéro 0001 j-m-h-mn+ID : j en complément à 2 pour une sortie
I00020001R0L2:0000000000000000  > Effacer ligne Historique numéro 0001
I00010002R1L8:0000000000000000  > Requête ligne Historique numéro 0002
I00010002R0L8:0000000000000000  < Ligne numéro 0002 j-m-h-mn+ID : j en complément à 2 pour une sortie
I00020002R0L2:0000000000000000  > Effacer ligne Historique numéro 0002
I00010003R1L8:0000000000000000  > Requête ligne Historique numéro 0003
I00010003R0L8:0000000000000000  < Ligne numéro 0003 j-m-h-mn+ID : j en complément à 2 pour une sortie
I00020003R0L2:0000000000000000  > Effacer ligne Historique numéro 0003
I00000F00R0L2:0000000000000000  > Nombre de places (2 octets)
I00001000R0L1:0000000000000000  > Entrée=0, Sortie=1, E/S=2
I00001100R1L2:0000000000000000  > Requête valeur compteur
I00001100R0L2:0000000000000000  < Valeur compteur
I00001200R0L0:0000000000000000  > RAZ compteur

Historique :    //<:entrée, >:sortie
01/01/2000  07:11   AABBCCDE    <
01/01/2000  08:00   AABBCCDD    <
01/01/2000  17:01   AABBCCDE    >
01/01/2000  19:08   AABBCCDD    >

AccèsRFID : //date/heure début puis fin
AABBCCDD    01/02/2002  05/02/2002
AABBCCDE    01/02/2002  01/04/2002

AccèsQRcode :
AABB0011    01/02/2002  01/07/2002
AABB0012    01/02/2002  01/02/2003

Config :
DATEHEURE   07/06/2019 13:55:00
IP      172.20.182.69
PORT        6942
NOM     Est
NBPLACES    50
ENTREE/SORTIE   ES
COMPTEUR    3
AFFICHETRAMES   0
LECTURE(C/R/Q)  C



*/