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
  ktrig metro 1
  if ktrig == 1 then
    reinit env
  endif
  kshift line -6, p3, 6
  
env:  
  aenv xadsr 0.010, 0.6, 0.0001, 0.1
  a0 oscili 1, ifreq
  a0 *= aenv
  
  a1 fa_transpose a0, 0.050, 32/sr, kshift
  
  rireturn 
  outch 1, a0
  outch 2, a1
endin

</CsInstruments>
<CsScore>
i 1 0 20

</CsScore>
</CsoundSynthesizer>

<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>100</x>
 <y>100</y>
 <width>320</width>
 <height>240</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="nobackground">
  <r>255</r>
  <g>255</g>
  <b>255</b>
 </bgcolor>
</bsbPanel>
<bsbPresets>
</bsbPresets>
