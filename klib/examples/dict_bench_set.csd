<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

 
ksmps = 64
nchnls = 2

    
instr 1
  ; tests dict_set performance
  idict dict_new "sf"
  kt timeinstk
  kcnt = 0
  while kcnt < 2000 do
    Sbar = sprintfk("bar%d", kcnt+kt*10000)
    dict_set idict, Sbar, kcnt
    kcnt += 1
  od
endin

instr 2
  ; tests chnset performance
  kt timeinstk
  kcnt = 0
  while kcnt < 2000 do
    Sbar = sprintfk("bar%d", kcnt+kt*10000)
    chnset kcnt, Sbar
    kcnt += 1
  od
endin

schedule $INSTNUM, 0, 1
    
</CsInstruments>
<CsScore>
f 0 1
</CsScore>
</CsoundSynthesizer>