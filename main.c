#include <xc.h>
#define _XTAL_FREQ 4000000

#pragma config FOSC=INTOSCIO, WDTE=OFF, PWRTE=ON, MCLRE=OFF
#pragma config BOREN=ON, LVP=OFF, CPD=OFF, CP=OFF

// ===== CONSTANTES =====
#define DUREE_RELAIS_ON_SEC   5
#define DUREE_RELAIS_OFF_SEC  15
#define TMR0_RELOAD           16   // ~1 seconde

#define RELAIS_ON()   do { RB1 = 0; RB2 = 1; } while(0)
#define RELAIS_OFF()  do { RB1 = 1; RB2 = 0; } while(0)
#define LED_JOUR()    do { RB4 = 0; RB5 = 1; } while(0)
#define LED_NUIT()    do { RB4 = 1; RB5 = 0; } while(0)

// ===== VARIABLES =====
volatile unsigned char tmr         = 0;
volatile unsigned char sec         = 0;
volatile unsigned char flag_sec    = 0;
volatile unsigned char flag_bouton = 0;

unsigned char etat_relais = 1;
unsigned char etat_jour   = 0;
unsigned char porte_etat  = 1;

// ===== SERVO =====
void servo_open(void) {
    unsigned char i;
    for(i = 0; i < 50; i++) {
        RB3 = 1; __delay_us(1000);
        RB3 = 0; __delay_ms(19);
    }
}

void servo_close(void) {
    unsigned char i;
    for(i = 0; i < 50; i++) {
        RB3 = 1; __delay_us(2000);
        RB3 = 0; __delay_ms(18);
    }
}

// ===== ISR ? courte et rapide =====
void __interrupt() isr(void) {
    if(T0IF) {
        T0IF = 0;
        if(++tmr >= TMR0_RELOAD) {
            tmr = 0;
            sec++;
            flag_sec = 1;
        }
    }
    if(INTF) {
        INTF = 0;
        flag_bouton = 1;   // traitement dans main()
    }
}

// ===== MAIN =====
void main(void) {
    CMCON = 0x07;

    TRISAbits.TRISA0 = 1;   // LDR entrée
    TRISB = 0b00000001;     // RB0 entrée, reste sortie

    OPTION_REGbits.nRBPU = 0;  // pull-up PORTB
    OPTION_REGbits.T0CS  = 0;
    OPTION_REGbits.PSA   = 0;
    OPTION_REGbits.PS2   = 1;
    OPTION_REGbits.PS1   = 1;
    OPTION_REGbits.PS0   = 1;

    TMR0 = 0; T0IF = 0; T0IE = 1;
    INTE = 1; INTF = 0;
    GIE  = 1;

    // État initial : jour
    LED_JOUR();
    RELAIS_ON();
    servo_open();

    while(1) {
        // ?? Gestion seconde ??
        if(flag_sec) {
            flag_sec = 0;
            if(etat_jour) {
                if(etat_relais) {
                    if(sec >= DUREE_RELAIS_ON_SEC) {
                        RELAIS_OFF();
                        etat_relais = 0; sec = 0;
                    }
                } else {
                    if(sec >= DUREE_RELAIS_OFF_SEC) {
                        RELAIS_ON();
                        etat_relais = 1; sec = 0;
                    }
                }
            }
        }

        // ?? Gestion bouton ??
        if(flag_bouton) {
            flag_bouton = 0;
            __delay_ms(50);          // anti-rebond hors ISR
            if(RB0 == 0) {
                if(porte_etat) { servo_close(); porte_etat = 0; }
                else           { servo_open();  porte_etat = 1; }
            }
        }

        // ?? Lecture LDR ??
        if(RA0 == 0 && etat_jour == 0) {   // passage NUIT ? JOUR
            etat_jour = 1;
            LED_JOUR();
            if(!porte_etat) { servo_open(); porte_etat = 1; }
            sec = 0; tmr = 0;
            RELAIS_ON(); etat_relais = 1;
        }
        else if(RA0 == 1 && etat_jour == 1) { // passage JOUR ? NUIT
            etat_jour = 0;
            LED_NUIT();
            servo_close(); porte_etat = 0;
            RELAIS_OFF();
        }
    }
}