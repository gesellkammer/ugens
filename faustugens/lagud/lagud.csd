<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 128
nchnls = 2
0dbfs = 1.0

instr 1
  kmidis[] array 60, 65, 60, 65, 60
  ilen = lenarray(kmidis)
  kidx = int(linseg(0, ilen*2, ilen-0.00000001))
  afreq = upsamp(mtof(kmidis[kidx]))
  afreq2 fa_lagud afreq, 1, 0.1
  a0 = oscili(0.7, afreq2)
  outch 1, a0 
endin

</CsInstruments>
<CsScore>
i 1 0 12

</CsScore>
</CsoundSynthesizer>

