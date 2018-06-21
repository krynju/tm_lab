#include <msp430.h>

#define TxD BIT4
#define RxD BIT5

void serialize_table();
void send_table();
void move_ship(int x_mov, int y_mov);
void move_bullets();
void shoot_bottom();
void shoot_top();

char tx_buff[255];
char rx_buff[30];

const int y_size=10;	//map size
const int x_size=20;
int y_ship=0;			//ship cords
int x_ship=0;

char table[y_size][x_size];	//map tiles


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
    //IE1 = UTXIE0;// | URXIE0;
    IFG1 &= ~UTXIFG0;

    // DMA Init
	DMACTL1 |= DMAONFETCH;

	// Kanal 0 DMA - odbiornik
	DMA0SA = &U0RXBUF;
	DMA0DA = rx_buff;
	DMA0SZ = 0x001E;
	DMACTL0 |= DMA0TSEL_3;
	DMA0CTL |= DMADT_0 | DMADSTBYTE | DMASRCBYTE| DMALEVEL | DMAEN | DMAIE | DMADSTINCR_3;

//	// Kanal 1 DMA - nadajnik
	DMA1SA = &tx_buff[0];
	DMA1DA = &U0TXBUF;
	DMA1SZ = 0x10;
	DMACTL0 |= DMA1TSEL_4;
	DMA1CTL |= DMADT_0 | DMADSTBYTE | DMASRCBYTE | DMALEVEL | DMAEN | DMAIE | DMASRCINCR_3;


    _BIS_SR(GIE);  //wlacza przerwania


	//inicjalizacja pola
    int i=0;
    int j=0;
    for(i = 0;i<y_size;i++){
    	for(j =0;j<x_size;j++){
    		table[i][j]=' ';
    	}
    }

	//inicjalizacja statku
	move_ship(1,4);

    while(1){
    	move_bullets();
		int i;
		DMA0CTL &= ~DMAEN;
		for(i=0;i<30-DMA0SZ;i++){//analyze received bytes
			switch(rx_buff[i]){
				case 'w':
					move_ship(0,-1);
					break;
				case 's':
					move_ship(0,1);
					break;
				case 'j':
					shoot_top();
					break;
				case 'k':
					shoot_bottom();
					break;
			}
		}

		DMA0DA = &rx_buff[0];
		DMA0SZ = 0x001E;
		DMA0CTL |= DMAEN;
		
    	send_table();
    	_BIS_SR(LPM0_bits);	//sleep
    }
    return 0;
}

void serialize_table(){
	char newline = 12;
	char endline[]={'\n','\r'};
	int i=0;
	int x,y;
	tx_buff[0] = newline;
	for (y=0; y<y_size; y++){

		for (x=0; x<x_size; x++){

			tx_buff[ 1 + x + y*(x_size + 2) ] = table[y][x];

		}
		tx_buff[(y+1)*(x_size+2) - 1] = endline[0];
		tx_buff[(y+1)*(x_size+2) ] = endline[1];
	}
}

void send_table() {
	serialize_table();
	
	DMA1CTL &= ~DMAEN;
	DMA1SA = &tx_buff[0];
	DMA1SZ = (x_size+2)*y_size + 1;
	DMA1CTL |= DMAEN;
	ME1 |= UTXE0;
	IFG1 |= UTXIFG0;
	IFG1 &= ~UTXIFG0;
}

void move_ship(int x_mov, int y_mov){
	int i;
	int j;
	for(i=y_ship; i<y_ship+3; i++){
		for(j=x_ship; j<x_ship+3; j++){
			table[i][j]=' ';
		}
	}
	x_ship +=x_mov;
	y_ship +=y_mov;

	if(y_ship<0) y_ship=0;
	if(y_ship>y_size-4) y_ship=y_size-3;

	for(i=y_ship; i<y_ship+3; i++){
		for(j=x_ship; j<x_ship+3; j++){
			table[i][j]='@';
		}
	}
}

void move_bullets(){
	int i;
	int j;
	for(i=0; i<y_size; i++){ //ostatnia kolumna
		if(table[i][x_size-1]=='X'){
			table[i][x_size-1]=' ';
		}
	}
	for(i=0; i<y_size; i++){ //przedostatnia kolumna
		if(table[i][x_size-2]=='-'){
			table[i][x_size-2]=' ';
			table[i][x_size-1]='X';
		}
	}
	for(i=0; i<y_size; i++){ //reszta od prawej do lewej
		for(j=x_size-3; j>0; j--){
			if(table[i][j]=='-'){
				table[i][j]=' ';
				table[i][j+1]='-';
			}
		}
	}
}

void shoot_bottom(){
	if(table[y_ship+2][x_ship+3]=='-'){
		return;
	}
	else{
		table[y_ship+2][x_ship+3]='-';
	}
}

void shoot_top(){
	if(table[y_ship][x_ship+3]=='-'){
		return;
	}
	else{
		table[y_ship][x_ship+3]='-';
	}
}





#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A0 (void) {
	TAIV = 0x00;
	_BIC_SR_IRQ(LPM0_bits);
}


#pragma vector=DACDMA_VECTOR
__interrupt void DMA (void) {
	if(DMA0CTL & DMAIFG){
	    DMA0DA=rx_buff;
		DMA0CTL &= ~DMAIFG;
		DMA0CTL |= DMAEN;


	} else if (DMA1CTL & DMAIFG) {

		DMA1CTL &= ~DMAREQ;
		DMA1CTL &= ~DMAIFG;
		ME1 &= ~UTXE0;
		DMA1CTL |= DMAEN;
	}
}