<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

ksmps = 128
nchnls = 2

instr 1
  kx line -1, p3, 2.5

  ky bpf kx,        \
         0,    0,   \
         1.01, 10,  \
         2,    0.5, \
         2.5,  -1

  ; the same expressed as a table (notice that we need to scale and
  ; limit kx). It gets even more complicated with breakpoints with
  ; negative x
  itab ftgenonce 0, 0, 1000, -27,  \
         0, 0,    \
       101, 10,   \
       200, 0.5,  \
       250, -1,   \
      1000, -1

  ky2 tablei limit(kx, 0, 2.5)*100, itab

  printks "kx: %f   ky: %f   ky2: %f \n", 1/kr, kx, ky, ky2

endin

instr 2
  kx linseg 0, p3, 7
  ky bpf kx, \
  0,0,       \
  0.5,5,     \
  1,10,      \
  1.5,15,    \
  2,20,      \
  2.5,25,    \
  3,30,      \
  3.5,35,    \
  4,40,      \
  4.5,45,    \
  5,50,      \
  5.5,55,    \
  6,60,      \
  6.5,65

endin

instr 3
  kx linseg 0, p3, 7
  ky bpf kx, 0,0,  0.5,50,  1,100,  2,200,  2.5,250,  6.5,650
endin
</CsInstruments>
<CsScore>
i 3 0 1000

</CsScore>
</CsoundSynthesizer>
