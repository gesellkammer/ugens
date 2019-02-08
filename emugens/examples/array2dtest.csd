<CsoundSynthesizer>
<CsOptions>
;-odac     ;;;realtime audio out

</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 64
nchnls = 2
0dbfs  = 1

; you can set some format options globally
;                 linewidth fmt
printarray_setfmt 70,       "%.4f"

instr 1
  kvalues[] init 20
  kvalues genarray 0, 19, 1
  kvalues2[] = kvalues * 2
  printarray kvalues2
  turnoff
endin

instr 2
  ; test i-time, 1D
  kvalues2[][] init 6, 3
  kvalues2 fillarray 0,   1,  2, \
                     10, 11, 12, \
                     20, 21, 22, \
                     30, 31, 32, \
                     40, 41, 42, \
                     50, 51, 52
  ; ivalues2 *= 100
  kvalues3[][] init 6, 3
  kvalues2 *= 100
  printarray kvalues2
  printk 1, kvalues2[4][2]
  turnoff 
endin

</CsInstruments>

<CsScore>
; i 1 0 0.01
i 2 0 0.01
</CsScore>

</CsoundSynthesizer>
