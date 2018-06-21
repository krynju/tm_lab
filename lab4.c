#include <msp430.h>

/*Generator button*/
char key_service = 0;
char prev_state = 0;
char new_state = 0;
char debounce_counter = 0;
char key_state_changed_to_pressed = 0;


/*Dynamic display*/
char display_service = 0;
char digit_selected = 0x01;
int random_value = 0;

/*Tick interrupt*/
char *countdown_table[]={&key_service,&display_service};


/*Declarations*/
char get_digit(int a,int position);
void capture();
char get_key_state();
void display_routine(int number);
void key_debounce_routine();



int main(void) {
    WDTCTL = WDTPW | WDTHOLD;

	/*Inicjalizacja timera*/
    TACTL = TASSEL_1 + MC_1;
    TAR = 0;		
    TACCR0 = 1000;		
    TACCR1 = 0;				
    TACCR2 = 0;				
    TACCTL1 = CCIE;			

	/* Inicjalizacja portow */
	P1IE  = 0xFF;	//button
	P1IES = 0xFF;
    P2DIR = 0xFF;	//select
    P3DIR = 0xFF;	//value
	
    _BIS_SR(GIE);  //wlacza przerwania

    while(1){

    	display_routine(random_value);
    	key_debounce_routine();
		
    	if (key_state_changed_to_pressed){
    		random_value=TACCR2;
    	}
		
    	_BIS_SR(LPM0_bits);	//sleep
    }
    return 0;
}

/*Wyswietla podana w argumencie liczbe (3 cyfrowa) na wyswietlaczu dynamicznym
/ za kazdym wywolaniem wyswietla sie kolejna cyfra
/ zmiana czestotliwosci poprzez dostosowanie wartosci display_service */
void display_routine(int number){
	if (!display_service){
		P3DIR = 0x00;
		P2OUT = ~digit_selected;
		P3OUT = 0xF0 + get_digit(number,digit_selected);
		P3DIR = 0xFF;

		display_service = 4;				//delay

		digit_selected=digit_selected<<1;	//shift
		if (digit_selected==0x08){			//overflow
			digit_selected=0x01;
		}
	}
}
/*Aktualizuje stan przycisku 
/dostosowanie odstepow pomiedzy sprawdzaniem stanu poprzez key_service
/dostosowanie liczby probek poprzez zmiane warunku na debounce_counter */
void key_debounce_routine(){
	if (!key_service){
		key_state_changed_to_pressed = 0;
		new_state = P1IN!=0xFF ? 1 : 0;

		if (new_state != prev_state){
			debounce_counter++;
		}
		else {
			debounce_counter = 0;
		}

		if (debounce_counter == 10){
			prev_state = new_state;
			if (prev_state){
				key_state_changed_to_pressed = 1;
			}
		}
		key_service = 2;
	}
}

/*Zwraca potwierdzony stan przycisku*/
char get_key_state(){
	return prev_state;
}

/*Zapisuje aktualna wartosc zegara A do rejestru TACCTL2*/
void capture(){
	TACCTL2=CM_1+CCIS_2+SCS+CAP;
	TACCTL2=CM_1+CCIS_3+SCS+CAP;
}

/*Zwraca cyfre liczby dziesietnej w zaleznosci od podanej pozycji
/ position binarnie - 001, 010, 100
/ tylko dla liczb 3 cyfrowych*/
char get_digit(int a,int position){
	switch (position){
		case 0x01:
			return (a%10);
		case 0x02:
			a = a/10;
			return a%10;
		case 0x04:
			return a/100;
	}
	return 0;
}


#pragma vector=TIMERA1_VECTOR
__interrupt void Timer_A1 (void) {
	TAIV = 0x00;
	_BIS_SR(GIE);  //wlacza przerwania
	TACCR1=TACCR1+50;
	if (TACCR1==1000)TACCR1 = 0;

	if(*(countdown_table[0])){		//key_service
		(*countdown_table[0])--;
	}
	if(*(countdown_table[1])){		//display_service
		(*countdown_table[1])--;
	}
	_BIC_SR_IRQ(LPM0_bits);			//sleep off after interrupt
}

#pragma vector=PORT1_VECTOR
__interrupt void Port1ISR (void){
	capture();
	P1IFG = 0x00;
}