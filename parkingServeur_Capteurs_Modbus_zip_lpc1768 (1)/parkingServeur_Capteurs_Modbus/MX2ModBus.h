#include "mbed.h"
class MX2ModBus {
public :
    MX2ModBus();
    void setAdresse(char adresseVariateur);
    char getAdresse();
    bool lectureBoucles(char &reponse);
    bool ecritureRegistre(unsigned short adresseRegistre, unsigned short valeurRegistre);
private :
    char adresse;
    Serial *ModBus;
    DigitalOut  *De,*Re;
    char trame_reponse[10];
    bool lecture_reponse(char nbOctets);
    unsigned short calculCRC(char *donnees,char nbOctets);     
};