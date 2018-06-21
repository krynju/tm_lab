#include <msp430.h>

#define CTS BIT2
#define TxD BIT4
#define RxD BIT5

int tx_buff_put(char *byte,char size);
int tx_buff_get(char *byte);

int rx_buff_put(char byte);
int rx_buff_get(char *byte);

char tx_max_size = 255;
char tx_buff[255];
char tx_size = 0;
char tx_newest = 0;
char tx_oldest = 0;

char rx_max_size =255;
char rx_buff[255];
char rx_size = 0;
char rx_newest = 0;
char rx_oldest = 0;

char tx_idle = 1;//1-idle,0-sending

const int y=10;
const int x=20;
int y_ship=0;
int x_ship=0;
char table[y][x];

void send(char *data,char size) {
	__bic_SR_register(GIE);
	tx_buff_put(data,size);
	char byte;
	if (tx_idle || (!tx_idle && tx_size==255)) {
		tx_buff_get(&byte);
		TXBUF0=byte;
		U0TCTL |= UTXE0;
		tx_idle = 0;
	}
	__bis_SR_register(GIE);
}


int main(void) {
    WDTCTL = WDTPW | WDTHOLD;
    TACTL = TASSEL_1 + MC_1;
    TAR = 0;
    TACCR0 = 8192/2;
    TACCR1 = 0;
    TACCR2 = 0;
    TACCTL0 = CCIE;
	P3DIR |= TxD;
    P3SEL |= RxD | TxD;
	BCSCTL1 = RSEL2 | RSEL0;
	DCOCTL = DCO0;
    U0CTL = CHAR;
    U0TCTL = SSEL1 | SSEL0 | TXEPT;
    U0RCTL = URXEIE;
    U0BR1 = 0x00;
	U0BR0 = 0x09;
    U0MCTL = 0x08;

    ME1 = UTXE0 | URXE0;
    IE1 = UTXIE0 | URXIE0;
    IFG1 &= ~UTXIFG0;
	
    _BIS_SR(GIE);  //wlacza przerwania


	//inicjalizacja pola
    int i=0;
    int j=0;
    for(i = 0;i<y;i++){
    	for(j =0;j<x;j++){
    		table[i][j]=' ';
    	}
    }

	//initial move to 5,5
	move_ship(1,4);

    while(1){

    	move_bullets();
		char byte;
		while(!rx_buff_get(&byte)){//analyze received bytes
			switch(byte){
				case 'w':
					move_ship(0,-1);
					break;
				case 's':
					move_ship(0,1);
					break;
				case 'f':
					shoot();
					break;
			}
		}
		//wykrywanie wcisnietych przyciskow
    	send_table();
    	_BIS_SR(LPM0_bits);	//sleep
    }
    return 0;
}

void move_bullets(){
	int i;
	int j;

	for(i=0; i<y; i++){ //ostatnia kolumna
		if(table[i][x-1]=='X'){
			table[i][x-1]=' ';
		}
	}
	for(i=0; i<y; i++){ //przedostatnia kolumna
		if(table[i][x-2]=='-'){
			table[i][x-2]=' ';
			table[i][x-1]='X';
		}
	}

	for(i=0; i<y; i++){ //reszta od prawej do lewej
		for(j=x-3; j>0; j--){
			if(table[i][j]=='-'){
				table[i][j]=' ';
				table[i][j+1]='-';
			}
		}
	}
}

void shoot(){
	if(table[y_ship+1][x_ship+3]=='-'){
		return;
	}
	else{
		table[y_ship+1][x_ship+3]='-';
	}
}

void move_ship(int x_mov, int y_mov){
	int i;
	int j;
	//clear last position
	for(i=y_ship; i<y_ship+3; i++){
		for(j=x_ship; j<x_ship+3; j++){
			table[i][j]=' ';
		}
	}

	//zmien pozycje
	x_ship +=x_mov;
	y_ship +=y_mov;

	if(y_ship<0) y_ship=0;
	if(y_ship>y-4) y_ship=y-3;

	//wpisz w nowa pozycje
	for(i=y_ship; i<y_ship+3; i++){
		for(j=x_ship; j<x_ship+3; j++){
			table[i][j]='@';
		}
	}
}

void send_table(){
	char newline = 12;
	char endline[]={'\n','\r'};
	int i=0;
	send(&newline,1);
	for(i=0;i<y;i++){
		send(table[i],x);
		send(endline,2);
	}
}

#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A0 (void) {
	TAIV = 0x00;
	_BIC_SR_IRQ(LPM0_bits);
}

#pragma vector=USART0RX_VECTOR
__interrupt void USART0_RX (void) {
	char received = RXBUF0;
	rx_buff_put(received);
}

#pragma vector=USART0TX_VECTOR
__interrupt void USART0_TX (void) {
	U0TCTL &= ~UTXE0;
	char byte;
	if (!tx_buff_get(&byte)) {
		TXBUF0=byte;
		U0TCTL |= UTXE0;
		return;
	}
	tx_idle=1;
}

int tx_buff_put(char *byte,char size) {
    if ((tx_size) == tx_max_size) {
        return -1; //buff full
    }
    int i=0;
    for (i = 0;i < size; i++) {
        tx_buff[tx_newest] = byte[i];
        tx_newest++;
        if (tx_newest == tx_max_size) {
            tx_newest = 0;
        }
        tx_size++;
    }
	return 0;
}

int tx_buff_get(char *byte){
	if (tx_size == 0){
		return -1; //buff empty
	}
	*byte=tx_buff[tx_oldest];
	tx_oldest++;
	if(tx_oldest==tx_max_size){
		tx_oldest = 0;
	}
	tx_size--;
	return 0;
}

int rx_buff_put(char byte){
	if (rx_size == rx_max_size){
		return -1; //buff full
	}
	rx_buff[rx_newest]=byte;
	rx_newest++;
	if(rx_newest==rx_max_size){
		rx_newest = 0;
	}
	rx_size++;
	return 0;
}

int rx_buff_get(char *byte){
	if (rx_size == 0){
		return -1; //buff empty
	}
	*byte=rx_buff[rx_oldest];
	rx_oldest++;
	if(rx_oldest==rx_max_size){
		rx_oldest = 0;
	}
	rx_size--;
	return 0;
}