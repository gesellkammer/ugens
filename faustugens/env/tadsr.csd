<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 128
nchnls = 2
0dbfs = 1.0

instr 1
  ktrig = sc_trig(metro(0.5), 0.001)
  aenv = fa_tadsr(ktrig, 0.01, 0.5, 1, 0.75)
  asig oscili 1, 1000
  outch 1, asig * aenv
  outch 2, asig * ktrig
endin

</CsInstruments>
<CsScore>
i 1 0 10

</CsScore>
</CsoundSynthesizer>

