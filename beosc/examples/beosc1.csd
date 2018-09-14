<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o oscili.wav -W ;;; for file output any platform
-b 2048
-B 4096
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 64
nchnls = 2
0dbfs  = 1

giSine	 ftgen 0, 0, 2^15, 10, 1

instr 1
	i0 = 0.000000001
	idur1 = 8
	kfreq linseg 440, idur1, 440, 1, 880*6, idur1, 880*1
	aenv  linsegr 0, 0.1, 1, 0.1, 1, 0.1, 0
	; kbw   linseg i0,   8,   0.1, 8, 0.1, 4, i0
	kbw transeg i0, idur1, 8, 0.05, 1, 0, 0.05, idur1, 0.5, i0
	; printk 0.1, kbw
  ;           freq   bw     fn  phs           noisetype(0=gauss, not interpolated)
	; aout beosc interp(kfreq, 0, 1), kbw*2, -1, unirand:i(6.28), 0
	aout beosc kfreq, kbw*2, -1, unirand:i(6.28), 0
	; aout oscil 1, kfreq	
	aenv *= 0.2
	aout *= aenv
	outs aout, aout 
endin

</CsInstruments>
<CsScore>
i 1 0 100
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
