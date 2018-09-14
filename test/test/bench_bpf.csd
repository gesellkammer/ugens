OB<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 48000
ksmps = 128
nchnls = 2
0dbfs = 1.0

#define N #1000#

instr 1
	kamp = line(0, p3, 1)
	kidx = 0
	i_amp2press1 ftgenonce 0,0,110,-27,\
		0,0,     10,0.7, 20, 0.8, 80,0.9,     100,0.99 , 101,0.99 
	while kidx < $N do
		kpress1 = tablei(kamp*100, i_amp2press1)
		kidx += 1
		; printk2 kpress1
	od
endin

instr 2
	kamp = line(0, p3, 1)
	kidx = 0
	while kidx < $N do
		kpress1 bpf kamp, 0,0,  0.1,0.7,  0.2, 0.8,  0.8, 0.9,  1,0.99
		kidx += 1
		; printk2 kpress1
	od
endin

</CsInstruments>
<CsScore>
i2 0 100
</CsScore>
</CsoundSynthesizer>
