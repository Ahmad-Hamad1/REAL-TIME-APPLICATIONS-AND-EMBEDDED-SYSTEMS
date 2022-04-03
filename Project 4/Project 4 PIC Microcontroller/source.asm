PROCESSOR 16F877
__CONFIG 0x3731
include "P16f877a.INC" 
cblock 0x70
	c1
	c2
	c3
	c4
	temp
	inc_val
	sleep_val
	current
endc

org 00
goto start
org 04
goto ISR

start

movlw d'1'
movwf inc_val

movlw d'1'
movwf sleep_val

movlw d'0'
movwf current

banksel TRISB
movlw b'00010001'
movwf TRISB

banksel TRISD
movlw b'00000000'
movwf TRISD

banksel PORTD
movlw d'0'
movwf PORTD

banksel INTCON
movlw b'10011000'
movwf INTCON



banksel OPTION_REG
bcf OPTION_REG,7

loop 
	banksel PORTD
	movf inc_val,w
	addwf current,f

	btfss current,5
	goto no_carry

	carry
	movlw d'0'
	movwf current
	movwf PORTD
	goto enn
	
	no_carry
	banksel PORTD
	movf current,w
	movwf PORTD
	
	enn
	call fun_delay
	clrwdt
	goto loop


fun_delay

	movf sleep_val,w
	movwf temp
	movlw 0

	loop5
		addlw d'4'
		decfsz temp,f
		goto loop5

	movwf c3
	loop3
	    movlw d'200'
	    movwf c2
	    loop2
	        movlw d'250'
	        movwf c1
	        loop1
	            nop
	            nop
	            decfsz c1, f
	        goto loop1
	        decfsz c2, f
	    goto loop2
	    decfsz c3, f
	
	goto loop3


return


ISR
	banksel INTCON
	btfsc INTCON, INTF
	goto first
	
	btfss INTCON,RBIF
	goto donee
	
	banksel PORTB
	btfsc PORTB,4
	goto second
	goto done
	
	first
		movf inc_val,w
		addlw d'1'
		movwf inc_val
		sublw d'6'
		banksel STATUS
		btfss STATUS,2
		goto done
		movlw d'1'
		movwf inc_val
		goto done
	
	second
		movf sleep_val,w
		addlw d'1'
		movwf sleep_val
		sublw d'6'
		banksel STATUS
		btfss STATUS,2
		goto done
		movlw d'1'
		movwf sleep_val
	
	done
		banksel INTCON
		bcf INTCON,RBIF
		bcf INTCON,INTF

donee retfie
end



