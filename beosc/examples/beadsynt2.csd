<CsoundSynthesizer>
<CsOptions>
--omacro:MATRIXPATH=/home/em/tmp/py/out.wav
-+rtaudio=jack
-odac

</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 64
nchnls = 2
0dbfs = 1.0

#define MAXPARTIALS        #1024#

opcode ftempty,i,i
	isize xin
	ifnum ftgen 0, 0, isize, -7, 0
	xout ifnum
endop

opcode sndread,i,S
	Spath xin
	ifnum ftgen 0, 0, 0, -1, Spath, 0, 0, 0
	xout ifnum
endop

gkFilterActive init 1
gkFilterFreq0 init 400
gkFilterFreq1 init 24000
gkMinBw init 0
gkMaxBw init 1

gkSpeed init 1
gkTime init 0
gkFreqScale init 1
gkBwScale init 1
gkBwExp init 1

; --------- tables ----------
gifreqs ftempty $MAXPARTIALS
giamps  ftempty $MAXPARTIALS
gibws   ftempty $MAXPARTIALS
gispectrum sndread "$MATRIXPATH"

gidur init 0

chn_k "speed", 1
chn_k "reltime", 3
chn_k "minfreq", 1
chn_k "maxfreq", 1
chn_k "minbw", 1
chn_k "maxbw", 1
chn_k "freqmul", 1
chn_k "bwmul", 1
chn_k "ctrl", 1, 1, 0

; --------------------------------------------

instr ctrl
	kusechannels chnget "ctrl"
	if kusechannels == 0 kgoto skip
	
	kreltime chnget "reltime"
	if changed(kreltime) == 1 then
	  gkTime = kreltime * gidur
	endif
	
	gkSpeed        chnget "speed"
	gkFilterActive chnget "filteractive"
	gkFilterFreq0  chnget "minfreq"
	gkFilterFreq1  chnget "maxfreq"
	gkMinBw        chnget "minbw"
	gkMaxBw        chnget "maxbw"
	gkFreqScale    chnget "freqmul"
	gkBwScale      chnget "bwmul"
 	
	kuitrig metro 12
	if (kuitrig == 1) then
		chnset gkTime / gidur, "reltime"
	endif
skip:
endin

opcode tabfilter,0,iiiikkkk
	itabfreq, itabamp, itabbw, imaxidx, kfreq0, kfreq1, kbw0, kbw1 xin
	kMask[] init imaxidx
	
	kF[] vecview itabfreq, 0, imaxidx
	kA[] vecview itabamp,  0, imaxidx	
	kB[] vecview itabbw,   0, imaxidx
	
	kMask = cmp(kfreq0,  "<" , kF, "<", kfreq1)
	kMask = cmp(kbw0, "<=", kB, "<", kbw1) & kMask
	kA *= kMask
	
	viewend kF, kA, kB
endop

instr oscils
	ifn = gispectrum
	iskip    tab_i 0, ifn
	idt      tab_i 1, ifn
	inumcols tab_i 2, ifn
	inumrows tab_i 3, ifn
	inumpartials = inumcols / 3 
	imaxtime = (inumrows - 2) * idt
	gidur = imaxtime
	ikdur = ksmps / sr
	
	krow = gkTime / idt
	
	;          krow, ifn, ifndst,  inumcols  iskip  istart iend istep
	tabrowlin  krow, ifn, gifreqs, inumcols, iskip, 0,     0,   3
	tabrowlin  krow, ifn, giamps,  inumcols, iskip, 1,     0,   3
	tabrowlin  krow, ifn, gibws,   inumcols, iskip, 2,     0,   3
	
	if gkFilterActive == 1 then
		tabfilter gifreqs, giamps, gibws, inumpartials, \
		          gkFilterFreq0, gkFilterFreq1, gkMinBw, gkMaxBw
	endif
	
	iwavefn = -1  ; builtin sine wavetable
	iphases = -1  ; randomize phase of each oscil
	iflags = 0    ; 0-1=uniform / gaussian, +2=wavetab interp, +4=freq interp
	aout beadsynt gkFreqScale, gkBwScale, gifreqs, giamps, gibws, \ 
	              inumpartials, iwavefn, iphases, iflags
	outs aout, aout
	
  gkTime += ikdur * gkSpeed
  if (gkTime >= imaxtime) then
  	  gkTime = 0
  	elseif (gkTime < 0) then
  	  gkTime = imaxtime
  	endif
  
endin

</CsInstruments>
<CsScore>
i "ctrl" 0 -1
i "oscils" 0 -1
f0 100
</CsScore>
</CsoundSynthesizer>
















<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>0</x>
 <y>0</y>
 <width>368</width>
 <height>385</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="nobackground">
  <r>255</r>
  <g>255</g>
  <b>255</b>
 </bgcolor>
 <bsbObject version="2" type="BSBHSlider">
  <objectName>speed</objectName>
  <x>64</x>
  <y>37</y>
  <width>222</width>
  <height>23</height>
  <uuid>{3f214759-21da-46b1-a5ad-770fda951200}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <minimum>-20.00000000</minimum>
  <maximum>20.00000000</maximum>
  <value>0.05000000</value>
  <mode>lin</mode>
  <mouseControl act="jump">continuous</mouseControl>
  <resolution>-1.00000000</resolution>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject version="2" type="BSBSpinBox">
  <objectName>speed</objectName>
  <x>287</x>
  <y>36</y>
  <width>80</width>
  <height>25</height>
  <uuid>{e2e809a1-b820-48b7-8789-0650d8907202}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>10</fontsize>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <resolution>0.01000000</resolution>
  <minimum>-20</minimum>
  <maximum>20</maximum>
  <randomizable group="0">false</randomizable>
  <value>0.05</value>
 </bsbObject>
 <bsbObject version="2" type="BSBHSlider">
  <objectName>reltime</objectName>
  <x>64</x>
  <y>68</y>
  <width>304</width>
  <height>24</height>
  <uuid>{831a80f2-bf71-4c6c-8dad-90499ce66f07}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <minimum>0.00000000</minimum>
  <maximum>1.00000000</maximum>
  <value>0.79019630</value>
  <mode>lin</mode>
  <mouseControl act="jump">continuous</mouseControl>
  <resolution>-1.00000000</resolution>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>12</x>
  <y>37</y>
  <width>50</width>
  <height>25</height>
  <uuid>{6459c135-7e06-47c2-8e85-7633211faca1}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <label>Speed</label>
  <alignment>right</alignment>
  <font>Arial</font>
  <fontsize>10</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>9</x>
  <y>68</y>
  <width>53</width>
  <height>25</height>
  <uuid>{fb3a20b0-6e47-40d7-b63c-a0175db68bf5}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <label>Location</label>
  <alignment>right</alignment>
  <font>Arial</font>
  <fontsize>10</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBSpinBox">
  <objectName>maxfreq</objectName>
  <x>151</x>
  <y>291</y>
  <width>80</width>
  <height>25</height>
  <uuid>{0ff8841c-fdea-4256-91e7-fccda31b7f7f}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>14</fontsize>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <resolution>20.00000000</resolution>
  <minimum>0</minimum>
  <maximum>20000</maximum>
  <randomizable group="0">false</randomizable>
  <value>20000</value>
 </bsbObject>
 <bsbObject version="2" type="BSBKnob">
  <objectName>maxfreq</objectName>
  <x>151</x>
  <y>208</y>
  <width>80</width>
  <height>80</height>
  <uuid>{a4c06506-4799-482e-af17-160939c734c6}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <minimum>0.00000000</minimum>
  <maximum>20000.00000000</maximum>
  <value>20000.00000000</value>
  <mode>lin</mode>
  <mouseControl act="jump">continuous</mouseControl>
  <resolution>0.01000000</resolution>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject version="2" type="BSBSpinBox">
  <objectName>minfreq</objectName>
  <x>62</x>
  <y>291</y>
  <width>80</width>
  <height>25</height>
  <uuid>{a056f349-f584-4f84-b337-56993ae4789d}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>14</fontsize>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <resolution>20.00000000</resolution>
  <minimum>0</minimum>
  <maximum>20000</maximum>
  <randomizable group="0">false</randomizable>
  <value>1680</value>
 </bsbObject>
 <bsbObject version="2" type="BSBKnob">
  <objectName>minfreq</objectName>
  <x>62</x>
  <y>208</y>
  <width>80</width>
  <height>80</height>
  <uuid>{54f33b33-5f8c-4d34-acee-e5de1f4f3ddf}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <minimum>0.00000000</minimum>
  <maximum>8000.00000000</maximum>
  <value>1680.00000000</value>
  <mode>lin</mode>
  <mouseControl act="jump">continuous</mouseControl>
  <resolution>0.01000000</resolution>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject version="2" type="BSBSpinBox">
  <objectName>minbw</objectName>
  <x>63</x>
  <y>360</y>
  <width>80</width>
  <height>25</height>
  <uuid>{03ac5c34-760f-49be-adcf-be7acb94fae4}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>14</fontsize>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <resolution>0.00010000</resolution>
  <minimum>0</minimum>
  <maximum>1</maximum>
  <randomizable group="0">false</randomizable>
  <value>0</value>
 </bsbObject>
 <bsbObject version="2" type="BSBSpinBox">
  <objectName>maxbw</objectName>
  <x>148</x>
  <y>360</y>
  <width>80</width>
  <height>25</height>
  <uuid>{c2240384-f50a-4214-9c3c-084e629bec4b}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>14</fontsize>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <resolution>0.00010000</resolution>
  <minimum>0</minimum>
  <maximum>1</maximum>
  <randomizable group="0">false</randomizable>
  <value>1</value>
 </bsbObject>
 <bsbObject version="2" type="BSBCheckBox">
  <objectName>filteractive</objectName>
  <x>37</x>
  <y>183</y>
  <width>20</width>
  <height>20</height>
  <uuid>{cdccf707-d1b0-46e0-8086-d1d279837259}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <selected>true</selected>
  <label/>
  <pressedValue>1</pressedValue>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>63</x>
  <y>334</y>
  <width>80</width>
  <height>25</height>
  <uuid>{82345e87-33f3-46bb-ab45-8d4057674ee3}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <label>Bandwidth</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>10</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>61</x>
  <y>183</y>
  <width>80</width>
  <height>25</height>
  <uuid>{9f0e58d9-bcf4-44ec-98f0-896cd8e1163e}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <label>Frequency</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>10</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBSpinBox">
  <objectName>freqmul</objectName>
  <x>191</x>
  <y>110</y>
  <width>80</width>
  <height>25</height>
  <uuid>{b42d2fba-cdf3-4a13-aaa3-55e658e6c9d9}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>14</fontsize>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <resolution>0.01000000</resolution>
  <minimum>0</minimum>
  <maximum>10</maximum>
  <randomizable group="0">false</randomizable>
  <value>0.4</value>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>110</x>
  <y>110</y>
  <width>80</width>
  <height>25</height>
  <uuid>{cdcdd548-607b-4d1f-b300-aaf2058f1bb6}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <label>Freq. Scale</label>
  <alignment>right</alignment>
  <font>Arial</font>
  <fontsize>10</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBSpinBox">
  <objectName>bwmul</objectName>
  <x>191</x>
  <y>137</y>
  <width>80</width>
  <height>25</height>
  <uuid>{348e6439-ecae-4a7f-9042-16ab3ae335cc}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>14</fontsize>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <resolution>0.10000000</resolution>
  <minimum>0</minimum>
  <maximum>100</maximum>
  <randomizable group="0">false</randomizable>
  <value>1</value>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>53</x>
  <y>137</y>
  <width>137</width>
  <height>26</height>
  <uuid>{8fb14123-8b60-4b9c-b50f-48875f813fe7}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <label>Bandwidth Scale</label>
  <alignment>right</alignment>
  <font>Arial</font>
  <fontsize>10</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBCheckBox">
  <objectName>ctrl</objectName>
  <x>5</x>
  <y>5</y>
  <width>20</width>
  <height>20</height>
  <uuid>{70276f50-76b9-41ab-b4e6-ddf16fb68246}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <selected>true</selected>
  <label/>
  <pressedValue>1</pressedValue>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>27</x>
  <y>5</y>
  <width>80</width>
  <height>25</height>
  <uuid>{e80f15f9-b62a-4650-9002-4e167fc7878c}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <label>GUI ON</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>10</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
</bsbPanel>
<bsbPresets>
</bsbPresets>
