#include "MX2ModBus.h"

MX2ModBus::MX2ModBus()
{
    adresse=1;
    ModBus=new Serial(p28,p27/*,9600*/);
    ModBus->baud(9600);
    De=new DigitalOut(p25);
    Re=new DigitalOut(p26);
}

void MX2ModBus::setAdresse(char adresseVariateur){
    adresse=adresseVariateur;
}

char MX2ModBus::getAdresse(){
    return adresse;
}

bool MX2ModBus::lectureBoucles(char &reponse){
    char trame_lecture_entrees[8]= {1,1,0,6,0,7,0x9d,0xc9};
    int j=0;
    unsigned short CRC;
    bool CRCok=false;
    trame_lecture_entrees[0]=adresse;
    CRC=calculCRC(trame_lecture_entrees,6);
    trame_lecture_entrees[6]=CRC&0x0FF;
    trame_lecture_entrees[7]=((CRC>>8)&0x0FF);
    do{
        Re->write(1);
        De->write(1);
        for(int i=0; i<8; i++) ModBus->putc(trame_lecture_entrees[i]);
        wait(0.005);//0.01
        Re->write(0);
        De->write(0);
        CRCok=lecture_reponse(6);  
        CRC=calculCRC(trame_reponse,4);
        if(((CRC&0x0FF)==trame_reponse[4])&&(((CRC>>8)&0x0FF)==trame_reponse[5])){
            CRCok=true;
            break;
            }
        else{
            while(ModBus->readable()) ModBus->getc(); // pour vider le buffer
            j++;
            CRCok=false;
        }
        wait(0.1);
    } while(j<10);
    if(CRCok) reponse=trame_reponse[3];
    else reponse=0;
    return CRCok;
}

bool MX2ModBus::ecritureRegistre(unsigned short adresseRegistre, unsigned short valeurRegistre){
    char trame_ecriture_entrees[11]= {0x01,0x10,0x16,0x65,0x00,0x01,0x02,0x00,0x01,0x19,0xA4};
    int j=0;
    unsigned short CRC;
    bool CRCok=false;
    trame_ecriture_entrees[0]=adresse;
    trame_ecriture_entrees[2]=(adresseRegistre>>8)&0x0FF;
    trame_ecriture_entrees[3]=adresseRegistre&0x0FF;
    trame_ecriture_entrees[7]=(valeurRegistre>>8)&0x0FF;
    trame_ecriture_entrees[8]=valeurRegistre&0x0FF;
    CRC=calculCRC(trame_ecriture_entrees,9);
    trame_ecriture_entrees[9]=CRC&0x0FF;
    trame_ecriture_entrees[10]=((CRC>>8)&0x0FF);
    do{
        Re->write(1);
        De->write(1);
        for(int i=0; i<11; i++) ModBus->putc(trame_ecriture_entrees[i]);
        wait(0.013);
        Re->write(0);
        De->write(0);
        CRCok=lecture_reponse(8);
        CRC=calculCRC(trame_reponse,6);
        if(((CRC&0x0FF)==trame_reponse[6])&&(((CRC>>8)&0x0FF)==trame_reponse[7])){
            CRCok=true;
            break;
            }
        else{
            while(ModBus->readable()) ModBus->getc(); // pour vider le buffer
            j++;
            CRCok=false;
        }
        wait(0.1);
    } while(j<10);
    return CRCok;
}

bool MX2ModBus::lecture_reponse(char nbOctets){
    int j=100;
    for (int i=0; i<nbOctets; i++) {
        while(!ModBus->readable()){
            j--;
            if(j<0) return false;
            wait(0.001);
        }
        trame_reponse[i]=ModBus->getc();
    }
    return true;
}

unsigned short MX2ModBus::calculCRC(char *donnees,char nbOctets){
    unsigned short code=0xFFFF;
    char i,j;
    bool LSB;
    for(i=0;i<nbOctets;i++){
        code^=donnees[i];
        for(j=0;j<8;j++){
            LSB=code&0x0001;
            code=code>>1;
            if(LSB) code^=0xA001;
        }
    }
    return code;
}     