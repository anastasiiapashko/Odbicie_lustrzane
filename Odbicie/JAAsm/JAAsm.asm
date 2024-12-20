;Anastasiia Pashko
.code

;bitmap do rcx
;width do rdx
;height do r8
;startRow do r9
;endRow na stosie
;PSHUFB - funkcja, która modyfikuje rejestr xmm przesuwając bajty


;prawa
 OdbicieASM proc
mov rax, 08070C0B0A0F0E0Dh        ;Prepare the lower half of the mask
pinsrq xmm3, rax, 0                ;Insert first half of the mask to the xmm3
mov rax, 0003020106050409h        ;Prepare the upper half of the mask
pinsrq xmm3, rax, 1                ;Insert second half of the mask to the xmm3

;lewa 0F0C0D0E090A0B06 0708030405000102
;Prepare mask for saving pixels
mov rax, 060B0A090E0D0C0Fh        ;Prepare the lower half of the mask
pinsrq xmm7, rax, 0                ;Insert first half of the mask to the xmm7
mov rax, 0201000504030807h        ;Prepare the upper half of the mask
pinsrq xmm7, rax, 1                ;Insert second half of the mask to the xmm7

     xor r10, r10               ; czyścimy rejest, czyli upewniamy się, że nie ma w sobie śmieci  
     mov  r10d, [rsp+40]        ;zapisana zmienna endRow 
     mov r11, r9                ;licznik wierszy, przypisanie licznikowi startRow

    push rbx        ;zapisanie co jest w rejestrach
    push rcx
    push rdx

     ;obliczenie odleglosci od centrum dla danego odwróconego wektora
      
   RowLoop:

    mov rax, 3
     mov r14, rdx           ;zapisana width 
     mul rdx                ;obliczenie szerokości wiersza w bajtach
     mov r15, rax
     mul r11                ;index pixela, początek wiersza, start
     add r15,rax            ; wyliczamy koniec wiersza
     mov r13, rax           ;przesuniety do r13
;obliczenie odleglosci od centrum dla danego odwróconego wektora
     mov rax, r14           ; wczytanie szerokości
     sar rax, 1             ; przesuniecie arytmetycznie o jedna pozycje (dzielenie przez dwa)
mov rdx,r14                 ;przewracamy wysokość
mov r14,rax                 ;zapisuje połowę szerokości
sub r15, 16
     xor r12, r12              ;musimy zaczynac od zera

     movdqu xmm4, [rcx+r13]    ;xmm4 i xmm5 są po to aby nie nadpisywac 1 bitu podczas zapisywania następnego wektora
     movdqu xmm5, [rcx+r15]
     add r12, 5
        KolLoop:
        sub r12, 5
            movdqu xmm1, xmm4               ;wpisujemy zapisany wektor do pustego wektora
            movdqu xmm0, xmm5               ;wpisujemy zapisany wektor do pustego wektora
            movdqu xmm4, [rcx+r13+15]       ;wczytujemy wektor na nasteępny przebieg pętli z lewej 
            movdqu xmm5, [rcx+r15-15]       ;wczytujemy wektor na nasteępny przebieg pętli z prawej
            pshufb xmm1, xmm7               ;odbijamy lustrzanie wektor
            pshufb xmm0, xmm3               ;odbijamy lustrzanie wektor
            movdqu [rcx+r13], xmm0          ;zapisuje w miejscu drugiego wektora na drugiej stronie 
            movdqu [rcx+r15], xmm1          ;zapisuje w miejscu drugiego wektora na drugiej stronie

            add r13, 15
            sub r15, 15          ; dekrementuje(przesuwa) adres kolejnego wektora na jego umiejscowienie po odwróceniu
        add r12, 10               ;inkrementacja licznika co 10 pixeli
        cmp r12, r14             ;czy licznik rowna sie połówie szerokości
        jnge KolLoop                ;skok warunkowy !>=

        sub r12, 5               ;przewracamy do 15
        mov r8, r14              ;ile jeszcze pixeli musimy przenieść na inna stronę
        sub r8, r12              ;i dostajemy o ile wyszliśmy
        add r15, 13

         movdqu xmm2, xmm5          
         vpextrb rax, xmm2, 15      ;wyciągniety ostatni 16 bajt(kolor) z prawej
         mov [rcx+r15+2], al        ;zapisany z powrotem do pamięci

         movdqu xmm2, xmm4
         vpextrb rax, xmm2, 0       ;wyciągniety ostatni 16 bajt(kolor) z lewej
         mov [rcx+r13], al          ;zapisany z powrotem do pamięci

        OstatniWektor:
                    
            mov bl, [rcx+r13]       ;wczytujemy kolor na obecny przebieg pętli z lewej 
            mov al, [rcx+r15]       ;wczytujemy kolor na obecny przebieg pętli z prawej
            mov [rcx+r15], bl       ;zapisujemy pojedynczy kolor na obecny przebieg pętli z lewej 8-bitowe
            mov [rcx+r13], al       ;zapisujemy pojedynczy kolor na obecny przebieg pętli z prawej 8-bitowe


            mov bl, [rcx+r13+1]     ;wczytujemy kolor + 1 na obecny przebieg pętli z lewej 
            mov al, [rcx+r15+1]     ;wczytujemy kolor + 1 na obecny przebieg pętli z prawej
            mov [rcx+r15+1], bl       ;zapisujemy pojedynczy kolor+1 na obecny przebieg pętli z lewej 8-bitowe
            mov [rcx+r13+1], al       ;zapisujemy pojedynczy kolor+1 na obecny przebieg pętli z prawej 8-bitowe

            mov bl, [rcx+r13+2]     ;wczytujemy  kolor+2 na obecny przebieg pętli z lewej 
            mov al, [rcx+r15+2]     ;wczytujemy kolor+2 na obecny przebieg pętli z prawej
            mov [rcx+r15+2], bl       ;zapisujemy  kolor+2 na obecny przebieg pętli z lewej 8-bitowe
            mov [rcx+r13+2], al       ;zapisujemy  kolor+2 na obecny przebieg pętli z prawej 8-bitowe

            add r13, 3              ; inkrementuje(przesuwa) adres kolejnego wektora na jego umiejscowienie po odwróceniu z prawej
            sub r15, 3              ; dekrementuje(przesuwa) adres kolejnego wektora na jego umiejscowienie po odwróceniu z prawej
            dec r8                  ;dekrementacja licznika
        cmp r8, 0
        jne OstatniWektor           ;skok warunkowy jezeli jest różny od 0

     inc r11 ;inkrementacja licznika o 1 wiersz
     cmp r11, r10 ;czy licznik rowna sie endRow
   jne RowLoop

pop rdx         ;przywrócenie wartości
pop rcx
pop rbx
 ret
 OdbicieASM endp


  PomiarCzasu proc
     
     rdtsc ;wczytuje ticks procesora do pary edx (dolna polowa rdx) i eax (dolna polowa rax)
     shl rdx, 32  ;shift left
     add rax, rdx ;dodajemy polowki czasowe do jednego rejestru rax
     
 ret
 PomiarCzasu endp


 end

 