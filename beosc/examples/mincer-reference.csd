<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o mincer.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 64
nchnls = 2
0dbfs  = 1

instr 1

idur  = p3
ilock = p4
ipitch = 1
itimescale = 0.5
iamp  = 0.8
ktime init 0
kspeed = 0.01
ikdur = ksmps/sr
atime = interp(ktime)
; atime line   0,idur,idur*itimescale
asig  mincer atime, iamp, ipitch, 1, ilock, 2048, 32
      outs asig, asig
      

ktime += ikdur * kspeed

endin
</CsInstruments>
<CsScore>
f 1 0 0 1 "/home/em/tmp/py/colours-1.wav" 0 0 0

i1 0 -1 1	;locked

f0 3600
e

</CsScore>
</CsoundSynthesizer> 
<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>0</x>
 <y>0</y>
 <width>0</width>
 <height>0</height>
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
