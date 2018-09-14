<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 128
nchnls = 2
0dbfs = 1.0

instr 1
  ifreq = 440
  a0 oscili 0.5, ifreq
  kshift line -12, p3, 12
  a1 fa_transpose a0, 0.050, 0.015, kshift
  outch 1, a0
  outch 2, a1
endin

</CsInstruments>
<CsScore>
i 1 0 20

</CsScore>
</CsoundSynthesizer>

