;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; aducgd1.inc	is an include file for the ADuC832 version 3 microcontroller
; boards (MGM/dp and EMSYS JDW).
;
; Written by Roggemans M (MGM) 07/2016 V0
;
; RESERVED REGISTERS:
; -All routines specify the use of registers as input-, output- and used registers
; -DO NOT USE MEMORY LOCATIONS 73h-7fh: used by arithmatic routines 
; -DO NOT USE MEMORY LOCATIONS 60h-72h: used by display driver
; -Always select register bank 0 when calling functions.
;
; To search the aducgdx.inc file for specific information, use [ctrl F], type the name 
; of the function followed by [:] and press enter (ea: ctrl F delaya0k05s: enter). 
;
; The file is composed of:
; -registerdefinitions for the ADuC832BSZ microcontroller
;			
; -math subroutines:
;     	mul32		multiplication of 2 32 bit integers.
;      mul16       	multiplication of 2 16 bit integers.
;      div32       	division of 2 32 bit integers.
;      div16       	division of 2 16 bit integers.
;      add32       	addition of 2 32 bit integers.
;      add16       	addition of 2 16 bit integers.
;      sub32       	substraction of 2 32 bit integers.
;      sub16      	substraction of 2 16 bit integers.
;      hexbcd16    	conversion of a 16 bit hex(binary) integer in a 24 bit BCD integer.
;      hexbcd8     	conversion of a 8 bit hex(binary) integer in a 8 bit BCD integer (limited to 99.
;      bcdhex16    	conversion of a 16 bit BCD (9999) integer in a 16 bit hex (binary) integer.
;      bcdhex8     	conversion of a 8 bit BCD (99) integer in a 8 bit hex (binary) integer.
;
; -aducgd routines for the graphical LCD color display (TFT) (240x320 RGB pixels):
; WARNING: these functions disrupt IIC settings!
; WARNING: these functions require register bank 0 to be selected!
; 	tftinit		enable tft display and write display with black pixels
;				set standard bg color with black (bg=background)
;				set standard fr color with white (fr=front)
; 	tftclear		sets all display pxels to background color
; 	tftsetbgcolor		sets background color
; 	tftsetfrcolor		sets front color
; 	tftclear		sets entire display in bgcolor
; 	tftdrawbgc		sets designated window in bgcolor
; 	tftdrawfrc		sets designated window in frcolor
; 	tftdrawc		sets designated window in color
; 	tftoutchar		prints 1 ASCII coded symbol on the display (uses tftxpos and tftypos)
; 	tftoutcharf		prints 1 ASCII coded symbol on the display (uses tftxh, tftxl, tfty)
; 	tftoutmsga		prints ascii string from memory
; 	tftoutnib		prints lower nibble in accu as 1 digit hex value on display
; 	tftoutbyte		prints 8 bit value in accu as 2 digit hex value on display
;
; -iic routines bitbanging an IIC interface
; WARNING: these functions disrupt TFT settings (graphical LCD display)
;       iicinit     klaar zetten van de iic voor gebruik
;       iicstart    verzenden van een start conditie
;       iicstop     verzenden van een stop conditie
;       iicoutbyte  verzenden van 1 byte vanuit de accu, er wordt een negende klok
;                   verzonden, in de carry wordt de ack van de slave weergegeven
;       iicinbyteack  leest een byte van de iic bus en genereerd een ack op de negende klokpuls
;       iicinbytenack  leest een byte van de iic bus en genereerd een nack op de negende klokpuls
;
; -sio routines voor het gebruik van de seriële poort
;       initsio     zal de seriele poort klaar zetten voor gebruik. 
;       siooutchar  is een subroutine die een karakter via de seriele interface naar buiten stuurt. 
;       siooutnib   verzend de laagste vier bits naar de seriele poort onder de vorm van een ascii code.
;       siooutbyte  zet de hex inhoud van de accu op de seriele poort. als flow control wordt xonxoff gebruikt.
;       siodispdptr zet de hex inhoud van de dptr op het scherm op de huidige plaats van de cursor.
;       siooutmsga  is een subroutine, die de ascii codes vanaf het adres in de dptr, tot aan de code 00h, op het scherm zet. 
;       xonxoff     laat het gebruik van software handshake toe. 
;       sioinchar   blijft wachten totdat er een karakter ontvangen werd door de seriele interface. 
;       sioinbyte   leest twee ascii codes binnen en vormt ze om tot een hex byte.
;       sioinbufa   leest een ascii buffer binnen via de seriele poort. 
;
; -miscellanious
; 	delaya0k05s 	Is a software delay of a*0,05s. 
;			The routines adapts in relation to the PLLCON value.
;
; 	getadc	    	meet de spanning op de analoge input waarvan het nummer in de staat.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; TFT subroutines.
; The TFT display used is a 2.2" QVGA 240x320 pixels color display. It uses a
; SPI interface. The LCD controller on the display is the ILI9341 driver.
;
; *****WARNING*****
; The ADuC832 hardware SPI is used to interface with the TFT display. SPI and
; IIC interfaces on the ADuC share I/O pins. The user must be aware that using the
; IIC interface will disrupt SPI functions and vice versa. 
;
; *****WARNING*****
; The functions use dedicated registers for normal operation (60h-72h).
; These are reserved and should not be altered by user software.
; The functions demand register bank 0 be selected!
;
; *****WARNING*****
; PLLCON is changed in order to run the system @16.777216 MHz
; This will change speed of all peripherals!!
; 
; Original version written by Roggemans M (MGM) 06/2016 for ADuCV3 board
;
; user functions:
; -tftinit		enable tft display and write display with black pixels
;			set standard bg color with black (bg=background)
;			set standard fr color with white (fr=front)
; -tftclear		sets all display pxels to background color
; -tftsetbgcolor	sets background color
; -tftsetfrcolor	sets front color
; -tftclear		sets entire display in bgcolor
; -tftdrawbgc		sets designated window in bgcolor
; -tftdrawfrc		sets designated window in frcolor
; -tftdrawc		sets designated window in color
; -tftoutchar		prints 1 ASCII coded symbol on the display (uses tftxpos and tftypos)
; -tftoutcharf	prints 1 ASCII coded symbol on the display (uses tftxh, tftxl, tfty)
; -tftoutmsga		prints ascii string from memory
; -tftoutnib		prints lower nibble in accu as 1 digit hex value on display
; -tftoutbyte		prints 8 bit value in accu as 2 digit hex value on display
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; I/O pin declarations

cs		bit	p3.6		;TFT chip select
reset		bit	p3.5		;TFT reset
dc		bit	p3.7		;tft chip select

; Memory declarations
free		equ	72h		;not used
free1		equ	71h		;not used
tftxpos	equ	70h		;register for simple line addressing
tftypos	equ	6fh		;register for simple line addressing
tftbgcolorh	equ	6eh		;register for background color high
tftbgcolorl	equ	6dh		;register for background color low
tftfrcolorh	equ	6ch		;register for front color high
tftfrcolorl	equ	6bh		;register for front color low
tftxhs		equ	6ah		;register for xhigh coordinate
tftxh		equ	6ah		;double defined
tftxls		equ	69h		;register for xlow coordinate
tftxl		equ	69h		;double defined
tftys		equ	68h		;register for y coordinate
tfty		equ	68h		;double defined
tftxhe		equ	67h		;ending address selected window xhigh
tftxle		equ	66h		;ending address selected window xlow
tftye		equ	65h		;ending address selected window y
tftcolorh	equ	64h		;color for draw function
tftcolorl	equ	63h		;color for draw function
red		equ	62h		;used for color settings
green		equ	61h		;used for color settings
blue		equ	60h		;used for color settings

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; tftinit
; -initialise SPI interface on ADuC832
; -basic settings on the TFT display module (ILI9341)
;
; WARNING: The driver will operate with all values of the PLLCON. High speed is advised
; and set by the driver!!
;
; input:	none
; output:	ADuC SPI init
; 		TFT basic settings
;		registers 60h-72h are loaded with default values
;		PLLCON set to 00h (16.777216 MHz system clock)
; used:	none
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

tftinit:	push	psw			;BACK UP REGISTERS ONTO STACK
		push	acc			;back up accu
		push	b			;needed for loop counting
		push	00h			;pointer is needed for loading memory f0h-ffh
		mov	pllcon,#00h		;set system clock to 16.777216 MHz
; on power on the tft module needs to be reset with a pulse >10us.
		setb	cs			;disable display module
		clr	reset			;hardware reset display module
		mov	a,#100			;accu used as loopcouter (should also work on ADuC841)
		djnz	acc,$			;repeat (total time should be >10us)
		setb	reset			;end hardware reset of display module
		mov	a,#3			;after reset 120ms delay is needed
		lcall	delaya0k05s		;done
; The microcontroller SPI interface needs to be enabled and configured
		mov	spicon,#00110001b
; Interrupt bit cleared, clear collision error, enable SPI, Enable master mode, clock
; idles low, trailing (rising) edge clocks data in slave, use xxxx shift clock.
; The display module allows 10MHz SPI write clock, but the SPI read clock cannot exceed 6.6MHz.
; Experiments show the shift clock is dependend on PLL settings. The display operates with fosc/4
; with system clock 16.777216 MHz. The level shifters used on the ADuCV3 board limit shift clock
; to this value.....Sorry...

; Default settings of the ILI9341:

		mov	a,#11h			;write sleep out command
		lcall	tftwrcbyte		;done

; This command demands a delay of minimal 120ms. Standard delay is used

		mov	a,#3			;delay 50ms used 3 times
		lcall	delaya0k05s		;done

		mov	a,#29h			;write display on command
		lcall	tftwrcbyte		;done

		mov	a,#3ah			;pixel format set (number of bits/pixel)
		lcall	tftwrcbyte		;done
		mov	a,#01010101b		;16 bit per pixel
		lcall	tftwrdbyte		;done
		mov	a,#36h			;memory access control
		lcall	tftwrcbyte		;send command
		mov	a,#11101000b		;my writing left to right
						;mx writing top down
						;mv row and column are excanged
						;ml reset value
						;bgr set to rgb
						;mh reset value
						;00 last 2 bits are zero
		lcall	tftwrdbyte		;send as parameter
		mov	red,#00h		;red color minimal
		mov	green,#00h		;green color minimal
		mov	blue,#00h		;blue color minimal
		lcall	tftsetbgcolor		;write colors as 16 bit value in bgcolor registers
		mov	red,#00;011111b	;red color maximum
		mov	green,#00111111b	;green color maximum
		mov	blue,#0;0011111b	;blue color maximum
		lcall	tftsetfrcolor		;write colors as 16 bit value in bgcolor registers
		lcall	tftclear		;clear=fill all pixels with background color		
		pop	00h			;restoring used registers	
		pop	b
		pop	acc
		pop	psw
		ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; tftclear
; -fill all pixels of the display with the background color
;
; input:	none
; output:	all display pixels display background color
;		reset tftxpos and tftypos
; used:	none
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

tftclear:

; We use a function tftdrawbgc to fill the display. This function needs start and ending coordinates
; of the display plane to be filled.

		mov	tftxhs,#00h		;starting address high display
		mov	tftxls,#00h		;starting address low display
		mov	tftxhe,#01h		;ending address high
		mov	tftxle,#3fh		;ending address low
		mov	tftys,#00h		;only use 8 bits for y
		mov	tftye,#efh		;see above
		mov	tftxpos,#00h		;reset tftxpos
		mov	tftypos,#00h		;reset tftypos
		lcall	tftdrawbgc		;write plane
		ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; tftsetbgcolor
; Is a subroutine that writes backround color information into dedicated registers
;
; input:	red	5 bits red color
;		green	6 bits green color
;		blue	5 bits blue color
; output:	store 2 bytes color info in tftbgcolorh and tftbgcolorl registers
; used:	none
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

tftsetbgcolor:	push	psw		;save registers onto stack
			push	acc
			push	b
			mov	a,red		;read red info
			rl	a		;shift 3 times to the left
			rl	a
			rl	a		;bits are in significant position
			mov	b,a		;store result
			mov	a,green		;read green info
			rl	a		;shift info left (3 significant bits are in the upper nibble)
			swap	a		;place them in the lower position
			anl	a,#00000111b	;clear all other bits
			orl	a,b		;merge red and 3 bits green=upper byte to be written
			mov	tftbgcolorh,a	;store data in memory
			mov	a,green		;read green again
			anl	a,#00000111b	;reset all bits except the lower 3
			swap	a		;put them in de upper nibble
			rl	a		;shift left once, now they occupy the 3 highest bits
			orl	a,blue		;merge with blue
			mov	tftbgcolorl,a	;store in memory
			pop	b
			pop	acc
			pop	psw
			ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; tftsetfrcolor
; Is a subroutine that writes front color information into dedicated registers
;
; input:	red	5 bits red color
;		green	6 bits green color
;		blue	5 bits blue color
; output:	store 2 bytes color info in tftfrcolorh and tftfrcolorl registers
; used:	none
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

tftsetfrcolor:	push	psw		;save registers onto stack
			push	acc
			push	b
			mov	a,red		;read red info
			rl	a		;shift 3 times to the left
			rl	a
			rl	a		;bits are in significant position
			mov	b,a		;store result
			mov	a,green		;read green info
			rl	a		;shift info left (3 significant bits are in the upper nibble)
			swap	a		;place them in the lower position
			anl	a,#00000111b	;clear all other bits
			orl	a,b		;merge red and 3 bits green=upper byte to be written
			mov	tftfrcolorh,a	;store data in memory
			mov	a,green		;read green again
			anl	a,#00000111b	;reset all bits except the lower 3
			swap	a		;put them in de upper nibble
			rl	a		;shift left once, now they occupy the 3 highest bits
			orl	a,blue		;merge with blue
			mov	tftfrcolorl,a	;store in memory
			pop	b
			pop	acc
			pop	psw
			ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; tftsetcolor
; Is a subroutine that writes front color information into dedicated registers
;
; input:	red	5 bits red color
;		green	6 bits green color
;		blue	5 bits blue color
; output:	store 2 bytes color info in tftcolorh and tftcolorl registers
; used:	none
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

tftsetcolor:		push	psw		;save registers onto stack
			push	acc
			push	b
			mov	a,red		;read red info
			rl	a		;shift 3 times to the left
			rl	a
			rl	a		;bits are in significant position
			mov	b,a		;store result
			mov	a,green		;read green info
			rl	a		;shift info left (3 significant bits are in the upper nibble)
			swap	a		;place them in the lower position
			anl	a,#00000111b	;clear all other bits
			orl	a,b		;merge red and 3 bits green=upper byte to be written
			mov	tftcolorh,a	;store data in memory
			mov	a,green		;read green again
			anl	a,#00000111b	;reset all bits except the lower 3
			swap	a		;put them in de upper nibble
			rl	a		;shift left once, now they occupy the 3 highest bits
			orl	a,blue		;merge with blue
			mov	tftcolorl,a	;store in memory
			pop	b
			pop	acc
			pop	psw
			ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; tftdrawbgc
; This function fills a window with the bgcolor.
;
; input:	tftxhs		high byte starting address x coordinate
;		tftxls		low byte starting address x coordinate
;		tftys		starting address y coordinate
;		tftxhe		high byte ending address x coordinate
;		tftxle		low byte ending address x coordinate
;		tftye		ending address y coordinate
; output:	window filled with tftbgcolor
; used:	none
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

tftdrawbgc:	mov	tftcolorl,tftbgcolorl
		mov	tftcolorh,tftbgcolorh
		lcall	tftdrawc
		ret
		
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; tftdrawfrc
; This function fills a window with the frcolor.
;
; input:	tftxhs		high byte starting address x coordinate
;		tftxls		low byte starting address x coordinate
;		tftys		starting address y coordinate
;		tftxhe		high byte ending address x coordinate
;		tftxle		low byte ending address x coordinate
;		tftye		ending address y coordinate
; output:	window filled with tftfrcolor
; used:	none
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

tftdrawfrc:	mov	tftcolorl,tftfrcolorl
		mov	tftcolorh,tftfrcolorh
		lcall	tftdrawc
		ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; tftdrawc
; This function fills a window with the color.
;
; input:	tftxhs		high byte starting address x coordinate
;		tftxls		low byte starting address x coordinate
;		tftys		starting address y coordinate
;		tftxhe		high byte ending address x coordinate
;		tftxle		low byte ending address x coordinate
;		tftye		ending address y coordinate
; output:	window filled with tftcolor
; used:	none
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

tftdrawc:	push	psw		;regs saved on stack
		push	acc
		push	b
		push	00h
		push	01h

; We need to load internal address pointers to define the window we will fill.

		mov	a,#2ah		;set column address limits (x coordinates)
		lcall	tftwrcbyte
		mov	a,tftxhs	;start address (high and low byte)
		lcall	tftwrdbyte
		mov	a,tftxls
		lcall	tftwrdbyte	;2 parameters 
		mov	a,tftxhe		;high address (high and low byte)
		lcall	tftwrdbyte
		mov	a,tftxle
		lcall	tftwrdbyte	;done
		mov	a,#2bh		;set page address (y coordinates)
		lcall	tftwrcbyte
		mov	a,#00h		;starting address is 00xxh
		lcall	tftwrdbyte
		mov	a,tftys
		lcall	tftwrdbyte	;write high and low
		mov	a,#00h		;ending is 00xxh
		lcall	tftwrdbyte
		mov	a,tftye
		lcall	tftwrdbyte	;done
		mov	a,#2ch		;give memory write command (sets internal pointers to starting address)
		lcall	tftwrcbyte

; we need to calculate how many pixels have to be written. By substarcting xe-xs and ye-ys we know the
; dimensions of the window. Delta Y can be used in a djnz loop counter. Delta x ma exceed 8 bit thus requiring
; a 9 bit loopcounter. By using a user flag we can speed loop-counting

		clr	c		;subb will follow
		mov	a,tftye	;it's coming soon
		subb	a,tftys	;told you so
		mov	r0,a		;store it in r0 (will become loopcounter)
		inc	r0		;0-0=0 but 1 line needs to be drawn!
; because endaddress and startaddress could be equal, de result of the substraction must be 1
; We increment the end address by 1
		mov	a,tftxle	;start with low byte
		add	a,#1
		mov	r1,a		;need this later
		mov	a,tftxhe	;but 16 bit addition
		addc	a,#00h		;done
		mov	b,a
		clr	c		;x is next for substraction
		mov	a,r1		;16 bit substraction
		subb	a,tftxls	;a=low 8 bits of loopcounter
		mov	r1,a		;and back up
		mov	a,b		;high byte substract
		subb	a,tftxhs		;done

; if a not equal to zero more then 256 pixels are in the x range. Since the maximum is 320, this  means a reload
; of the b register as loopcounter is needed. This condition is stored in f0 (user flag PSW)

		mov	c,acc.0	;bit zero = 0 then less trhan 256 loops needed

		mov	f1,c		;and in flag 1

; Let's start filling memory!!

tftdrawc0:	mov	b,r1			;load b register with delta xl
		mov	c,f1			;store f1 in f0
		mov	f0,c
	
tftdrawc1:	mov	a,tftcolorh		;write high byte to display
		lcall	tftwrdbyte
		mov	a,tftcolorl		;write low byte to memory
		lcall	tftwrdbyte
		djnz	b,tftdrawc1	;repeat number in b
; b is 0. If the f0 is set, we need to repeat 
		jbc	f0,tftdrawc1	;another 256 times to go
		djnz	r0,tftdrawc0	;reload b and f0 and restart filling
		pop	01h		;restore registers from stack
		pop	00h
		pop	b
		pop	acc
		pop	psw
		ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; tftwrcbyte
; tft write control byte will write content of a register to the control register
; of the display
;
; input	a register (byte to be written)
; output	write a to control register
; used		none
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

tftwrcbyte:	clr	cs			;activate slave SPI interface
		clr	dc			;select control register
		mov	spidat,a		;start transmission
tftwrcbyte1:	jnb	ispi,tftwrcbyte1	;wait for transmission to end
		clr	ispi			;clear bit
		setb	cs			;end write session
		ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; tftwrdbyte
; tft write data byte will write content of a register to the data register
; of the display
;
; input	a register (byte to be written)
; output	write a to data register
; used		none
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

tftwrdbyte:	clr	cs			;activate slave SPI interface
		setb	dc			;select data register
		mov	spidat,a		;start transmission
tftwrdbyte1:	jnb	ispi,tftwrdbyte1	;wait for transmission to end
		clr	ispi			;clear bit
		setb	cs			;end write session
		ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; tftoutchar
; This function prints de symbol corresponding to the ASCII code in the accu
; on the tft display.
;
; ASCII codes 20h-7fh are valid.
; ASCII code ff (0ch) will clear the display (set to background color).
; All other ascii codes are rejected.
;
; input:	a=ASCII code to be printed (0ch and 20h-7fh)
;		tftxpos	0-19 position horizontal on the display
;		tftypos	0-6 position vertical on the display
;		position 0,0 is upper left corner of the display
; output:	charcter printed on the display in front color on background color
; used:	tftxpos is incremented in if >19 set to zero, tftypos is incremented 
;		when tftxpos is reset to 0. Ik tftypos >6 it will be reset to 0
;
; Incrementing tftxpos and tftypos allows total display scrolling with consecutive
; printing.
;
; Ik random printing is required, use tftoutcharhd.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

tftoutchar:	push	psw			;save regs on stack
		push	acc
		push	b
		push	acc			;back up ascii code
; tftoutchar uses the tftxpos and tftypos registers. This part of the routine calculates
; the real coordinates and stores them in the appropriate registers.
; The font used is 16x32 pixels. tftypos is multiplied by 32 for the real position
		mov	a,tftypos
		mov	b,#32			;multiplier
		mul	ab			;done
		mov	tfty,a			;done
; tftxpos is multiplied by 16
		mov	a,tftxpos
		mov	b,#16			;multiplier
		mul	ab			;done
		mov	tftxh,b		;store in correct registers
		mov	tftxl,a
		pop	acc			;restore ascii code
		lcall	tftoutcharf		;this function does the actual printing
; Printing is done. tftxpos needs to be updated
		inc	tftxpos		;point to next location
		mov	a,tftxpos		;cjne does not work with direct addressable register
		cjne	a,#20,tftoutchar1	;if value 20, reset to 0 and update tftypos
		mov	tftxpos,#0		;restore to 0
		inc	tftypos		;increment y position (next line)
		mov	a,tftypos		;cjne does not work with direct addressable register
		cjne	a,#7,tftoutchar1	;if value 7, restore to 0
		mov	tftypos,#0
tftoutchar1:	pop	b			;restore regs from stack
		pop	acc
		pop	psw
		ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; tftoutcharf
; This function prints de symbol corresponding to the ASCII code in the accu
; on the tft display.
;
; ASCII codes 20h-7fh are valid.
; ASCII code ff (0ch) will clear the display (set to background color).
; All other ascii codes are rejected.
;
; input:	a=ASCII code to be printed (0ch and 20h-7fh)
;		tftxh,tftxl	0-320 position horizontal on the display
;		tfty		0-240 position vertical on the display
;		position 0,0 is upper left corner of the display
;		WARNING: being stupid is possible!!
; output:	charcter printed on the display in front color on background color
; used:	tftxh and tftxlow are incremented by 15, tfty is incremented by 31
;
; Ik simple positioning is required, use tftoutchar
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


tftoutcharf:	push	psw			;save regs on stack
		push	acc
		push	b
		push	00h
		push	01h
		push	02h
		push	dph
		push	dpl
		push	acc		;needed!!(back up ASCII code)
		mov	a,#2ah		;set column address limits (x coordinates)
		lcall	tftwrcbyte
		mov	a,tftxh	;start address (high and low byte)
		lcall	tftwrdbyte
		mov	a,tftxl
		lcall	tftwrdbyte	;2 parameters 
; We need to calculate the end of the window
		mov	a,tftxl	;we need to add 15 (window is 16 pixels wide, but 0 counts as 1)
		add	a,#15		;add\
		mov	tftxl,a	;store
		mov	a,tftxh	;carry needs to be recovered
		addc	a,#00h
		mov	tftxh,a	;store	high byte	
		lcall	tftwrdbyte
		mov	a,tftxl	;low byte
		lcall	tftwrdbyte	;done
		mov	a,#2bh		;set page address (y coordinates)
		lcall	tftwrcbyte
		mov	a,#00h		;starting address is 00xxh
		lcall	tftwrdbyte
		mov	a,tfty
		lcall	tftwrdbyte	;write high and low
		mov	a,#00h		;ending is 00xxh
		lcall	tftwrdbyte
; end of y window needs to be calculated. Window is 32 pixels but we only add 31 (pixel 0 is teh 32nd)
		mov	a,tfty
		add	a,#31
		mov	tfty,a		;store result
		lcall	tftwrdbyte	;done
		mov	a,#2ch		;give memory write command (sets internal pointers to starting address)
		lcall	tftwrcbyte
		pop	acc		;restore ascii code in a
; if the ascii code is 0ch, the we need to clear the display.
		cjne	a,#0ch,tftoutcharf1
		lcall	tftclear	;clear display
tftoutcharf3:	pop	dpl		;restore regs from stack
		pop	dph
		pop	01h
		pop	02h		
		pop	00h
		pop	b
		pop	acc
		pop	psw
		ret

; ascii code is not form feed. We eliminate codes larger then 7fh by clearing the msb.
; To avoid further errors we discart codes smaller than 20h

tftoutcharf1:	clr	acc.7		;msb=0
		cjne	a,#20h,tftoutcharf2	; testing for to low values
tftoutcharf2:	jc	tftoutcharf3	;if carry ==1 then invalid codes, end function

; The ascii code is (made) valid. We need to calculate the offset in the lookup table

		clr	c		;we substract the ascii offset (20h)
		subb	a,#20h		;done
		mov	b,#64		;each character has 64 bytes code
		mul	ab		;ba contains the offset
		mov	dptr,#chr_20	;load in dptr starting address of the look-up table
		add	a,dpl		;16 bit add
		mov	dpl,a
		mov	a,b		;store high byte in accu
		addc	a,dph		;done
		mov	dph,a		;dptr == address code for ascii code
; Let printing commence
		mov	b,#64		;number of bytes for 1 character
tftoutcharf4:	mov	r0,#8		;bitcounter
		clr	a
		movc	a,@a+dptr
tftoutcharf5:	rlc	a		;test bits
		jnc	tftoutcharf6
		push	acc
		mov	a,tftfrcolorh
		lcall	tftwrdbyte
		mov	a,tftfrcolorl
		lcall	tftwrdbyte
		sjmp 	tftoutcharf7
tftoutcharf6:	push	acc
		mov	a,tftbgcolorh
		lcall	tftwrdbyte
		mov	a,tftbgcolorl
		lcall	tftwrdbyte
tftoutcharf7:	pop	acc
		djnz	r0,tftoutcharf5
		inc	dptr
		djnz	b,tftoutcharf4
		pop	dpl		;restore regs from stack
		pop	dph
		pop	02h
		pop	01h
		pop	00h
		pop	b
		pop	acc
		pop	psw
		ret
		
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; tftoutmsga
; This function prints a string from code memory on the display.
; Printing starts @ ftfxpos and tftypos. Both registers are updated during printing.
; 
; input:	dptr	holds the starting address of the ascii string in memory
;             text string in memory, ended with 00h code
;		tftbgcolor and tftfrcolor are used for printing
; output:	string printed on the display
; used:	none (tftxpos and tftypos are updated)
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

tftoutmsga:	push	psw		;save registerds on stack
		push	acc
		push	dpl
		push	dph
tftoutmsga1:	clr	a		;needed for movc instruction
		movc	a,@a+dptr	;load ascii code from external memory (code memory)
		jz	tftoutmsga2	;if code==00h end function
		lcall	tftoutchar	;print character
		inc	dptr		;increment datapointer to string
		sjmp	tftoutmsga1	;repeat action
tftoutmsga2:	pop	dph		;restore registers
		pop	dpl
		pop	acc
		pop	psw
		ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; tftoutnib
; This function prints the lower 4 bits (lower nibble) as a numeric value of the 
; accu on the display. (0-9 and A-F).
;
; input:	accu (only lower nibble)
; output:	1 digit hex value on display
; used:	none (tftxpos and tftypos are updated)
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

tftoutnib:	push	acc		;save register on stack
		anl	a,#0fh		;separate lower nibble
		lcall	batrans	;transform into ascii code
		lcall	tftoutchar	;print
		pop	acc		;restore registers
		ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; tftoutbyte
; This function prints the contents of the accu as a numeric value on the display.
; The value is printed in HEX format (eg 00-FF)
;
; input:	value in accu
; output:	2 digit number printed on the display
; used:	none (tftxpos and tftypos are updated)
;
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

tftoutbyte:	push	acc		;save registers on stack
		swap	a		;swap nibbles
		lcall	tftoutnib	;print high nibble
		swap	a		;swap nibbles
		lcall	tftoutnib
		pop	acc		;restore registers from stack
		ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; ADuCsioX.inc is een include file voor de ADuC832 microcontroller.
; De file laat toe om de seriele poort te gebruiken en biedt de standaard
; monitorroutines zoals die beschikbaar zijn in de MGM monitors voor 8051.
; De file wordt achteraan in de .asm file ge-included. De belangrijkste
; afwijking van de standaard routines is de initsio subroutine.
; Die laat toe om 9600 baud in te stellen, maar zal rekening houden met de PLL
; waarde. Wil je de initsio routine aanroepen, dan moet de PLL reeds geladen
; zijn.
;
; Geschreven door Roggemans Marc (MGM) op 11/2004 v0
; Aangepast door Roggemans Marc (MGM) op 11/2009
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

blank         equ    020h          ;ascii code voor een blank
xoff          equ    013h          ;ascii code xoff caracter (ctrl s)
ff            equ    00ch          ;ascii code voor nieuw blad
cr            equ    00dh          ;ascii code voor een wagenterugloop
lf            equ    00ah          ;ascii code voor volgende lijn
backsp        equ    008h          ;ascii code voor het bs karakter
beep          equ    007h          ;ascii code voor een beep

wissen:       db     backsp,blank,backsp,000h    ;string wissen karakter
                                                        ;van het scherm

strtbuf       equ     040h         ;start adres van de buffer
endbuf        equ     06fh         ;laatst gebruikte locatie (kan cr bevatten)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; initsio zal de seriele poort klaar zetten voor gebruik. De baudrate is 9600.
; De routine maakt gebruik van de pllcon waarde om te bepalen hoe de timer 3
; als baud rate generator ingesteld moet worden. Indien de instelling niet
; mogelijk is wordt de carry geset. In het andere geval wordt de carry laag
; gemaakt. De routine is pedagogisch geschreven en kan dus korter
;
; de routine gebruikt de psw
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

sioinit:	push   acc           ;registers op de stack zetten
              mov    a,pllcon      ;we lezen het pllcon SFR uit
              anl    a,#007h       ;alleen de laagste 3 bits tellen
              cjne   a,#006h,initsio1        ;005h is maximaal toegelaten
initsio1:     jnc    initsioe      ;geen carry dan is waarde in pllcon >=6

; de waarde in pllcon is bruikbaar. We gebruiken de waarde om de inhoud van
; t3con te bepalen. t3fd is altijd 02dh voor 9600 baud.

              mov    t3fd,#02dh    ;zie databoek t3 als baud rate generator
              cpl    a             ;bitbangen om laten uit te komen
              clr    c             ;verder doen
              subb   a,#002h       ;laagste 3 bits zijn 0k
              anl    a,#007h       ;hoogste 5 bits op 0
              add    a,#080h       ;nu alles in orde
              mov    t3con,a       ;baud rate klaar
              mov    scon,#01010000b ;UART initialiseren
              clr    c             ;aangeven init ok (overbodige instructie)
              pop    acc
              ret

initsioe:     setb   c             ;aangeven initialisatie niet mogelijk
              pop    acc
              ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; OUTCHAR IS EEN SUBROUTINE DIE EEN KARAKTER VIA DE SERIELE INTERFACE NAAR
; BUITEN STUURT. DE ROUTINE GEBRUIKT DE ACCU ALS INPUT. DE ROUTINE GAAT NA DAT
; DE TRANSMITTER LEEG IS ALVORENS HET KARAKTER TE VERSTUREN.
;
; de routine gebruikt geen registers.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

siooutchar:	MOV    SBUF,A        ;KARAKTER VERZENDEN
OUTCHAR1:     JNB    TI,OUTCHAR1   ;WACHT TOT ZENDER BESCHIKBAAR
              CLR    TI            ;LAAG MAKEN VAN DE BIT
              RET

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; OUTNIB VERZEND DE LAAGSTE VIER BITS NAAR DE SERIELE POORT ONDER DE VORM VAN
; EEN ASCII CODE. DE ACCU EN DE VLAGGEN WORDEN DOOR DE ROUTINE GEBRUIKT.
;
; de routine gebruikt geen registers.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

siooutnib:	push   acc           ;registers op de stack
              push   psw
              ANL    A,#00FH       ;AFZONDEREN
              LCALL  BATRANS       ;OMZETTEN IN ASCII CODE
              LCALL  sioOUTCHAR       ;VERSTUREN
              LCALL  XONXOFF       ;FLOW CONTROL
              pop    psw           ;registers herstellen
              pop    acc
              RET

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; OUTBYTE ZET DE HEX INHOUD VAN DE ACCU OP DE SERIELE POORT. ALS FLOW CONTROL
; WORDT XONXOFF GEBRUIKT.
;
; de routine gebruikt geen registers.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

siooutbyte:	push   acc           ;registers op de stack zetten
              push   psw
              PUSH   ACC           ;BACK UP NEMEN VAN DE ACCU
              SWAP   A             ;EERST HOOGSTE DEEL
              LCALL  sioOUTNIB        ;NIBBLE NAAR HET SCHERM
              POP    ACC           ;HERSTELLEN VAN DE ACCU
              LCALL  sioOUTNIB        ;NIBBLE NAAR HET SCHERM
              pop    psw           ;registers herstellen
              pop    acc
              RET

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; DISPDPTR ZET DE HEX INHOUD VAN DE DPTR OP HET SCHERM OP DE HUIDIGE PLAATS
; VAN DE CURSOR.
;
; de routine gebruikt geen registers.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

siodispdptr:	push   acc           ;registers op de stack zetten
              push   psw
              MOV    A,DPH         ;EERST HOOGSTE DEEL OP SCHERM
              LCALL  sioOUTBYTE       ;BYTE IN ASCII NAAR HET SCHERM
              MOV    A,DPL         ;MINST BEDUIDENDE DEEL VERZENDEN
              LCALL  sioOUTBYTE       ;WEG ER MEE
              pop    psw           ;registers herstellen
              pop    acc
              RET                  ;EINDE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; OUTMSGA IS EEN SUBROUTINE, DIE DE ASCII CODES VANAF HET ADRES IN DE DPTR,
; TOT AAN DE CODE 00H, OP HET SCHERM ZET. HET BERICHT STAAT IN CODE MEMORY.
;
; de routine gebruikt geen registers.
; 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

siooutmsga:	push   acc           ;alles op de stack zetten
              push   psw
              push   dpl
              push   dph
outmsga2:     CLR    A             ;ACCU LEEG MAKEN
              MOVC   A,@A+DPTR     ;EERSTE CODE OPHALEN
              JZ     OUTMSGA1      ;ALS NUL DAN EINDE
              LCALL  sioOUTCHAR       ;KARAKTER NAAR HET SCHERM
              LCALL  XONXOFF       ;SOFTWARE HANDSHAKE
              INC    DPTR          ;DPTR AANPASSEN
              SJMP   OUTMSGA2      ;HERHAAL TOT EINDE
OUTMSGA1:     pop    dph           ;alles herstellen
              pop    dpl
              pop    psw
              pop    acc
              RET                  ;EINDE ROUTINE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; XONXOFF LAAT HET GEBRUIK VAN SOFTWARE HANDSHAKE TOE. DE ROUTINE GAAT NA DAT
; ER EEN KARAKTER ONTVANGEN WERD. ALS DIT HET XOFF KARAKTER IS BLIJFT DE
; ROUTINE WACHTEN OP EEN ANDER WILLEKEURIG KARAKTER. DIT LAATSTE WORDT NIET
; VERDER DOORGEGEVEN.
;
; de routine gebruikt geen registers.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

XONXOFF:      push   acc           ;registers op de stack zetten
              push   psw
              JBC    RI,XONXOFF1   ;GA NA KARAKTER ONTVANGEN
XONXOFF2:     pop    psw           ;registers herstellen
              pop    acc
              RET                  ;ANDERS EINDE
XONXOFF1:     MOV    A,SBUF        ;ZO JA, KARAKTER LADEN IN ACCU
              CJNE   A,#XOFF,XONXOFF2     ;ALS NIET XOFF DAN VERDER
XONXOFF3:     JBC    RI,XONXOFF2   ;WACHTEN OP WILLEKEURIG KARAKTER
              SJMP   XONXOFF3      ;EN DAN RIBBE EN DE BIE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; INCHAR BLIJFT WACHTEN TOTDAT ER EEN KARAKTER ONTVANGEN WERD DOOR DE SERIELE
; INTERFACE. HET KARAKTER WORDT DOORGEGEVEN IN DE ACCUMULATOR.
;
; de routine gebruikt de accu.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

sioinchar:	JNB    RI,sioINCHAR            ;BLIJF WACHTEN OP HET KARAKTER
              CLR    RI                   ;BIT LAAG MAKEN
              MOV    A,SBUF               ;KARAKTER IN DE ACCU
              RET

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; INBYTE LEEST TWEE ASCII CODES BINNEN EN VORMT ZE OM TOT EEN HEX BYTE.
; ALS OUTPUT WORDT DE ACCU GEBRUIKT.
; ALS HET DE TWEE ASCII CODES NIET TOT EEN BYTE OMGEZET KUNNEN WORDEN, ZAL
; DE CARRY OP 1 GEZET WORDEN.
;
; De routine gebruikt de accu, r0 en de psw.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

sioinbyte:	LCALL  sioINCHAR               ;EERSTE KARAKTER INLEZEN
              LCALL  LOWUPTR
              LCALL  ASCBINTRANS          ;OMZETTEN NAAR HEX GETAL
              JC     INBYTE1              ;BIJ ERROR AKTIE ONDERNEMEN
              SWAP   A                    ;IS HOOGSTE NIBBLE
              MOV    R0,A                 ;ACCU TIJDELIJK BEWAREN
              LCALL  sioINCHAR               ;TWEEDE KARAKTER OPHALEN
              LCALL  LOWUPTR
              LCALL  ASCBINTRANS          ;OMZETTEN NAAR BIN GETAL
              JC     INBYTE1              ;BIJ ERROR ....
              ORL    A,R0                 ;BIJDE GETALLEN SAMEN VOEGEN
INBYTE1:      RET

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; INBUFA LEEST EEN ASCII BUFFER BINNEN VIA DE SERIELE POORT. DE ROUTINE LAAT
; TOE OM FOUTIEVE INGAVEN TE VERBETEREN. HIERVOOR KAN DE BACKSPACE TOETS
; GEBRUIKT WORDEN. HET STARTADRES VAN DE BUFFER IS STRTBUF. DE LAATSE BYTE
; HEEFT HET ADRES ENDBUF. ALS DE BUFFER TE VOL, OF TE LEEG, GEMAAKT WORDT,
; LAAT DE CPU EEN BEEP HOREN. DE ROUTINE GEBRUIKT DE ACCU EN R0. CONTROLE
; KARAKTERS WORDEN NIET VERWERKT. DE ROUTINE GEEFT OOK EEN ECHO NAAR HET
; SCHERM. ZE GEBRUIKT DE SUBROUTINES INCHAR, OUTCHAR EN ASCCTR. INBUFA DOET
; OOK DE OMVORMING NAAR ASCII 0 KARAKTERS. Door de waarden van strtbuf en endbuf
; aan te passen kan de gebruiker zelf de lengte en de plaats van de buffer
; bepalen.
;
; De routine gebruikt de accu, r0 en de psw.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

sioinbufa:	MOV    R0,#STRTBUF          ;START ADRES VAN DE BUFFER LADEN
INBUFA2:      LCALL  sioINCHAR               ;KARAKTER LEZEN
              ANL    A,#07FH              ;KARAKTERS OMVORMEN TOT ASCII 0
; ALVORENS EEN ECHO NAAR HET SCHERM TE VERSTUREN, MOET NAGEGAAN WORDEN DAT HET
; BEKOMEN KARAKTER AAN DE TE VERWACHTEN KARAKTERS VOLDOET. DE ENIGE TOEGELATEN
; CONTROLEKARAKTERS ZIJN 'CR' EN 'BACKSPACE'. VOOR AL DE ANDERE KARAKTERS
; WORDT EEN BEEP NAAR HET SCHERM GESTUURD. DE CONTROLE WORDT DOOR ASCCTR UIT-
; GEVOERD.
              LCALL  LOWUPTR              ;WEL EFFE IN CAPS TE ZETTEN
              CJNE   A,#CR,INBUFA1        ;CR INGEGEVEN?
              MOV    @R0,A                ;IN GEHEUGEN ZETTEN
              RET                         ;EINDE VAN DE ROUTINE
INBUFA1:      CJNE   A,#BACKSP,INBUFA6    ;NAGAAN BACK SPACE INDRUKKEN
              CJNE   R0,#STRTBUF,INBUFA3  ;STAAT DE BUFFER ONDERAAN,DAN BEEP
INBUFA4:      MOV    A,#BEEP
INBUFA7:      LCALL  sioOUTCHAR
              SJMP   INBUFA2              ;NIEUW KARAKTER OPHALEN
INBUFA3:      DEC    R0                   ;ENTRY UIT BUFFER HALEN
              MOV    DPTR,#wissen         ;WIS KARAKTER DATA NAAR SCHERM
              LCALL  sioOUTMSGA
              SJMP   INBUFA2              ;WACHT OP VOLGENDE INGAVE
INBUFA6:      LCALL  ASCCTR               ;ALS CARRY DAN NIET GELDIG
              JC     INBUFA4              ;BEEP NAAR SCHERM
              CJNE   R0,#ENDBUF-1,INBUFA5 ;BUFFER OP HET EINDE DAN ERROR
              SJMP   INBUFA4              ;BEEP NAAR HET SCHERM
INBUFA5:      MOV    @R0,A                ;KARAKTER IN DE BUFFER
              INC    R0                   ;BUFFER POINTER AANPASSEN
              SJMP   INBUFA7              ;KARAKTER IN BUFFER, VOLGENDE ?

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; BDELETE HAALT ALLE blank KARAKTERS UIT de inputbuffer
;
; De routine gebruikt de accu, r0, r1 en de psw.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

BDELETE:      MOV    R0,#STRTBUF          ;START ADRES IN R0
              MOV    R1,#STRTBUF          ;START ADRES IN R1
BDELETE1:     MOV    A,@R0                ;LEES KARAKTER
              CJNE   A,#BLANK,BDELETE2    ;KARAKTER GEEN BLANKO,DAN BDELETE2
BDELETE3:     INC    R0                   ;VOLGENDE KARAKTER OPHALEN
              CJNE   R0,#ENDBUF+1,BDELETE1 ;ALS NIET EINDE DAN VERDER
              RET                         ;EINDE VAN DE ROUTINE
BDELETE2:     MOV    @R1,A                ;KARAKTER NAAR BUFFER STUREN
              INC    R1
              SJMP   BDELETE3

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; ASCCTR MOET NAGAAN DAT HET KARAKTER IN DE ACCU BIJ DE STANDAARD KARAKTERS
; HOORT. ENKEL DE KONTROLEKARAKTERS CR EN BP ZIJN TOEGELATEN. DE ACCU EN DE
; VLAGGEN WORDEN GEBRUIKT. DE KARAKTERS < 020H EN > DAN 07EH WORDEN ALS FOUT
; AANZIEN.
;
; De routine gebruikt de accu en de psw.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ASCCTR:       CJNE   A,#020H,ASCCTR1      ;NAGAAN KARAKTER KLEINER DAN
ASCCTR1:      JC     ASCCTR3              ;CARRY, DAN ERROR
              CJNE   A,#07FH,ASCCTR2      ;GROTER DAN 7EH
ASCCTR2:      JNC    ASCCTR3              ;GEEN CARRY DAN ERROR
              CLR    C                    ;AANGEVEN GEEN FOUT
              RET
ASCCTR3:      SETB   C                    ;AANGEVEN FOUT TOESTAND
              RET

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; LOWUPTR ZET HET KARAKTER IN DE ACCU OM VAN LOWER NAAR UPPER CASE. DE ROUTINE
; GEBRUIKT DE VLAGGEN EN DE ACCU. DE HEX WAARDEN 61H TOT EN MET 7AH MOETEN
; OMGEZET WORDEN NAAR 41H TOT EN MET 5AH. DE ANDERE GETALLEN MOGEN NIET
; AANGEPAST WORDEN.
;
; De routine gebruikt de accu en de psw.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

LOWUPTR:      CJNE   A,#07BH,LOWUPTR1     ;BOVENSTE GRENS
LOWUPTR1:     JNC    LOWUPTR2             ;GEEN CARRY DAN GEEN OMZETTING
              CJNE   A,#061H,LOWUPTR3     ;ONDERSTE GRENS NAGAAN
LOWUPTR3:     JC     LOWUPTR2             ;CARRY DAN GEEN OMZETTING
              SUBB   A,#020H              ;OMZETTING
LOWUPTR2:     RET                         ;EINDE OMZETTING

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; BATRANS DOET DE OMZETTING VAN EEN HEX NIBBLE IN DE ACCU NAAR DE PASSENDE
; ASCII CODE.
;
; De routine gebruikt de accu en de psw.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

BATRANS:      CJNE   A,#00AH,BATRANS1     ;NAGAAN 30 OF 37 MOETEN BIJ OPTELLEN
BATRANS1:     JC     BATRANS2             ;CARRY=1 DAN SLECHTS 30
              ADD    A,#007H              ;ANDERS 30+7=37
BATRANS2:     ADD    A,#030H              ;30 BIJ OPTELLEN
              RET

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; ASCBINTRANS ZET HET ASCII KARAKTER IN DE ACCU OM IN EEN BINAIR GETAL. DE
; ROUTINE KAN ENKEL DE KARAKTERS TUSSEN 0-9 EN A-F VERWERKEN. IN HET ANDERE
; GEVAL WORDT DE CARRY GEZET.
;
; De routine gebruikt de accu en de psw.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ASCBINTRANS:  CLR    C                    ;CARRY LAAG MAKEN VOOR BEREKENINGEN
              SUBB   A,#030H              ;NAGAAN GETAL BINNEN GRENZEN
              JC     ASCBINTRANS1         ;ALS C=1 DAN FOUT
              CJNE   A,#00AH,ASCBINTRANS3 ;GETAL TUSSEN 0 EN 9 ?
ASCBINTRANS3: JC     ASCBINTRANS2         ;ALS CARRY DAN ALLES OK
              SUBB   A,#007H              ;VERDER MET DE OMZETTING
              JC     ASCBINTRANS1         ;ALS CARRY DAN ERROR
              CJNE   A,#010H,ASCBINTRANS4 ;CONTROLE OP TE GROTE GETALLEN
ASCBINTRANS4: CPL    C                    ;CARRY COMPLEMENTEREN
              JC     ASCBINTRANS1         ;ALS CARRY DAN FOUT IN HET GETAL
ASCBINTRANS2: CLR    C                    ;CARRY LAAG MAKEN
ASCBINTRANS1: RET

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; ASCII1 ZET HET ASCII KARAKTER @R0 OM IN EEN HEX GETAL IN DE ACCU.
; DE ROUTINE VERANDERD OOK DE VLAGGEN.
;
; De routine gebruikt de accu en de psw.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ASCII1:       MOV    A,@R0                ;KARAKTER OPHALEN
              LCALL  ASCBINTRANS          ;OMZETTING ASCII NAAR BINAIR
              RET                         ;EINDE OMZETTING

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; ASCII2 ZET TWEE ASCII KARAKTERS @R0 EN @(R0+1) OM NAAR EEN HEX KARAKTER
; IN DE ACCU. R0 WORDT MET EEN VERHOOGD. R1 WORDT AANGEPAST. ALS OP HET EINDE
; VAN DE ROUTINE DE CARRY GESET IS, DAN IS DE OMZETTING NIET MOGELIJK.
;
; De routine gebruikt de accu, r0, r1 en de psw.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ASCII2:       LCALL  ASCII1               ;OMZETTEN
              JC     ASCII21              ;CARRY 1 DAN OMZETTING NIET MOGELIJK
              SWAP   A                    ;NIBBLE VAN PLAATS VERANDEREN
              MOV    R1,A                 ;TIJDELIJK WEG STOPPEN
              INC    R0                   ;NAAR VOLGENDE KARAKTER WIJZEN
              LCALL  ASCII1               ;KARAKTER OMZETTEN
              JC     ASCII21               ;WEG ALS C=1
              ORL    A,R1                 ;GETALLEN BIJ ELKAAR VOEGEN
ASCII21:      RET

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; ASCII4 ZET DE 4 ASCII KARAKTERS @R0 TOT @[R0+3] OM IN EEN HEX GETAL IN DE
; DATAPOINTER. ALS DE OMZETTING NIET MOGELIJK IS WORDT DE CARRY GEZET.
; R0 WORDT MET DRIE VERHOOGD, EN DE ACCU WORDT AANGEPAST,ALSOOK R1. ALS OP HET
; EINDE VAN DE ROUTINE DE CARRY GESET IS, DAN WAS EEN OMZETTING NIET MOGELIJK.
;
; De routine gebruikt de accu, r0, r1 en de psw.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ASCII4:       LCALL  ASCII2               ;OMZETTEN EERSTE TWEE ASCII BYTES
              JC     ASCII41              ;IN GEVAL VAN ERROR DAN WEG
              MOV    DPH,A                ;HEX GETAL IN DE DPTRH
              INC    R0                   ;WIIJST NU NAAR VOLGENDE GROEP VAN 2
              LCALL  ASCII2               ;OMZETTEN VOLGENDE TWEE BYTES
              JC     ASCII41              ;BIJ FOUT DAN WEG
              MOV    DPL,A                ;IN DPTR LADEN
ASCII41:      RET

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; 51mathx.inc is een driver die subroutines bevat voor wiskundige berekeningen.
; Het is niet de bedoeling om de meest compacte code te bekomen. De driver zal
; ook niet alle gewenste routines bevatten. Hij is geschreven met pedagogische
; doelstellingen in het achterhoofd. 
; 
; De driver bevat routines voor 8 bit bcd naar hex en hex naar bcd omvorming
; LET OP!!!! de getallen moeten in waarde kleiner dan 100d zijn (zowel hex als
; bcd)
;      8bit hex naar bcd
;      8bit bcd naar hex
;
; De driver bevat routines voor de 4 hoofdbewerkingen op 16 bit getallen:
;      16/16 bit met 16 bit uitkomst en 16 bit rest
;      16*16 bit met 32 bit uitkomst
;      16+16 bit met 24 bit uitkomst
;      16-16 bit met 24 bit uitkomst
;      16bit hex naar bcd
;      16bit bcd naar hex
;
; De driver bevat routines voor de 4 hoofdbewerkingen op 32 bit getallen:
;      32/32 bit met 32 bit uitkomst en 32 bit rest
;      32*32 bit met 64 bit uitkomst
;      32+32 bit met 40 bit uitkomst
;      32-32 bit met 40 bit uitkomst
;
; de gebruiker moet zorgen dat de berekeningen mogelijk zijn!!!!!!!!!!!!!!!!!!
; 
; De routines gebruiken als input en output de huidige registerbank.
; 
; geschreven door Roggemans Marc (MGM) op 11/2004 uitvoering 51math0.inc
; aangepast voor rigel door Roggemans Marc (MGM) op 11/2009 uitvoering 51mathR.inc
;
;
; WAARSCHUWING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
; DEZE ROUTINES GEBRUIKEN 15 BYTES STACK. ZE WERKEN ENKEL ALS DE STACK NIET DE
; ADRESSEN 73h-7fh GEBRUIKT. ER WORDEN HIER IMMERS DIRECT ADRESSEERBARE REGISTERS
; VRIJ GEMAAKT OM IN TE REKENEN.
; !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Hier worden gpr's verbonden met een naam. Ze worden gebruikt door sommige
; routines. Omdat ze informatie kunnen bevatten worden ze op de stack gezet.

h0            equ    073h
h1            equ    074h
h2            equ    075h
h3            equ    076h
h4            equ    077h
h5            equ    078h
h6            equ    079h
h7            equ    07ah
h8            equ    07bh
h9            equ    07ch
h10           equ    07dh
h11           equ    07eh
h12           equ    07fh

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; mul32       is de routine die twee 32 bit getallen zal vermenigvuldigen.
; input:      r3,r2,r1,r0 = eerste getal (r3=msb)
;             r7,r6,r5,r4 = tweede getal (r7=msb)
; output:     r7,r6,r5,r4,r3,r2,r1,r0 met r7=msb
;
; De routine gebruikt alleen de registers van de huidig geselecteerde bank
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

mul32:        push   acc                  ;registers op de stack zetten
              push   psw

; eerst moeten de hulpregisters klaar gezet worden

              mov    h0,#000h
              mov    h1,#000h
              mov    h2,#000h
              mov    h3,#000h
              mov    h4,#000h
              mov    h5,#000h
              mov    h6,#000h
              mov    h7,#000h
              mov    h8,#000h
              mov    h9,#000h
              mov    h10,#000h
              mov    h11,#000h
              mov    h12,#32              ;loopcounter

; volgende lus moet 32 keer doorlopen worden om de berekening te maken

; nu wordt de vermenigvuldiger naar rechts in de carry geshift

mul321:       mov    a,r3
              rrc    a
              mov    r3,a
              mov    a,r2
              rrc    a
              mov    r2,a
              mov    a,r1
              rrc    a
              mov    r1,a
              mov    a,r0
              rrc    a
              mov    r0,a                 ;32 bit shift klaar

;als de carry 1 is moet het 64 bit getal h3-r4 opgeteld worden bij h11-h4

              jnc    mul322               ;optelling niet nodig

; hier staat en 64 bit optelling

              mov    a,r4
              add    a,h4
              mov    h4,a
              mov    a,r5
              addc   a,h5
              mov    h5,a
              mov    a,r6
              addc   a,h6
              mov    h6,a
              mov    a,r7
              addc   a,h7
              mov    h7,a
              mov    a,h0
              addc   a,h8
              mov    h8,a
              mov    a,h1
              addc   a,h9
              mov    h9,a
              mov    a,h2
              addc   a,h10
              mov    h10,a
              mov    a,h3
              addc   a,h11
              mov    h11,a                ;optelling klaar

; het getal h3-r4 moet met twee vermenigvuldigd worden

mul322:       clr    c                    ;moet op 0 staan
              mov    a,r4
              rlc    a
              mov    r4,a
              mov    a,r5
              rlc    a
              mov    r5,a
              mov    a,r6
              rlc    a
              mov    r6,a
              mov    a,r7
              rlc    a
              mov    r7,a
              mov    a,h0
              rlc    a
              mov    h0,a
              mov    a,h1
              rlc    a
              mov    h1,a
              mov    a,h2
              rlc    a
              mov    h2,a
              mov    a,h3
              rlc    a
              mov    h3,a

              djnz   h12,mul321           ;herhaal 32 keer

; de uitkomst moet in de juiste registers komen

              mov    r0,h4
              mov    r1,h5
              mov    r2,h6
              mov    r3,h7
              mov    r4,h8
              mov    r5,h9
              mov    r6,h10
              mov    r7,h11               ;klaar

		pop    psw			;registers herstellen
              pop    acc
              ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; mul16       is de routine die twee 16 bit getallen zal vermenigvuldigen.
; input:      r1,r0 = eerste getal (r1=msb)
;             r3,r2 = tweede getal (r3=msb)
; output:     r3,r2,r1,r0 met r3=msb
;
; De routine gebruikt alleen de registers r3, r2, r1, r0 van de huidige bank
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

mul16:        push   acc                  ;registers op de stack zetten
              push   psw

; eerst registers laden met startwaarden. h0 en h1 zijn het verlengstuk
; van r3 en r2. h2, h3, h4, h5 worden gebruikt voor sommatie.
; h6 is de loopcounter.

              mov    h0,#000h
              mov    h1,#000h
              mov    h2,#000h
              mov    h3,#000h
              mov    h4,#000h
              mov    h5,#000h
              mov    h6,#16               ;loopcounter
              
; hier staat de lus die 16 keer doorlopen wordt voor het uitvoeren van de
; berekening

mul161:       mov    a,r1                 ;getal 1 1*rechts in carry
              rrc    a
              mov    r1,a
              mov    a,r0
              rrc    a
              mov    r0,a                 ;ok

; als carry 1 dan h1, h0, r3, r2 optellen bij h5, h4, h3, h2

              jnc    mul162

              mov    a,r2
              add    a,h2
              mov    h2,a                 ;32 bit som uitvoeren
              mov    a,r3
              addc   a,h3
              mov    h3,a
              mov    a,h0
              addc   a,h4
              mov    h4,a
              mov    a,h1
              addc   a,h5
              mov    h5,a                 ;klaar

; het te vermenigvuldige getal maal 2 (1*links shiften)

mul162:       clr    c
              mov    a,r2
              rlc    a
              mov    r2,a                 ;ook 32 bit getal
              mov    a,r3
              rlc    a
              mov    r3,a
              mov    a,h0
              rlc    a
              mov    h0,a
              mov    a,h1
              rlc    a
              mov    h1,a                 ;klaar

              djnz   h6,mul161            ;herhaal 16 keer

; de vermenigvuldiging is klaar. Nu uitkomstregisters laden

              mov    r0,h2
              mov    r1,h3
              mov    r2,h4
              mov    r3,h5                ;klaar

              pop    psw			;registers herstellen
              pop    acc
              ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; div32       is de routine die twee 32 bit getallen zal delen (eerste/tweede).
; input:      r3,r2,r1,r0 = eerste getal (r3=msb)
;             r7,r6,r5,r4 = tweede getal (r7=msb)
; output:     r7,r6,r5,r4 = quotient (r7=msb)
;             r3,r2,r1,r0 = rest (r3=msb)
;             carry=1 bij delen door 0
;
; De routine gebruikt alleen de registers van de huidig geselecteerde bank
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

div32:        push   acc                  ;registers op de stack zetten
              push   psw
             
; eerst alles voorladen

              mov    h0,#000h
              mov    h1,#000h
              mov    h2,#000h
              mov    h3,#000h
              mov    h4,#000h
              mov    h5,#000h
              mov    h6,#000h
              mov    h7,#000h
              mov    h8,#000h
              mov    h9,#000h
              mov    h10,#000h
              mov    h11,#000h
              mov    h12,#32              ;loopcounter

; we gaan het te delen getal in de passende hulpregisters shiften

; deze lus moet 32 keer heraald worden

div321:       mov    a,r0                 ;h3-r0 is te delen getal
              rlc    a
              mov    r0,a
              mov    a,r1
              rlc    a
              mov    r1,a
              mov    a,r2
              rlc    a
              mov    r2,a
              mov    a,r3
              rlc    a
              mov    r3,a
              mov    a,h0
              rlc    a
              mov    h0,a
              mov    a,h1
              rlc    a
              mov    h1,a
              mov    a,h2
              rlc    a
              mov    h2,a
              mov    a,h3
              rlc    a
              mov    h3,a

; nu testen of het deeltal in h3-h0 past, tussenuitkomst in h7-h4

              clr    c                    ;zit in de weg
              mov    a,h0			
              subb   a,r4
              mov    h4,a
              mov    a,h1
              subb   a,r5
              mov    h5,a
              mov    a,h2
              subb   a,r6
              mov    h6,a
              mov    a,h3
              subb   a,r7
              mov    h7,a

; als er geen carry is, dan is het verschil succesvol, en moeten h3-h0
; vervangen worden door h7-h4

              jc     div322               ;niet vervangen

              mov    h0,h4
              mov    h1,h5
              mov    h2,h6
              mov    h3,h7                ;vervanging klaar

div322:       cpl    c                    ;dit moet in h11-h8 (uitkomst)
              mov    a,h8
              rlc    a
              mov    h8,a
              mov    a,h9
              rlc    a
              mov    h9,a
              mov    a,h10
              rlc    a
              mov    h10,a
              mov    a,h11
              rlc    a
              mov    h11,a                ;qoutient aangepast

              djnz   h12,div321           ;32 keer herhalen

; de berekening is klaar. uitkomstregisters nog laden

              mov    r4,h8
              mov    r5,h9
              mov    r6,h10
              mov    r7,h11               ;uitkomst geladen

              mov    r0,h0                ;rest laden
              mov    r1,h1
              mov    r2,h2
              mov    r3,h3

              pop    psw			;registers herstellen
              pop    acc
              ret


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; div16       is de routine die twee 16 bit getallen zal delen (eerste/tweede).
; input:      r1,r0 = eerste getal (r1=msb)
;             r3,r2 = tweede getal (r3=msb)
; output:     r3,r2 = quotient (r3=msb)
;             r1,r0 = rest (r1=msb)
;             carry=1 bij deling door 0
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

div16:        push   acc                  ;registers op de stack zetten
              push   psw
              
; registers voorladen

              mov    h0,#000h
              mov    h1,#000h
              mov    h2,#000h
              mov    h3,#000h
              mov    h4,#000h
              mov    h5,#000h
              mov    h6,#16               ;loopcounter
              
; eerst te delen getal 1*links shiften in hulpregisters

; deze lus moet 16 keer herhaald worden

div161:       mov    a,r0                 ;32 bit shift in h1-r0
              rlc    a
              mov    r0,a
              mov    a,r1
              rlc    a
              mov    r1,a
              mov    a,h0
              rlc    a
              mov    h0,a
              mov    a,h1
              rlc    a
              mov    h1,a

; nagaan of in h1-h0 de deler past r3-r2. h3-h2 wordt gebruikt voor
; het tussenresultaat.

              clr    c
              mov    a,h0
              subb   a,r2
              mov    h2,a
              mov    a,h1
              subb   a,r3
              mov    h3,a

              jc     div162               ;als carry dan h1-h0 niet vervangen
                                          ;door h3,h2
              mov    h0,h2
              mov    h1,h3                ;vervanging klaar

; uitkomst moet aangepast worden

div162:       cpl    c                    ;truuk
              mov    a,h4
              rlc    a
              mov    h4,a
              mov    a,h5
              rlc    a
              mov    h5,a
              djnz   h6,div161            ;16 keer herhalen

; de berekening is klaar. uitkomsten in passende registers

              mov    r2,h4                ;quotient
              mov    r3,h5

              mov    r0,h0                ;rest
              mov    r1,h1

              pop    psw			;registers herstellen
              pop    acc
              ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; add32       zal twee 32 bit getallen optellen en een 40 bit uitkomst.
;
; input:      r3,r2,r1,r0 = eerste getal (r3=msb)
;             r7,r6,r5,r4 = tweede getal (r7=msb)
; output:     r4,r3,r2,r1,r0 (r4=msb)
;
; r4,r3,r2,r1,r0 worden aangepast
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

add32:        push   acc                  ;registers op de stack zetten
              push   psw
              mov    a,r0                 ;eerste 8 bit
              add    a,r4
              mov    r0,a                 ;klaar
              mov    a,r1                 ;volgende 8 bit
              addc   a,r5
              mov    r1,a                 ;klaar
              mov    a,r2                 ;volgende 8 bit
              addc   a,r6
              mov    r2,a                 ;klaar
              mov    a,r3                 ;laatste 8 bit
              addc   a,r7
              mov    r3,a                 ;32 bit klaar
              mov    a,#000h
              addc   a,#000h              ;carry verrekenen
              mov    r4,a
              pop    psw                  ;registers herstellen
              pop    acc
              ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; add16       zal twee 16 bit getallen optellen en een 24 bit uitkomst.
;
; input:      r1,r0 = eerste getal (r1=msb)
;             r3,r2 = tweede getal (r3=msb)
; output:     r2,r1,r0 (r2=msb)
;
; r3,r2,r1,r0 worden aangepast
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

add16:        push   acc                  ;geen registers gebruiken behalve rx
              push   psw
              mov    a,r0                 ;starten met de optelling
              add    a,r2                 ;8 bit klaar
              mov    r0,a                 ;uitkomst wegschrijven
              mov    a,r1                 ;volgende 8 bit
              addc   a,r3                 ;16 bit klaar
              mov    r1,a                 ;uitkomst weggeschreven
              mov    a,#000h              ;carry verrekenen
              addc   a,#000h
              mov    r2,a                 ;24 bit klaar
              pop    psw                  ;registers herstellen
              pop    acc
              ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; sub32       zal het verschil berekenen van 2 32 bit getallen. De uitkomst is
;             40 bit groot. (eerste -tweede)
; input:      r3,r2,r1,r0 = eerste getal (r3=msb)
;             r7,r6,r5,r4 = tweede getal (r7=msb)
; output:     r4,r3,r2,r1,r0 (r4=msb)
;
; r4,r3,r2,r1,r0 worden aangepast
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

sub32:        push   acc                  ;registers bewaren
              push   psw                  ;ook de vlaggen
              clr    c                    ;zit nog effe in de weg
              mov    a,r0                 ;eerste 8 bit
              subb   a,r4
              mov    r0,a
              mov    a,r1                 ;tweede 8 bit
              subb   a,r5
              mov    r1,a
              mov    a,r2                 ;volgende 8 bit
              subb   a,r6
              mov    r2,a
              mov    a,r3                 ;laatste 8 bit
              subb   a,r7
              mov    r3,a
              mov    a,#000h              ;nu nog de carry
              subb   a,#000h
              mov    r4,a                 ;40 bit klaar
              pop    psw                  ;registers herstellen
              pop    acc
              ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; sub16       zal het verschil berekenen van 2 16 bit getallen. De uitkomst is
;             24 bit groot. (eerste - tweede)
; input:      r1,r0 = eerste getal (r1=msb)
;             r3,r2 = tweede getal (r3=msb)
; output:     r2,r1,r0 (r2=msb)
;
; r3,r2,r1,r0 worden aangepast
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

sub16:        push   acc                  ;registers bewaren
              push   psw
              clr    c                    ;mag nog niet meespelen
              mov    a,r0                 ;eerste 8 bit verschil
              subb   a,r2
              mov    r0,a                 ;klaar
              mov    a,r1
              subb   a,r3                 ;16 bit klaar
              mov    r1,a
              mov    a,#000h
              subb   a,#000h              ;eventuele carry verrekenen
              mov    r2,a                 ;24 bit klaar
              pop    psw                  ;registers herstellen
              pop    acc
              ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; hexbcd16    zet een 16 bit hex getal om in een 24 bit bcd getal
; input       r1,r0 = 16 bit hex getal (r1=msb)
; output      r2,r1,r0 =24 bit bcd getal (r2=msb)
;
; de routine gebruikt alleen de registers r3,r2,r1,r0 van de huidige bank 
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

hexbcd16:     push   acc                  ;registers effe bewaren op de stack
              push   psw

              mov    h9,#005h             ;loopcounter
              
hexbcd161:    mov    r3,#000h
              mov    r2,#00ah             ;hierdoor delen we
              lcall  div16                ;deling uitvoeren
              mov    a,r0                 ;rest
              push   acc                  ;op stack zetten
              mov    a,r2
              mov    r0,a
              mov    a,r3
              mov    r1,a                 ;uitkomst terug delen
              djnz   h9,hexbcd161

              pop    acc                  ;hoogste bits van de stack
              mov    r2,a                 ;klaar
              pop    acc
              swap   a
              mov    h9,a
              pop    acc
              add    a,h9
              mov    r1,a                 ;volgende 8 klaar
              pop    acc
              swap   a
              mov    h9,a
              pop    acc
              add    a,h9
              mov    r0,a                 ;klaar

              pop    psw                  ;registers herstellen
              pop    acc
              ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; hexbcd8     zet een 8 bit hex getal om in een 8 bit bcd getal
; input       a (maximale waarde = 63h)
; output      a
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

hexbcd8:      push   psw                  ;registers op stack zetten
              push   b
              mov    b,#00ah              ;hierdoor delen
              div    ab                   ;in accu qoutient in b rest
              swap   a
              add    a,b                  ;bcd getal klaar
              pop    b
              pop    psw
              ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; bcdhex16    zal een 16 bit bcd getal omzetten naar een hex getal.
; input:       r1,r0 = 16 bit bcd getal (r1=msb)
; output:      r1,r0 = 16 bit hex getal (r1=msb)
;
; de routine gebruikt r1,r0 van de huidige bank
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

bcdhex16:     push   b                    ;hebben we nodig
              push   acc
              push   psw

              mov    a,r0
              anl    a,#00fh              ;eenheden
              push   acc                  ;effe bewaren

              mov    a,r0
              swap   a
              anl    a,#00fh              ;tientallen
              mov    b,#00ah              ;waarde berekenen
              mul    ab                   ;klaar
              pop    b                    ;truukje
              add    a,b                  ;a is lage deel hex getal
              push   acc                  ;straks nodig

              mov    a,r1                 ;hoge deel 
              anl    a,#0f0h
              swap   a
              push   acc                  ;straks nodig

              mov    a,r1
              anl    a,#00fh              ;gaan we verder bewerken
              mov    b,#100               ;waarde uitrekenen
              mul    ab
              mov    r1,b
              mov    r0,a

              pop    acc
              mov    b,#10 
              mul    ab                   ;kan nog in a
              mov    b,#100               ;*10*100=*1000
              mul    ab
              add    a,r0
              mov    r0,a
              mov    a,r1
              addc   a,b
              mov    r1,a

              pop    acc
              add    a,r0
              mov    r0,a
              mov    a,#000h
              addc   a,r1
              mov    r1,a                 ;klaar

              pop    psw
              pop    acc
              pop    b
              ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; bcdhex8     zal een 8 bit bcd getal omzetten naar een hex getal.
; input:      a (maximale waarde 99h)
; output:     a
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

bcdhex8:      push   psw                  ;registers op de stack zetten
              push   b
              push   h0
              mov    h0,a                 ;effe bewaren
              anl    h0,#00fh             ;alleen 4 laagste bits
              anl    a,#0f0h              ;hier alleen 4 hoogste bits
              swap   a
              mov    b,#00ah              ;berekenen waarde 4 hoogste bits
              mul    ab
              add    a,h0                 ;eenheden bijtellen
              pop    h0
              pop    b
              pop    psw
              ret


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Dit is een IIC driver voor de ADuC832 microcontroller.
; De driver bevat alle routines om de ADuC als master te gebruiken.
; Als I/O worden de IIC pinnen gebruikt sdata en sclock. Vermits de pinnen niet
; eenvoudig gebruikt kunnen worden, zijn er macro's voorzien.
;
; !!!!!!!!!WARNING!!!!!!!!
; IIC and SPI use the same hardware pins. SPI is used for the TFT display. If IIC
; is used TFT operations will be disrupted.
;
; Geschreven door Roggemans Marc (MGM) op 03/2005 v0
;
; Beschikbare routines:
; iicinit       klaar zetten van de iic voor gebruik
; iicstart      verzenden van een start conditie
; iicstop       verzenden van een stop conditie
; iicoutbyte    verzenden van 1 byte vanuit de accu, er wordt een negende klok
;               verzonden, in de carry wordt de ack van de slave weergegeven
; iicinbyteack  leest een byte van de iic bus en genereerd een ack op de
;               negende klokpuls
; iicinbytenack leest een byte van de iic bus en genereerd een nack op de 
;               negende klokpuls
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; iicinit       is een subroutine die de iic van de ADuC als master instelt
initiic:
iicinit:        setb    mdo             ;data moet hoog wakker worden
                setb    mco             ;klok moet hoog wakker worden
                setb    mde             ;data wordt als output geschakeld
                setb    i2cm            ;master mode inschakelen
                ret

; iicstart      is een subroutine die de start conditie op de pinnen plaatst

iicstart:       setb    mco             ;zeker zijn dat de klok hoog is
                clr     mdo             ;data moet laag worden
                setb    mde             ;data pin enable
                clr     mco             ;klok laag maken
                ret

; iicstop       is een subroutine die de stop conditie opwekt.

iicstop:        clr     mdo             ;data moet laag worden
                setb    mde             ;data mag naar buiten
                setb    mco             ;nu moet de klok omhoog
                setb    mdo             ;nu data pin hoog maken
                ret                

; iicoutbyte    zal de accu naar buiten sturen, msb eerst.
;               De routine gebruikt geen registers, behalve de psw
;               De carry zal de waarde van de ack bit bevatten na het
;               uitvoeren van de routine.

iicoutbyte:     push    acc             ;worden effe op stack bewaard
                push    b
                mov     b,#008h         ;loopcounter
iicoutbyte1:    rlc     a               ;bit in de carry plaatsen
		  mov     mdo,c           ;carry naar de mdo pin
                setb    mde             ;pin als output activeren
                setb    mco             ;klokpukls hoog
                clr     mco             ;klokpuls laag
                djnz    b,iicoutbyte1   ;volledige byte verzenden
                clr     mde             ;iic data pin als input schakelen
                setb    mco             ;klokpuls hoog
                mov     c,mdi           ;data inlezen in de carry
                clr     mco             ;klokpuls terug laag
                pop     b
                pop     acc             ;registers herstellen
                ret

; iicinbyteack  zal in de accu een byte inlezen, msb eerst.
;               De routine gebruikt geen registers. De routine zal als
;               negende bit een ack verzenden.

iicinbyteack:   push    b               ;deze registers worden gebruikt
                push    psw
                mov     b,#008h         ;wordt gebruikt als loopcounter
iicinbytea1:    clr     mde             ;iic data pin als input schakelen
                setb    mco             ;klokpuls hoog
                mov     c,mdi           ;data inlezen in de carry
                clr     mco             ;klokpuls terug laag
                rlc     a               ;in accu shiften
                djnz    b,iicinbytea1   ;volledige byte ontvangen
                clr     c               ;ack verzenden
		  mov     mdo,c           ;carry naar de mdo pin
                setb    mde             ;pin als output activeren
                setb    mco             ;klokpukls hoog
                clr     mco             ;klokpuls laag
                pop     psw             ;registers herstellen
                pop     b
                ret

; iicinbytenack zal in de accu een byte inlezen, msb eerst.
;               De routine gebruikt geen registers. De routine zal als
;               negende bit een nack verzenden.

iicinbytenack:  push    b               ;deze registers worden gebruikt
                push    psw
                mov     b,#008h         ;wordt gebruikt als loopcounter
iicinbyten1:    clr     mde             ;iic data pin als input schakelen
                setb    mco             ;klokpuls hoog
                mov     c,mdi           ;data inlezen in de carry
                clr     mco             ;klokpuls terug laag
                rlc     a               ;in accu shiften
                djnz    b,iicinbyten1   ;volledige byte ontvangen
                setb    c               ;nack verzenden
		  mov     mdo,c           ;carry naar de mdo pin
                setb    mde             ;pin als output activeren
                setb    mco             ;klokpukls hoog
                clr     mco             ;klokpuls laag
                pop     psw             ;registers herstellen
                pop     b
                ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; delaya0k05s
; This delay routine reads the contents of the PLLCON register in order to adapt
; to the microcontroller clock.
;
; input:	accu contains a integer range 00h-ffh
; output: 	delay= (accu)*0,05s
; used:	accu
;
; Adapted by Roggemans M. (MGM) on 05/2016 (PLLCON used for calculation of delay)
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

delaya0k05s:	push	psw			;regs op stack
		push	acc
		push	b
		push	00h
		push	01h
		push	acc			;needed to use PLLCON value and calculate time base
		mov	b,pllcon		;read pllcon
		anl	b,#00000111b		;retain PLL bits
; The smaller the number, the faster the clock is running (0=16MHz, 7=0.13Mhz).
; By substracting 8 with the pll value we get 8 for highest speed and 1 for lowest speed.
; This number is used to calculate the number of delay loops is needed.
		mov	a,#08h			;base value
		clr	c			;required for subb instruction
		subb	a,b			;substract (a=power of two that indicates number of delay loops)
		mov	b,a			;need accu
		mov	a,#01h			;we need to calculate the power of 2 (shifting accu left number of times)
delaya0k05s1: djnz	b,delaya0k05s2
		sjmp	delaya0k05s3
delaya0k05s2:	rl	a			;multiply a by 2
		sjmp	delaya0k05s1
delaya0k05s3: mov	01h,a			;store value in register 01h for back up
		pop	acc			;restore original acc
delaya0k05s4:	mov	00h,01h		;load loopcounter (number ranges between 1-128 in powers of 2)
delaya0k05s5: mov	b,#0			;load loopcounter inner loop
delaya0k05s6:	djnz	b,delaya0k05s6
		djnz	00h,delaya0k05s5
		djnz	acc,delaya0k05s4	;repeat standard time loop of 0.05s number a times
; delay time=(((sysclok/24)*(register b))*(register 01))*(register a)
		pop	01h			;restore used registers		
		pop	00h
		pop	b
		pop	acc
		pop	psw
		ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; getadc	meet de spanning op de analoge input waarvan het nummer in de
; staat.
;
; input:	accu= te meten kanaalnummer
; output:	accu= 8 bit meetresultaat
; gebruikt: 	accu
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

getadc:	push	psw			;regs op stack
		push	b
		mov	adccon1,#11001100b	;settings adc omvormer
		anl	a,#0fh			;zorgen enkel kanaalnummer in accu
		mov	adccon2,a		;in kanaalselectie plaatsen
		setb	sconv			;meting starten
getadc1:	jb	sconv,getadc1		;wacht op meting klaar
		mov	a,adcdatah		;12 bit uitkomst omzetten naar 8 bit
		mov	b,adcdatal
		swap	a
		anl	a,#f0h
		xch	a,b
		swap	a
		anl	a,#0fh
		add	a,b
		pop	b		;regs herstellen
		pop	psw
		ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; This is the font table for ASCII codes (build with software from cosmicvoid, adapted by MGM)
;
; The font table is suited for translating ASCII code 20h-7fh.
;
; Format: 16x32 bits. The data is stored per row (2 bytes=1 row) and subsequent 31 rows. 
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

colorascii:
chr_20:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
	 db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
	 db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
	 db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_21:
        db 00h,00h,00h,00h,03h,F0h,03h,F0h,01h,E0h,01h,E0h,01h,E0h,01h,E0h
        db 01h,E0h,01h,E0h,01h,E0h,01h,E0h,00h,C0h,00h,C0h,00h,C0h,00h,C0h
        db 00h,C0h,00h,00h,00h,00h,00h,00h,00h,C0h,01h,E0h,03h,F0h,03h,F0h
        db 01h,E0h,00h,C0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_22:
        db 00h,00h,00h,00h,3Eh,7Ch,3Eh,7Ch,3Ch,3Ch,1Ch,38h,1Ch,38h,1Ch,38h
        db 18h,18h,18h,18h,18h,18h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_23:
        db 00h,00h,00h,00h,00h,00h,0Fh,3Ch,0Fh,3Ch,0Fh,3Ch,0Fh,3Ch,0Fh,3Ch
        db 0Fh,3Ch,3Fh,FEh,3Fh,FEh,3Fh,FEh,3Fh,FEh,0Fh,3Ch,0Fh,3Ch,0Fh,3Ch
        db 0Fh,3Ch,3Fh,FEh,3Fh,FEh,3Fh,FEh,0Fh,3Ch,0Fh,3Ch,0Fh,3Ch,0Fh,3Ch
        db 0Fh,3Ch,0Fh,3Ch,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_24:
        db 00h,00h,00h,00h,00h,C0h,07h,F0h,0Fh,FCh,1Fh,FEh,3Ch,DEh,38h,CEh
        db 38h,C0h,38h,C0h,38h,C0h,3Ch,C0h,1Eh,C0h,0Fh,E0h,07h,F8h,03h,FCh
        db 00h,DEh,00h,CEh,00h,C6h,00h,C6h,00h,C6h,38h,CEh,3Ch,DEh,1Fh,FCh
        db 0Fh,FCh,07h,F0h,00h,C0h,00h,C0h,00h,C0h,00h,C0h,00h,00h,00h,00h
chr_25:
        db 00h,00h,00h,00h,1Fh,00h,3Fh,80h,7Bh,C0h,71h,C0h,60h,C2h,71h,C6h
        db 7Bh,CEh,3Fh,9Eh,1Fh,3Ch,00h,78h,00h,F0h,01h,E0h,03h,C0h,07h,80h
        db 0Fh,00h,1Eh,00h,3Ch,00h,78h,F8h,71h,FCh,63h,DEh,43h,8Eh,03h,06h
        db 03h,8Eh,03h,DEh,01h,FCh,00h,F8h,00h,00h,00h,00h,00h,00h,00h,00h
chr_26:
        db 00h,00h,00h,00h,07h,F0h,0Fh,F8h,1Eh,38h,1Ch,1Ch,1Ch,1Ch,1Ch,1Ch
        db 1Ch,1Ch,1Ch,38h,0Eh,78h,0Fh,F0h,07h,E0h,0Fh,86h,0Fh,CEh,1Fh,CEh
        db 3Dh,FCh,38h,FCh,38h,78h,38h,70h,38h,78h,38h,78h,3Ch,FCh,1Fh,CEh
        db 0Fh,C6h,07h,06h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_27:
        db 00h,00h,00h,00h,03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h
        db 03h,C0h,07h,80h,07h,80h,0Fh,00h,0Eh,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_28:
        db 00h,00h,00h,00h,00h,1Ch,00h,3Ch,00h,78h,00h,F0h,01h,E0h,03h,C0h
        db 03h,C0h,07h,80h,07h,80h,0Fh,00h,0Fh,00h,0Fh,00h,0Eh,00h,0Eh,00h
        db 0Fh,00h,0Fh,00h,0Fh,00h,07h,80h,07h,C0h,03h,E0h,01h,F0h,00h,F0h
        db 00h,78h,00h,3Ch,00h,1Ch,00h,0Ch,00h,00h,00h,00h,00h,00h,00h,00h
chr_29:
        db 00h,00h,00h,00h,0Eh,00h,0Fh,00h,07h,80h,03h,C0h,01h,E0h,00h,F0h
        db 00h,F0h,00h,78h,00h,78h,00h,3Ch,00h,3Ch,00h,3Ch,00h,1Ch,00h,1Ch
        db 00h,3Ch,00h,3Ch,00h,3Ch,00h,78h,00h,F8h,01h,F0h,03h,E0h,03h,C0h
        db 07h,80h,0Fh,00h,0Eh,00h,0Ch,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_2A:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,38h,1Ch,3Ch,3Ch,3Eh,7Ch
        db 1Fh,F8h,0Fh,F0h,07h,E0h,7Fh,FEh,7Fh,FEh,7Fh,FEh,07h,E0h,0Fh,F0h
        db 1Fh,F8h,3Eh,7Ch,3Ch,3Ch,38h,1Ch,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_2B:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,01h,E0h
        db 01h,E0h,01h,E0h,01h,E0h,1Fh,FEh,1Fh,FEh,1Fh,FEh,1Fh,FEh,01h,E0h
        db 01h,E0h,01h,E0h,01h,E0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_2C:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,01h,C0h,03h,E0h,03h,E0h,03h,E0h
        db 03h,E0h,01h,E0h,01h,C0h,03h,C0h,03h,80h,07h,80h,0Fh,00h,0Ch,00h
chr_2D:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,1Fh,FEh,1Fh,FEh,1Fh,FEh,1Fh,FEh,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_2E:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,07h,80h,0Fh,C0h,0Fh,C0h,0Fh,C0h
        db 0Fh,C0h,07h,80h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_2F:
        db 00h,00h,00h,00h,00h,0Eh,00h,0Eh,00h,1Eh,00h,1Eh,00h,3Eh,00h,3Ch
        db 00h,7Ch,00h,78h,00h,F8h,00h,F0h,01h,F0h,01h,E0h,03h,E0h,03h,C0h
        db 07h,C0h,07h,80h,0Fh,80h,0Fh,00h,1Fh,00h,1Eh,00h,3Eh,00h,3Ch,00h
        db 7Ch,00h,78h,00h,F8h,00h,F0h,00h,F0h,00h,E0h,00h,E0h,00h,C0h,00h
chr_30:
        db 00h,00h,00h,00h,03h,F0h,0Fh,FCh,1Fh,FCh,3Eh,1Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,1Eh,3Ch,3Eh,3Ch,7Eh,3Ch,FEh,3Dh,EEh,3Fh,CEh,3Fh,8Eh
        db 3Fh,0Eh,3Eh,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Eh,1Eh,3Fh,FEh,1Fh,FCh
        db 0Fh,FCh,03h,F0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_31:
        db 00h,00h,00h,00h,00h,30h,00h,70h,00h,F0h,01h,F0h,03h,F0h,07h,F0h
        db 0Fh,F0h,00h,F0h,00h,F0h,00h,F0h,00h,F0h,00h,F0h,00h,F0h,00h,F0h
        db 00h,F0h,00h,F0h,00h,F0h,00h,F0h,00h,F0h,00h,F0h,00h,F0h,00h,F0h
        db 00h,F0h,00h,F0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_32:
        db 00h,00h,00h,00h,07h,F0h,1Fh,FCh,1Fh,FCh,3Eh,3Eh,3Ch,1Eh,3Ch,0Eh
        db 00h,0Eh,00h,0Eh,00h,1Eh,00h,3Eh,00h,7Ch,01h,FCh,03h,F0h,0Fh,E0h
        db 1Fh,80h,1Eh,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Fh,FEh,3Fh,FEh
        db 3Fh,FEh,3Fh,FEh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_33:
        db 00h,00h,00h,00h,07h,F0h,1Fh,FCh,1Fh,FCh,3Ch,1Eh,38h,0Eh,00h,0Eh
        db 00h,0Eh,00h,1Eh,00h,3Ch,03h,FCh,03h,F8h,03h,F8h,03h,FCh,00h,3Ch
        db 00h,1Eh,00h,0Eh,00h,0Eh,00h,0Eh,38h,0Eh,3Ch,1Eh,3Fh,FCh,1Fh,FCh
        db 0Fh,F8h,03h,E0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_34:
        db 00h,00h,00h,00h,00h,3Ch,00h,7Ch,00h,7Ch,00h,FCh,01h,FCh,01h,FCh
        db 03h,FCh,03h,BCh,07h,BCh,0Fh,3Ch,0Eh,3Ch,1Eh,3Ch,3Ch,3Ch,38h,3Ch
        db 70h,3Ch,7Fh,FEh,7Fh,FEh,7Fh,FEh,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch
        db 00h,3Ch,00h,3Ch,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_35:
        db 00h,00h,00h,00h,3Fh,FEh,3Fh,FEh,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h
        db 3Ch,00h,3Fh,F0h,3Fh,FCh,3Fh,FCh,38h,1Eh,00h,0Eh,00h,0Eh,00h,0Eh
        db 00h,0Eh,00h,0Eh,00h,0Eh,00h,0Eh,38h,0Eh,3Ch,1Eh,3Eh,3Eh,1Fh,FCh
        db 0Fh,F8h,03h,E0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_36:
        db 00h,00h,00h,00h,03h,F8h,0Fh,FCh,1Fh,FEh,1Eh,0Eh,3Ch,06h,3Ch,00h
        db 3Ch,00h,3Ch,00h,3Ch,00h,3Fh,F0h,3Fh,FCh,3Fh,FCh,3Eh,1Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,1Eh,1Eh,1Fh,FCh
        db 0Fh,FCh,03h,F0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_37:
        db 00h,00h,00h,00h,3Fh,FEh,3Fh,FEh,3Fh,FEh,00h,1Eh,00h,1Eh,00h,3Ch
        db 00h,3Ch,00h,3Ch,00h,78h,00h,78h,00h,70h,00h,F0h,00h,F0h,01h,E0h
        db 01h,E0h,03h,C0h,03h,C0h,07h,80h,07h,80h,07h,00h,0Fh,00h,0Eh,00h
        db 1Eh,00h,1Eh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_38:
        db 00h,00h,00h,00h,07h,F0h,1Fh,FCh,1Eh,3Ch,3Ch,1Eh,38h,0Eh,38h,0Eh
        db 38h,0Eh,3Ch,1Eh,1Eh,3Ch,0Fh,F8h,07h,F0h,0Fh,F8h,1Eh,3Ch,3Ch,1Eh
        db 38h,0Eh,38h,0Eh,38h,0Eh,38h,0Eh,38h,0Eh,3Ch,1Eh,3Eh,3Eh,1Fh,FCh
        db 0Fh,F8h,03h,E0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_39:
        db 00h,00h,00h,00h,07h,F0h,1Fh,FCh,1Eh,3Ch,3Ch,1Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Eh,1Eh,1Fh,FEh,1Fh,FEh,0Fh,FEh
        db 03h,EEh,00h,0Eh,00h,0Eh,00h,0Eh,00h,1Eh,00h,3Eh,0Fh,FCh,0Fh,FCh
        db 0Fh,F0h,0Fh,80h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_3A:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,01h,80h
        db 03h,C0h,07h,E0h,07h,E0h,03h,C0h,01h,80h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,01h,80h,03h,C0h,07h,E0h,07h,E0h,03h,C0h,01h,80h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_3B:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,01h,C0h
        db 03h,E0h,03h,E0h,03h,E0h,03h,E0h,01h,C0h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,01h,C0h,03h,E0h,03h,E0h,03h,E0h,03h,E0h,03h,E0h
        db 01h,E0h,01h,E0h,01h,C0h,03h,C0h,0Fh,80h,0Fh,00h,00h,00h,00h,00h
chr_3C:
        db 00h,00h,00h,00h,00h,06h,00h,0Eh,00h,1Eh,00h,3Eh,00h,7Eh,00h,FCh
        db 01h,F8h,03h,F0h,07h,E0h,0Fh,C0h,1Fh,80h,3Fh,00h,3Fh,00h,1Fh,80h
        db 0Fh,C0h,07h,E0h,03h,F0h,01h,F8h,00h,FCh,00h,7Eh,00h,3Eh,00h,1Eh
        db 00h,0Eh,00h,06h,00h,02h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_3D:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,3Fh,FEh
        db 3Fh,FEh,3Fh,FEh,3Fh,FEh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 3Fh,FEh,3Fh,FEh,3Fh,FEh,3Fh,FEh,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_3E:
        db 00h,00h,00h,00h,30h,00h,38h,00h,3Ch,00h,3Eh,00h,1Fh,00h,0Fh,80h
        db 07h,C0h,03h,E0h,01h,F0h,00h,F8h,00h,7Ch,00h,3Eh,00h,3Eh,00h,7Ch
        db 00h,F8h,01h,F0h,03h,E0h,07h,C0h,0Fh,80h,1Fh,00h,3Eh,00h,3Ch,00h
        db 38h,00h,30h,00h,20h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_3F:
        db 00h,00h,00h,00h,07h,F0h,1Fh,FCh,3Fh,FEh,3Ch,1Eh,38h,0Eh,00h,0Eh
        db 00h,0Eh,00h,1Eh,00h,3Eh,00h,7Ch,00h,F8h,01h,F0h,03h,E0h,03h,C0h
        db 03h,C0h,00h,00h,00h,00h,00h,00h,01h,80h,03h,C0h,07h,E0h,07h,E0h
        db 03h,C0h,01h,80h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_40:
        db 00h,00h,00h,00h,07h,F0h,0Fh,FCh,1Fh,FEh,3Eh,0Eh,3Ch,06h,3Ch,7Eh
        db 3Ch,FEh,3Ch,EEh,3Ch,C6h,3Ch,C6h,3Ch,C6h,3Ch,C6h,3Ch,C6h,3Ch,C6h
        db 3Ch,C6h,3Ch,EEh,3Ch,FEh,3Ch,7Ch,3Ch,00h,3Eh,00h,3Fh,FCh,1Fh,FCh
        db 1Fh,FCh,07h,FCh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_41:
        db 00h,00h,00h,00h,03h,E0h,07h,F0h,0Fh,F8h,1Fh,3Ch,3Eh,1Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Fh,FEh
        db 3Fh,FEh,3Fh,FEh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_42:
        db 00h,00h,00h,00h,3Fh,F8h,3Fh,FCh,3Fh,FEh,3Ch,1Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,1Eh,3Fh,FCh,3Fh,F8h,3Fh,F8h,3Fh,FCh,3Ch,1Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,1Eh,3Fh,FEh,3Fh,FEh
        db 3Fh,FCh,3Fh,F8h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_43:
        db 00h,00h,00h,00h,07h,F8h,1Fh,FCh,1Fh,FEh,3Eh,1Eh,3Ch,0Eh,3Ch,00h
        db 3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h
        db 3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,0Eh,3Eh,1Eh,1Fh,FEh,1Fh,FCh
        db 0Fh,FCh,03h,F0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_44:
        db 00h,00h,00h,00h,3Fh,F0h,3Fh,FCh,3Fh,FCh,3Ch,3Eh,3Ch,1Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,1Eh,3Ch,3Eh,3Fh,FEh,3Fh,FCh
        db 3Fh,F8h,3Fh,F0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_45:
        db 00h,00h,00h,00h,3Fh,FEh,3Fh,FEh,3Fh,FEh,3Ch,00h,3Ch,00h,3Ch,00h
        db 3Ch,00h,3Ch,00h,3Ch,00h,3Fh,F0h,3Fh,F0h,3Fh,F0h,3Fh,F0h,3Ch,00h
        db 3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Fh,FEh,3Fh,FEh
        db 3Fh,FEh,3Fh,FEh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_46:
        db 00h,00h,00h,00h,3Fh,FEh,3Fh,FEh,3Fh,FEh,3Ch,00h,3Ch,00h,3Ch,00h
        db 3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Fh,F0h,3Fh,F0h,3Fh,F0h
        db 3Fh,F0h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h
        db 3Ch,00h,3Ch,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_47:
        db 00h,00h,00h,00h,07h,F8h,0Fh,FCh,1Fh,FEh,3Eh,1Eh,3Ch,0Eh,3Ch,00h
        db 3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h
        db 3Ch,3Eh,3Ch,3Eh,3Ch,3Eh,3Ch,0Eh,3Ch,0Eh,3Eh,1Eh,3Fh,FEh,1Fh,FCh
        db 0Fh,FCh,07h,F0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_48:
        db 00h,00h,00h,00h,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Fh,FEh,3Fh,FEh,3Fh,FEh,3Fh,FEh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_49:
        db 00h,00h,00h,00h,03h,F8h,03h,F8h,03h,F8h,00h,E0h,00h,E0h,00h,E0h
        db 00h,E0h,00h,E0h,00h,E0h,00h,E0h,00h,E0h,00h,E0h,00h,E0h,00h,E0h
        db 00h,E0h,00h,E0h,00h,E0h,00h,E0h,00h,E0h,00h,E0h,03h,F8h,03h,F8h
        db 03h,F8h,03h,F8h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_4A:
        db 00h,00h,00h,00h,00h,0Eh,00h,0Eh,00h,0Eh,00h,0Eh,00h,0Eh,00h,0Eh
        db 00h,0Eh,00h,0Eh,00h,0Eh,00h,0Eh,00h,0Eh,00h,0Eh,00h,0Eh,00h,0Eh
        db 00h,0Eh,00h,0Eh,00h,0Eh,3Ch,0Eh,3Ch,0Eh,3Eh,1Eh,3Fh,FEh,1Fh,FCh
        db 1Fh,FCh,07h,F0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_4B:
        db 00h,00h,00h,00h,3Ch,06h,3Ch,0Eh,3Ch,1Eh,3Ch,3Eh,3Ch,7Ch,3Ch,F8h
        db 3Dh,F0h,3Fh,E0h,3Fh,C0h,3Fh,80h,3Fh,00h,3Eh,00h,3Fh,00h,3Fh,80h
        db 3Fh,C0h,3Fh,E0h,3Dh,F0h,3Ch,F8h,3Ch,7Ch,3Ch,3Eh,3Ch,1Eh,3Ch,0Eh
        db 3Ch,06h,3Ch,02h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_4C:
        db 00h,00h,00h,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h
        db 3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h
        db 3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Fh,FEh,3Fh,FEh
        db 3Fh,FEh,3Fh,FEh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_4D:
        db 00h,00h,00h,00h,78h,0Eh,78h,0Eh,7Ch,1Eh,7Ch,1Eh,7Eh,3Eh,7Eh,3Eh
        db 7Fh,7Eh,7Fh,7Eh,7Fh,7Eh,7Fh,FEh,7Bh,EEh,7Bh,EEh,79h,CEh,79h,CEh
        db 78h,8Eh,78h,8Eh,78h,0Eh,78h,0Eh,78h,0Eh,78h,0Eh,78h,0Eh,78h,0Eh
        db 78h,0Eh,78h,0Eh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_4E:
        db 00h,00h,00h,00h,3Ch,0Eh,3Ch,0Eh,3Eh,0Eh,3Eh,0Eh,3Fh,0Eh,3Fh,0Eh
        db 3Fh,0Eh,3Fh,8Eh,3Fh,8Eh,3Fh,CEh,3Fh,CEh,3Dh,EEh,3Dh,EEh,3Ch,FEh
        db 3Ch,FEh,3Ch,7Eh,3Ch,7Eh,3Ch,7Eh,3Ch,3Eh,3Ch,3Eh,3Ch,1Eh,3Ch,1Eh
        db 3Ch,0Eh,3Ch,0Eh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_4F:
        db 00h,00h,00h,00h,0Fh,F8h,1Fh,FCh,3Fh,FEh,3Eh,1Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Eh,1Eh,3Fh,FEh,3Fh,FEh
        db 1Fh,FCh,0Fh,F8h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_50:
        db 00h,00h,00h,00h,3Fh,F8h,3Fh,FCh,3Fh,FEh,3Ch,1Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,1Eh,3Fh,FEh,3Fh,FEh,3Fh,FCh
        db 3Fh,F8h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h
        db 3Ch,00h,3Ch,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_51:
        db 00h,00h,00h,00h,07h,F8h,1Fh,FCh,1Fh,FEh,3Eh,1Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,EEh,3Dh,FEh,3Ch,FEh,3Fh,FEh,1Fh,FEh
        db 1Fh,FCh,07h,F8h,00h,7Eh,00h,3Eh,00h,1Eh,00h,06h,00h,00h,00h,00h
chr_52:
        db 00h,00h,00h,00h,3Fh,F8h,3Fh,FCh,3Fh,FEh,3Ch,1Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,1Eh,3Ch,3Ch,3Fh,FCh,3Fh,F8h,3Fh,FCh
        db 3Fh,FCh,3Ch,1Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_53:
        db 00h,00h,00h,00h,0Fh,F8h,1Fh,FCh,3Fh,FEh,3Eh,1Eh,3Ch,0Eh,3Ch,00h
        db 3Eh,00h,3Fh,00h,1Fh,80h,0Fh,C0h,07h,E0h,03h,F0h,01h,F8h,00h,FCh
        db 00h,7Eh,00h,3Eh,00h,1Eh,00h,0Eh,3Ch,0Eh,3Ch,1Eh,3Fh,FEh,3Fh,FEh
        db 1Fh,FCh,0Fh,F8h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_54:
        db 00h,00h,00h,00h,1Fh,FEh,1Fh,FEh,1Fh,FEh,01h,E0h,01h,E0h,01h,E0h
        db 01h,E0h,01h,E0h,01h,E0h,01h,E0h,01h,E0h,01h,E0h,01h,E0h,01h,E0h
        db 01h,E0h,01h,E0h,01h,E0h,01h,E0h,01h,E0h,01h,E0h,01h,E0h,01h,E0h
        db 01h,E0h,01h,E0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_55:
        db 00h,00h,00h,00h,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Eh,1Eh,3Fh,FEh,1Fh,FCh
        db 1Fh,FCh,07h,F0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_56:
        db 00h,00h,00h,00h,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,1Eh,1Eh,0Fh,3Ch,0Fh,3Ch,07h,F8h,07h,F8h,03h,F0h,01h,E0h
        db 00h,C0h,00h,C0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_57:
        db 00h,00h,00h,00h,78h,0Eh,78h,0Eh,78h,0Eh,78h,0Eh,78h,8Eh,78h,8Eh
        db 78h,8Eh,79h,CEh,79h,CEh,7Bh,CEh,7Bh,EEh,7Bh,EEh,7Fh,FEh,7Fh,FEh
        db 7Fh,7Eh,7Fh,7Eh,7Eh,3Eh,7Eh,3Eh,7Eh,3Eh,7Ch,1Eh,7Ch,1Eh,7Ch,1Eh
        db 78h,0Eh,78h,0Eh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_58:
        db 00h,00h,00h,00h,38h,0Eh,3Ch,1Eh,1Ch,1Eh,1Eh,3Ch,0Eh,3Ch,0Eh,38h
        db 0Fh,78h,07h,70h,07h,F0h,07h,F0h,03h,E0h,03h,E0h,03h,E0h,07h,F0h
        db 07h,F0h,07h,70h,0Fh,78h,0Fh,78h,0Eh,3Ch,1Eh,3Ch,1Ch,1Ch,3Ch,1Eh
        db 38h,0Eh,38h,0Eh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_59:
        db 00h,00h,00h,00h,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Eh,1Eh,1Fh,3Eh,0Fh,FCh,07h,F8h,03h,F0h
        db 01h,E0h,01h,E0h,01h,E0h,01h,E0h,01h,E0h,01h,E0h,01h,E0h,01h,E0h
        db 01h,E0h,01h,E0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_5A:
        db 00h,00h,00h,00h,3Fh,FEh,3Fh,FEh,3Fh,FEh,00h,1Eh,00h,3Eh,00h,3Ch
        db 00h,7Ch,00h,78h,00h,F0h,00h,F0h,01h,E0h,01h,E0h,03h,C0h,03h,C0h
        db 07h,80h,0Fh,80h,0Fh,00h,1Eh,00h,1Eh,00h,3Ch,00h,3Fh,FEh,3Fh,FEh
        db 3Fh,FEh,3Fh,FEh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_5B:
        db 00h,00h,00h,00h,03h,FCh,03h,FCh,03h,FCh,03h,C0h,03h,C0h,03h,C0h
        db 03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h
        db 03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h
        db 03h,C0h,03h,FCh,03h,FCh,03h,FCh,03h,FCh,00h,00h,00h,00h,00h,00h
chr_5C:
        db 00h,00h,00h,00h,E0h,00h,E0h,00h,F0h,00h,70h,00h,78h,00h,38h,00h
        db 3Ch,00h,1Ch,00h,1Eh,00h,0Eh,00h,0Fh,00h,07h,00h,07h,80h,03h,80h
        db 03h,C0h,01h,C0h,01h,E0h,00h,E0h,00h,F0h,00h,70h,00h,78h,00h,38h
        db 00h,3Ch,00h,1Ch,00h,1Eh,00h,0Eh,00h,0Eh,00h,06h,00h,06h,00h,02h
chr_5D:
        db 00h,00h,00h,00h,0Fh,F0h,0Fh,F0h,0Fh,F0h,00h,F0h,00h,F0h,00h,F0h
        db 00h,F0h,00h,F0h,00h,F0h,00h,F0h,00h,F0h,00h,F0h,00h,F0h,00h,F0h
        db 00h,F0h,00h,F0h,00h,F0h,00h,F0h,00h,F0h,00h,F0h,00h,F0h,00h,F0h
        db 00h,F0h,0Fh,F0h,0Fh,F0h,0Fh,F0h,0Fh,F0h,00h,00h,00h,00h,00h,00h
chr_5E:
        db 00h,00h,00h,00h,00h,C0h,01h,E0h,03h,F0h,07h,F8h,0Fh,FCh,1Fh,3Eh
        db 1Eh,1Eh,1Ch,0Eh,18h,06h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_5F:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db FFh,FEh,FFh,FEh,FFh,FEh,FFh,FEh,00h,00h,00h,00h,00h,00h,00h,00h
chr_60:
        db 00h,00h,00h,00h,03h,E0h,01h,E0h,01h,F0h,00h,F0h,00h,F0h,00h,78h
        db 00h,78h,00h,3Ch,00h,1Ch,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_61:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,0Fh,F8h,0Fh,FCh,0Fh,FEh,0Fh,FEh,00h,0Eh,00h,0Eh,07h,FEh
        db 1Fh,FEh,1Fh,FEh,3Eh,1Eh,3Ch,0Eh,3Ch,0Eh,3Eh,0Eh,3Fh,FEh,1Fh,FEh
        db 1Fh,FEh,07h,FEh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_62:
        db 00h,00h,00h,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h
        db 3Ch,00h,3Fh,F0h,3Fh,FCh,3Fh,FCh,3Fh,FEh,3Ch,1Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,1Eh,3Fh,FEh,3Fh,FCh
        db 3Fh,FCh,3Fh,F0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_63:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,07h,F0h,1Fh,FCh,1Fh,FCh,3Fh,FEh,3Eh,1Eh,3Ch,0Eh,3Ch,00h
        db 3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,0Eh,3Eh,1Eh,3Fh,FEh,1Fh,FCh
        db 1Fh,FCh,07h,F0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_64:
        db 00h,00h,00h,00h,00h,0Eh,00h,0Eh,00h,0Eh,00h,0Eh,00h,0Eh,00h,0Eh
        db 00h,0Eh,07h,FEh,1Fh,FEh,1Fh,FEh,3Fh,FEh,3Eh,0Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Eh,0Eh,1Fh,FEh,1Fh,FEh
        db 0Fh,FEh,03h,FEh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_65:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,07h,F8h,1Fh,FCh,1Fh,FEh,3Eh,1Eh,3Ch,0Eh,3Eh,1Eh,3Fh,FEh
        db 3Fh,FCh,3Fh,F8h,3Ch,00h,3Ch,00h,3Ch,0Eh,3Eh,1Eh,3Fh,FEh,1Fh,FCh
        db 1Fh,FCh,07h,F0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_66:
        db 00h,00h,00h,00h,01h,F8h,03h,FCh,03h,FEh,03h,DEh,03h,CEh,03h,C0h
        db 03h,C0h,03h,C0h,03h,C0h,3Fh,FCh,3Fh,FCh,3Fh,FCh,3Fh,FCh,03h,C0h
        db 03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h
        db 03h,C0h,03h,C0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_67:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,07h,FEh,1Fh,FEh,1Fh,FEh,3Fh,FEh,3Eh,0Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Eh,0Eh,3Fh,FEh,1Fh,FEh
        db 1Fh,FEh,07h,FEh,00h,0Eh,00h,0Eh,00h,1Eh,0Fh,FEh,0Fh,FCh,0Fh,F8h
chr_68:
        db 00h,00h,00h,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h
        db 3Ch,00h,3Fh,F0h,3Fh,FCh,3Fh,FCh,3Fh,FEh,3Ch,1Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_69:
        db 00h,00h,00h,00h,00h,C0h,01h,E0h,01h,E0h,00h,C0h,00h,00h,00h,00h
        db 00h,00h,01h,E0h,01h,E0h,01h,E0h,01h,E0h,01h,E0h,01h,E0h,01h,E0h
        db 01h,E0h,01h,E0h,01h,E0h,01h,E0h,01h,E0h,01h,E0h,01h,E0h,01h,E0h
        db 01h,E0h,01h,E0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_6A:
        db 00h,00h,00h,00h,00h,1Ch,00h,3Eh,00h,3Eh,00h,1Ch,00h,00h,00h,00h
        db 00h,00h,00h,1Eh,00h,1Eh,00h,1Eh,00h,1Eh,00h,1Eh,00h,1Eh,00h,1Eh
        db 00h,1Eh,00h,1Eh,00h,1Eh,00h,1Eh,00h,1Eh,00h,1Eh,00h,1Eh,00h,1Eh
        db 00h,1Eh,00h,1Eh,1Ch,1Eh,1Eh,3Eh,1Fh,FEh,0Fh,FCh,0Fh,F8h,03h,E0h
chr_6B:
        db 00h,00h,00h,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h
        db 3Ch,00h,3Ch,0Eh,3Ch,1Eh,3Ch,3Eh,3Ch,7Ch,3Ch,F8h,3Dh,F0h,3Fh,E0h
        db 3Fh,C0h,3Fh,80h,3Fh,C0h,3Fh,E0h,3Dh,F0h,3Ch,F8h,3Ch,7Ch,3Ch,3Eh
        db 3Ch,1Eh,3Ch,0Eh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_6C:
        db 00h,00h,00h,00h,03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h
        db 03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h
        db 03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,E0h
        db 01h,FEh,00h,FEh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_6D:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,3Eh,3Ch,7Fh,FEh,7Fh,FEh,7Fh,FEh,7Fh,FEh,79h,CEh,79h,CEh
        db 79h,CEh,79h,CEh,79h,CEh,79h,CEh,79h,CEh,79h,CEh,78h,0Eh,78h,0Eh
        db 78h,0Eh,78h,0Eh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_6E:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,3Fh,F8h,3Fh,FCh,3Fh,FCh,3Fh,FEh,3Ch,1Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_6F:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,03h,F0h,0Fh,FCh,1Fh,FCh,1Fh,FEh,3Eh,1Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Eh,1Eh,3Fh,FEh,1Fh,FCh
        db 1Fh,FCh,07h,F0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_70:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,3Fh,F0h,3Fh,FCh,3Fh,FCh,3Fh,FEh,3Ch,1Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,1Eh,3Fh,FEh,3Fh,FCh
        db 3Fh,FCh,3Fh,F0h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h
chr_71:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,07h,FEh,1Fh,FEh,1Fh,FEh,3Fh,FEh,3Eh,0Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Eh,0Eh,3Fh,FEh,1Fh,FEh
        db 1Fh,FEh,07h,FEh,00h,0Eh,00h,0Eh,00h,0Eh,00h,0Eh,00h,0Eh,00h,0Eh
chr_72:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,3Dh,F8h,3Fh,FCh,3Fh,FEh,3Fh,FEh,3Ch,1Eh,3Ch,0Eh,3Ch,00h
        db 3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h,3Ch,00h
        db 3Ch,00h,3Ch,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_73:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,0Fh,F8h,1Fh,FCh,1Fh,FEh,3Eh,1Eh,3Ch,0Eh,3Eh,0Eh,1Fh,80h
        db 0Fh,E0h,07h,F8h,01h,FCh,00h,FEh,3Ch,3Eh,3Ch,0Eh,3Eh,1Eh,1Fh,FEh
        db 1Fh,FCh,0Fh,F8h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_74:
        db 00h,00h,00h,00h,00h,00h,03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,C0h
        db 03h,C0h,3Fh,FCh,3Fh,FCh,3Fh,FCh,3Fh,FCh,03h,C0h,03h,C0h,03h,C0h
        db 03h,C0h,03h,C0h,03h,C0h,03h,C0h,03h,CEh,03h,CEh,03h,FEh,01h,FEh
        db 01h,FCh,00h,78h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_75:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Eh,0Eh,3Fh,FEh,1Fh,FEh
        db 1Fh,FEh,07h,FEh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_76:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Eh,1Eh,1Fh,3Eh,0Fh,FCh,07h,F8h,03h,F0h
        db 01h,E0h,00h,C0h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_77:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,78h,0Eh,78h,0Eh,78h,0Eh,78h,0Eh,79h,CEh,79h,CEh,79h,CEh
        db 79h,CEh,79h,CEh,79h,CEh,7Bh,EEh,7Fh,FEh,3Fh,FEh,1Fh,7Ch,1Eh,3Ch
        db 0Ch,18h,08h,08h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_78:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,70h,0Eh,78h,0Eh,7Ch,1Eh,3Eh,3Eh,1Fh,7Ch,0Fh,FCh,0Fh,F8h
        db 07h,F0h,03h,E0h,07h,F0h,0Fh,F8h,1Fh,FCh,1Fh,7Ch,3Eh,3Eh,7Ch,1Eh
        db 78h,0Eh,70h,0Eh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_79:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh
        db 3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Ch,0Eh,3Eh,0Eh,3Fh,FEh,1Fh,FEh
        db 1Fh,FEh,07h,FEh,00h,0Eh,00h,0Eh,00h,1Eh,00h,3Eh,0Fh,FCh,0Fh,F8h
chr_7A:
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,3Fh,FEh,3Fh,FEh,3Fh,FEh,3Fh,FEh,00h,3Eh,00h,7Ch,00h,F8h
        db 01h,F0h,03h,E0h,07h,C0h,0Fh,80h,1Fh,00h,3Eh,00h,3Fh,FEh,3Fh,FEh
        db 3Fh,FEh,3Fh,FEh,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_7B:
        db 00h,00h,00h,00h,01h,F8h,03h,F0h,07h,C0h,07h,80h,07h,00h,07h,80h
        db 07h,80h,03h,C0h,03h,C0h,03h,80h,07h,80h,0Fh,00h,3Fh,00h,3Fh,00h
        db 0Fh,00h,07h,80h,03h,80h,03h,C0h,03h,C0h,07h,80h,07h,80h,07h,00h
        db 07h,80h,07h,C0h,03h,F0h,01h,F8h,00h,00h,00h,00h,00h,00h,00h,00h
chr_7C:
        db 00h,00h,00h,00h,03h,E0h,03h,E0h,03h,E0h,03h,E0h,03h,E0h,03h,E0h
        db 03h,E0h,03h,E0h,03h,E0h,03h,E0h,03h,E0h,03h,E0h,03h,E0h,03h,E0h
        db 03h,E0h,03h,E0h,03h,E0h,03h,E0h,03h,E0h,03h,E0h,03h,E0h,03h,E0h
        db 03h,E0h,03h,E0h,03h,E0h,03h,E0h,03h,E0h,03h,E0h,03h,E0h,00h,00h
chr_7D:
        db 00h,00h,00h,00h,07h,E0h,03h,F0h,00h,F8h,00h,78h,00h,78h,00h,78h
        db 00h,F0h,00h,F0h,00h,F0h,00h,F0h,00h,78h,00h,7Ch,00h,3Eh,00h,3Eh
        db 00h,7Ch,00h,78h,00h,F0h,00h,F0h,00h,F0h,00h,F0h,00h,78h,00h,78h
        db 00h,78h,00h,F8h,03h,F0h,07h,E0h,00h,00h,00h,00h,00h,00h,00h,00h
chr_7E:
        db 00h,00h,00h,00h,00h,00h,07h,0Eh,1Fh,9Eh,1Fh,FEh,3Fh,FCh,3Ch,FCh
        db 38h,70h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
chr_7F:
        db 00h,00h,00h,00h,07h,E0h,1Fh,F8h,1Fh,F8h,3Eh,7Ch,3Ch,3Ch,3Ch,3Ch
        db 3Ch,3Ch,3Eh,7Ch,1Fh,F8h,1Fh,F8h,07h,E0h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h
        db 00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h

;REV.  1.1    31 October 2002
;ADuC831   Apps, Analog Devices Inc.
P0       DATA  080H  ;PORT 0	
SP       DATA  081H  ;STACK POINTER	
DPL      DATA  082H  ;DATA POINTER - LOW BYTE
DPH      DATA  083H  ;DATA POINTER - HIGH BYTE
DPP      DATA  084H  ;DATA POINTER - PAGE BYTE
PCON     DATA  087H  ;POWER CONTROL
TCON     DATA  088H  ;TIMER CONTROL
TMOD     DATA  089H  ;TIMER MODE
TL0      DATA  08AH  ;TIMER 0 - LOW BYTE
TL1      DATA  08BH  ;TIMER 1 - LOW BYTE
TH0      DATA  08CH  ;TIMER 0 - HIGH BYTE
TH1      DATA  08DH  ;TIMER 1 - HIGH BYTE
P1       DATA  090H  ;PORT 1
SCON     DATA  098H  ;SERIAL PORT CONTROL
SBUF     DATA  099H  ;SERIAL PORT BUFFER
I2CDAT   DATA  09AH  ;I2C DATA REGISTER
I2CADD   DATA  09BH  ;I2C ADDRESS REGISTER
T3CON    DATA  09EH  ;TIMER 3 CONTROL
T3FD     DATA  09DH  ;TIMER 3 FRACTIONAL DIVIDER
P2       DATA  0A0H  ;PORT 2
TIMECON  DATA  0A1H  ;TIC CONTROL
HTHSEC   DATA  0A2H  ;TIC - HTHSEC DATA
SEC      DATA  0A3H  ;TIC - SEC DATA
MIN      DATA  0A4H  ;TIC - MIN DATA
HOUR     DATA  0A5H  ;TIC - HOUR DATA
INTVAL   DATA  0A6H  ;TIC INTERVAL REGISTER
DPCON    DATA  0A7H  ;DUAL DATA POINTER CONTROL REGISTER
IE       DATA  0A8H  ;INTERRUPT ENABLE 1
IEIP2    DATA  0A9H  ;INTERRUPT ENABLE 2
PWMCON   DATA  0AEH  ;PWM CONTROL REGISTER
CFG832   DATA  0AFH  ;GENERAL FLASH/PWM CONTROL REGISTER
P3       DATA  0B0H  ;PORT 3
PWM0L    DATA  0B1H  ;PWM DATA REGISTER
PWM0H    DATA  0B2H  ;PWM DATA REGISTER
PWM1L    DATA  0B3H  ;PWM DATA REGISTER
PWM1H    DATA  0B4H  ;PWM DATA REGISTER
SPH      DATA  0B7H  ;EXTENDED STACK POINTER REGISTER
IP       DATA  0B8H  ;INTERRUPT PRIORITY
ECON     DATA  0B9H  ;FLASH EEPROM CONTROL
EDATA1   DATA  0BCH  ;FLASH EEPROM DATA1 
EDATA2   DATA  0BDH  ;FLASH EEPROM DATA2 
EDATA3   DATA  0BEH  ;FLASH EEPROM DATA3 
EDATA4   DATA  0BFH  ;FLASH EEPROM DATA4 
WDCON    DATA  0C0H  ;WATCHDOG TIMER CONTROL
CHIPID   DATA  0C2H  ;CHIPID REGISTER
EADRL    DATA  0C6H  ;FLASH EEPROM PAGE ADDRESS - LOW BYTE
EADRH    DATA  0C7H  ;FLASH EEPROM PAGE ADDRESS - LOW BYTE
T2CON    DATA  0C8H  ;TIMER 2 CONTROL
RCAP2L   DATA  0CAH  ;TIMER 2 CAPTURE REGISTER - LOW BYTE
RCAP2H   DATA  0CBH  ;TIMER 2 CAPTURE REGISTER - HIGH BYTE
TL2      DATA  0CCH  ;TIMER 2 - LOW BYTE
TH2      DATA  0CDH  ;TIMER 2 - HIGH BYTE
PSW      DATA  0D0H  ;PROGRAM STATUS WORD
DMAL     DATA  0D2H  ;DMA ADDRESS LOW BYTE
DMAH     DATA  0D3H  ;DMA ADDRESS HIGH BYTE
DMAP     DATA  0D4H  ;DMA ADDRESS PAGE BYTE
PLLCON   DATA  0D7H  ;PLL CONTROL REGISTER
ADCCON2  DATA  0D8H  ;ADC CONTROL
ADCDATAL DATA  0D9H  ;ADC DATA LOW BYTE
ADCDATAH DATA  0DAH  ;ADC DATA HIGH BYTE
PSMCON   DATA  0DFH  ;POWER SUPPLY MONITOR
ACC      DATA  0E0H  ;ACCUMULATOR
DCON     DATA  0E8H  ;D1 AND D0 CONTROL
I2CCON   DATA  0E8H  ;I2C CONTROL
ADCCON1  DATA  0EFH  ;ADC CONTROL
B        DATA  0F0H  ;MULTIPLICATION REGISTER
ADCOFSL  DATA  0F1H  ;ADC OFFSET LOW BYTE
ADCOFSH  DATA  0F2H  ;ADC OFFSET HIGH BYTE
ADCGAINL DATA  0F3H  ;ADC GAIN LOW BYTE
ADCGAINH DATA  0F4H  ;ADC GAIN HIGH BYTE
ADCCON3  DATA  0F5H  ;ADC CONTROL
SPIDAT   DATA  0F7H  ;SPI DATA REGISTER
SPICON   DATA  0F8H  ;SPI CONTROL REGISTER
DAC0L    DATA  0F9H  ;DAC0 LOW BYTE
DAC0H    DATA  0FAH  ;DAC0 HIGH BYTE
DAC1L    DATA  0FBH  ;DAC1 LOW BYTE
DAC1H    DATA  0FCH  ;DAC1 HIGH BYTE
DACCON   DATA  0FDH  ;DAC CONTROL REGISTER
IT0      BIT   088H  ;TCON.0 - EXT. INTERRUPT 0 TYPE
IE0      BIT   089H  ;TCON.1 - EXT. INTERRUPT 0 EDGE FLAG
IT1      BIT   08AH  ;TCON.2 - EXT. INTERRUPT 1 TYPE
IE1      BIT   08BH  ;TCON.3 - EXT. INTERRUPT 1 EDGE FLAG
TR0      BIT   08CH  ;TCON.4 - TIMER 0 ON/OFF CONTROL
TF0      BIT   08DH  ;TCON.5 - TIMER 0 OVERFLOW FLAG
TR1      BIT   08EH  ;TCON.6 - TIMER 1 ON/OFF CONTROL
TF1      BIT   08FH  ;TCON.7 - TIMER 1 OVERFLOW FLAG
T2       BIT   090H  ;P1.0 - TIMER 2 TRIGGER INPUT
T2EX     BIT   091H  ;P1.1 - TIMER 2 COUNT INPUT
RI       BIT   098H  ;SCON.0 - RECEIVE INTERRUPT FLAG
TI       BIT   099H  ;SCON.1 - TRANSMIT INTERRUPT FLAG
RB8      BIT   09AH  ;SCON.2 - RECEIVE BIT 8
TB8      BIT   09BH  ;SCON.3 - TRANSMIT BIT 8
REN      BIT   09CH  ;SCON.4 - RECEIVE ENABLE
SM2      BIT   09DH  ;SCON.5 - SERIAL MODE CONTROL BIT 2
SM1      BIT   09EH  ;SCON.6 - SERIAL MODE CONTROL BIT 1
SM0      BIT   09FH  ;SCON.7 - SERIAL MODE CONTROL BIT 0
EX0      BIT   0A8H  ;IE.0 - EXTERNAL INTERRUPT 0 ENABLE
ET0      BIT   0A9H  ;IE.1 - TIMER 0 INTERRUPT ENABLE
EX1      BIT   0AAH  ;IE.2 - EXTERNAL INTERRUPT 1 ENABLE
ET1      BIT   0ABH  ;IE.3 - TIMER 1 INTERRUPT ENABLE
ES       BIT   0ACH  ;IE.4 - SERIAL PORT INTERRUPT ENABLE
ET2      BIT   0ADH  ;IE.5 - TIMER 2 INTERRUPT ENABLE
EADC     BIT   0AEH  ;IE.6 - ENABLE ADC INTURRUPT
EA       BIT   0AFH  ;IE.7 - GLOBAL INTERRUPT ENABLE
RXD      BIT   0B0H  ;P3.0 - SERIAL PORT RECEIVE INPUT
TXD      BIT   0B1H  ;P3.1 - SERIAL PORT TRANSMIT OUTPUT
INT0     BIT   0B2H  ;P3.2 - EXTERNAL INTERRUPT 0 INPUT
INT1     BIT   0B3H  ;P3.3 - EXTERNAL INTERRUPT 1 INPUT
T0       BIT   0B4H  ;P3.4 - TIMER 0 COUNT INPUT
T1       BIT   0B5H  ;P3.5 - TIMER 1 COUNT INPUT
WR       BIT   0B6H  ;P3.6 - WRITE CONTROL FOR EXT. MEMORY
RD       BIT   0B7H  ;P3.7 - READ CONTROL FOR EXT. MEMORY
PX0      BIT   0B8H  ;IP.0 - EXTERNAL INTERRUPT 0 PRIORITY
PT0      BIT   0B9H  ;IP.1 - TIMER 0 PRIORITY
PX1      BIT   0BAH  ;IP.2 - EXTERNAL INTERRUPT 1 PRIORITY
PT1      BIT   0BBH  ;IP.3 - TIMER 1 PRIORITY
PS       BIT   0BCH  ;IP.4 - SERIAL PORT PRIORITY
PT2      BIT   0BDH  ;IP.5 - TIMER 2 PRIORITY
PADC     BIT   0BEH  ;IP.6 - ADC PRIORITY
PSI      BIT   0BFH  ;IP.7 - SPI OR 2-WIRE SERIAL INTERFACE PRIORITY
WDWR     BIT   0C0H  ;WDCON.0 - WATCHDOG WRITE ENABLE
WDE      BIT   0C1H  ;WDCON.1 - WATCHDOG ENABLE
WDS      BIT   0C2H  ;WDCON.2 - WATCHDOG STATUS BIT
WDIR     BIT   0C3H  ;WDCON.3 - WATCHDOG INTERRUPT RESPONSE ENABLE BIT
PRE0     BIT   0C4H  ;WDCON.4 - WATCHDOG TIMEOUT SELECTION BIT0
PRE1     BIT   0C5H  ;WDCON.5 - WATCHDOG TIMEOUT SELECTION BIT1
PRE2     BIT   0C6H  ;WDCON.6 - WATCHDOG TIMEOUT SELECTION BIT2
PRE3     BIT   0C7H  ;WDCON.7 - WATCHDOG TIMEOUT SELECTION BIT3
CAP2     BIT   0C8H  ;T2CON.0 - CAPTURE OR RELOAD SELECT
CNT2     BIT   0C9H  ;T2CON.1 - TIMER OR COUNTER SELECT
TR2      BIT   0CAH  ;T2CON.2 - TIMER 2 ON/OFF CONTROL
EXEN2    BIT   0CBH  ;T2CON.3 - TIMER 2 EXTERNAL ENABLE FLAG
TCLK     BIT   0CCH  ;T2CON.4 - TRANSMIT CLOCK SELECT
RCLK     BIT   0CDH  ;T2CON.5 - RECEIVE CLOCK SELECTT
EXF2     BIT   0CEH  ;T2CON.6 - EXTERNAL TRANSITION FLAG
TF2      BIT   0CFH  ;T2CON.7 - TIMER 2 OVERFLOW FLAG
P        BIT   0D0H  ;PSW.0 - ACCUMULATOR PARITY FLAG
F1       BIT   0D1H  ;PSW.1 - FLAG 0
OV       BIT   0D2H  ;PSW.2 - OVERFLOW FLAG
RS0      BIT   0D3H  ;PSW.3 - REGISTER BANK SELECT 0
RS1      BIT   0D4H  ;PSW.4 - REGISTER BANK SELECT 1
F0       BIT   0D5H  ;PSW.5 - FLAG 0
AC       BIT   0D6H  ;PSW.6 - AUXILIARY CARRY FLAG
CY       BIT   0D7H  ;PSW.7 - CARRY FLAG
CS0      BIT   0D8H  ;ADCCON2.0 - ADC INPUT CHANNEL SELECT BIT0
CS1      BIT   0D9H  ;ADCCON2.1 - ADC INPUT CHANNEL SELECT BIT1
CS2      BIT   0DAH  ;ADCCON2.2 - ADC INPUT CHANNEL SELECT BIT2
CS3      BIT   0DBH  ;ADCCON2.3 - ADC INPUT CHANNEL SELECT BIT3
SCONV    BIT   0DCH  ;ADCCON2.4 - SINGLE CONVERSION ENABLE
CCONV    BIT   0DDH  ;ADCCON2.5 - CONTINUOUS CONVERSION ENABLE
DMA      BIT   0DEH  ;ADCCON2.6 - DMA MODE ENABLE
ADCI     BIT   0DFH  ;ADCCON2.7 - ADC INTURRUPT FLAG
D0EN     BIT   0EBH  ;DCON.3 - D0 ENABLE BIT
D0       BIT   0EDH  ;DCON.5 - D0 DATA BIT
D1EN     BIT   0EEH  ;DCON.6 - D1 ENABLE BIT
D1       BIT   0EFH  ;DCON.7 - D1 DATA BIT
I2CI     BIT   0E8H  ;I2CCON.0 - I2C INTURRUPT FLAG
I2CTX    BIT   0E9H  ;I2CCON.1 - I2C TRANSMIT SELECT
I2CRS    BIT   0EAH  ;I2CCON.2 - I2C RESET
I2CM     BIT   0EBH  ;I2CCON.3 - I2C MASTER MODE SELECT
MDI      BIT   0ECH  ;I2CCON.4 - I2C MASTER MODE SDATA INPUT
MCO      BIT   0EDH  ;I2CCON.5 - I2C MASTER MODE SCLOCK OUTPUT
MDE      BIT   0EEH  ;I2CCON.6 - I2C MASTER MODE SDATA ENABLE
MDO      BIT   0EFH  ;I2CCON.7 - I2C MASTER MODE SDATA OUTPUT
SPR0     BIT   0F8H  ;SPICON.0 - SPI BITRATE SELECT BIT0
SPR1     BIT   0F9H  ;SPICON.1 - SPI BITRATE SELECT BIT1
CPHA     BIT   0FAH  ;SPICON.2 - SPI CLOCK PHASE SELECT
CPOL     BIT   0FBH  ;SPICON.3 - SPI CLOCK POLARITY SELECT
SPIM     BIT   0FCH  ;SPICON.4 - SPI MASTER/SLAVE MODE SELECT
SPE      BIT   0FDH  ;SPICON.5 - SPI INTERFACE ENABLE
WCOL     BIT   0FEH  ;SPICON.6 - SPI WRITE COLLISION ERROR FLAG
ISPI     BIT   0FFH  ;SPICON.7 - SPI END OF TRANSFER FLAG
end