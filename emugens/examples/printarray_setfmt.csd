<CsoundSynthesizer>
<CsOptions>
;-odac     ;;;realtime audio out

</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 64
nchnls = 2
0dbfs  = 1

; Example file for printarray_setfmt

/*

printarray_setfmt

Sets global/default options for printarray
and ftprint

printarray_setfmt [iLinelength=0], [Sfmt="%.4f"]

Example
=======

printarray_setfmt 40, "%.2f"

Will set the global line length to 40 chars and the
default format used to print each element to "%.2f"
for any subsequent call to printarray or ftprint

See also: printarray, ftprint

*/

printarray_setfmt 92,       "%.3f"

instr 1
  ; test i-time, 1D
  ivalues[] fillarray 0, 1, 3, 5, 7, 9
  printarray ivalues                      ; default fmt, no label
  printarray ivalues, "%.1f"              ; with given fmt
  printarray ivalues, "", "ivalues = "    ; uses default fmt

  ; test i-time, long array
  ilong[] genarray 0, 100, 0.1
  printarray ilong
  printarray ilong, "%.2f", "long="

  ; 2D
  ivalues2[][] init 11, 4
  ivalues2 fillarray 0,   1,  2, 3, \
                     10, 11, 12, 13, \
                     20, 21, 22, 23, \
                     30, 31, 32, 33, \
                     40, 41, 42, 43, \
                     50, 51, 52, 53, \
                     60, 61, 62, 63, \
                     70, 71, 72, 73, \
                     80, 81, 82, 83, \
                     90, 91, 92, 93, \
                    100,101,102,103
  printarray ivalues2
  printarray ivalues2, "%.1f", "ivalues2="
  turnoff 
endin

instr 2
  ; k-time, 1D, print every cycle
  kxs[] fillarray 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
  printarray kxs, -1, "", "instr 2"
  kxs += 1
endin

instr 3
  kxs[] fillarray 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
  printarray kxs, metro(20), "%.0f", "kxs 1D="
  kxs += 1
  if kxs[0] > 1000 then
    turnoff
  endif
endin

</CsInstruments>

<CsScore>
i 1 0 0.01
i 2 1 0.05
; i 3 2 2
; i 4 2 2
</CsScore>

</CsoundSynthesizer>
