<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o oscili.wav -W ;;; for file output any platform
-b 256
-B 1024
-+rtaudio=jack
-+jack_client=beadsynt
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 128
nchnls = 2
0dbfs  = 1

opcode empty, i,i
	isize xin
	iout ftgen 0,0, nxtpow2(isize), -2, 0
	xout iout
endop

giSine	 ftgen 0, 0, 2^15, 10, 1

gifreqs  ftgen 0, 0, nxtpow2(500), -7,   0, 400, 0
; gifreqs = empty:i(500)
giamps   ftgen 0, 0, nxtpow2(500), -7,   db(-40), 400, db(-40)
gibws	   ftgen 0, 0, nxtpow2(500), -17,	0, 0.1

instr 100
	/*
	out,  *kfreq, *kbw, *ifreqtbl, *iamptbl, *ibwtbl, *icnt, 
        *ifn, *iphs, *iflags;
	*/
	aout beadsynt 1, 0.1, gifreqs, giamps, gibws, 400, -1, -1, 0
	; aout adsynt2 1, 1, -1, gifreqs,giamps, 400, 2
	aout *= 0.2
	outch 1, aout
	outch 2, aout
endin 

instr 1
	k0 init 0
	kf0 linseg 200, 2, 200, 4, 800, 4, 800
	k0 = 0
	while k0 < 400 do
		k0 = k0 + 1
		kf = kf0 * (1 + k0 * 0.1)
		kf = kf < 20000 ? kf : 0.01
		kbw unirand 0.4
		kbw = kbw > 0.000001 ? kbw : 0 
		tabw kf, k0, gifreqs
		tabw kbw, k0, gibws
	od
endin


instr 3
	k0 = 0
	while (k0 < 400) do
		kv = tab:k(k0, gifreqs)
		k0 = k0 + 1
	od
endin

</CsInstruments>
<CsScore>
i 1   0 20
i 3   0 20
i 100 0 20

</CsScore>
</CsoundSynthesizer> 
<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>0</x>
 <y>0</y>
 <width>443</width>
 <height>272</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="nobackground">
  <r>255</r>
  <g>255</g>
  <b>255</b>
 </bgcolor>
 <bsbObject version="2" type="BSBScope">
  <objectName/>
  <x>93</x>
  <y>122</y>
  <width>350</width>
  <height>150</height>
  <uuid>{2740c500-5c30-4512-90a2-802b431ceae3}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <value>1.00000000</value>
  <type>scope</type>
  <zoomx>2.00000000</zoomx>
  <zoomy>9.00000000</zoomy>
  <dispx>1.00000000</dispx>
  <dispy>1.00000000</dispy>
  <mode>0.00000000</mode>
 </bsbObject>
</bsbPanel>
<bsbPresets>
</bsbPresets>
