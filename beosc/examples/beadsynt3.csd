<CsoundSynthesizer>
<CsOptions>
-odac
-+rtaudio=jack
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 128
nchnls = 1
0dbfs = 1.0

gispectrum ftgen 0, 0, 0, -1, "countdown.mtx.wav", 0, 0, 0

gkFilterBw0 init 0
gkFilterBw1 init 1

gkFilterActive init 0
gkFilterFreq0 init 30
gkFilterFreq1 init 2000

instr resynt
  ifn = gispectrum
  iskip    tab_i 0, ifn
  idt      tab_i 1, ifn
  inumcols tab_i 2, ifn
  inumrows tab_i 3, ifn
  itimestart tab_i 4, ifn
  inumpartials = inumcols / 3 
  imaxrow = inumrows - 2
  it = ksmps / sr
  idur = imaxrow * idt
  kGains[] init inumpartials

  ; kspeed    transeg 0.25, idur*2, 10, 4
  kspeed init 1
  ; kfreqscale  transeg 2, idur, 4, 1
  kfreqscale init 1
  
  kplayhead = phasor:k(kspeed/idur)*idur
  krow = kplayhead / idt
  kF[] getrowlin krow, ifn, inumcols, iskip, 0, 0, 3
  kA[] getrowlin krow, ifn, inumcols, iskip, 1, 0, 3
  kB[] getrowlin krow, ifn, inumcols, iskip, 2, 0, 3
    
  if (gkFilterBw0 > 0 || gkFilterBw1 < 1) then
    kGains bpf kB, gkFilterBw0 - 0.01, 0, gkFilterBw0, 1, gkFilterBw1, 1, gkFilterBw1+0.001, 0
    kA *= kGains
  endif

  if (gkFilterActive == 1) then
    kGains bpf kF, gkFilterFreq0-30, 0.001, gkFilterFreq0, 1, gkFilterFreq1, 1, gkFilterFreq1+30, 0.001
    kA *= kGains
  endif 
  
  iflags = 0    ; uniform noise, no interpolation
  aout beadsynt kF, kA, kB, -1, iflags, kfreqscale
  
  aout *= 0.5
  outch 1, aout
  
endin

schedule "resynt", 0, -1

</CsInstruments>
<CsScore>
</CsScore>
</CsoundSynthesizer>