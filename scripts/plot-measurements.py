"""Render exported, actual DSP measurements; no simulated meter or response data."""
import csv,pathlib,sys
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
root=pathlib.Path(sys.argv[1]);plt.style.use('dark_background')
def read(name):return list(csv.DictReader((root/name).open()))
def save(name):
 plt.tight_layout();plt.savefig(root/(name+'.png'),dpi=160);plt.savefig(root/(name+'.svg'));plt.close()
rows=read('transfer.csv');x=np.array([float(r['input']) for r in rows])
for family,names in [('clippers',['linear','hard','classic','modern']),('gold',['linear','goldModern','goldClassic','goldUnity'])]:
 plt.figure(figsize=(8,5))
 for name in names:plt.plot(x,[float(r[name]) for r in rows],label=name)
 plt.xlabel('Input amplitude');plt.ylabel('Output amplitude');plt.title('Measured independent '+family+' transfers, ceiling 0.5');plt.legend();plt.grid(alpha=.2);save(family+'-transfer')
rows=read('box-tone.csv');plt.figure(figsize=(8,5))
for rate in [44100,48000,96000]:
 for mode in [1,2]:
  selected=[r for r in rows if int(r['rate'])==rate and int(r['mode'])==mode];plt.semilogx([float(r['hz']) for r in selected],[float(r['db']) for r in selected],label=f'{rate} Hz, mode {mode}')
plt.xlim(100,24000);plt.ylim(-1,.2);plt.xlabel('Frequency (Hz)');plt.ylabel('Gain (dB)');plt.title('Actual Box Tone FIR response');plt.legend(fontsize=8);plt.grid(alpha=.2);save('box-tone-response')
rows=read('alchemy.csv');plt.figure(figsize=(8,5))
for amplitude in [.05,.25,.5,.98]:
 selected=[r for r in rows if float(r['amplitude'])==amplitude];plt.semilogx([float(r['hz']) for r in selected],[float(r['attenuation_db']) for r in selected],marker='o',label=f'Input peak {amplitude}')
plt.xlabel('Frequency (Hz)');plt.ylabel('Measured RMS change (dB)');plt.title('Alchemy amount 2 dB: level-dependent contour');plt.legend();plt.grid(alpha=.2);save('alchemy-response')
rows=read('alias.csv');plt.figure(figsize=(8,5))
for phase in [0,1]:
 for quality in [0,1,2]:
  selected=[r for r in rows if int(r['phase'])==phase and int(r['quality'])==quality];plt.plot([float(r['tone_hz']) for r in selected],[float(r['suppression_db']) for r in selected],marker='o',label=f'{"FIR" if phase==0 else "IIR"}, {4*2**quality}x')
plt.xlabel('Input sine frequency (Hz), 48 kHz host');plt.ylabel('Folded third-harmonic suppression vs 1x (dB)');plt.title('Actual oversampling regression measurements; guard off');plt.legend(fontsize=8);plt.grid(alpha=.2);save('alias-suppression')
rows=read('harmonics.csv');plt.figure(figsize=(8,5))
for mode in range(5):
 selected=[r for r in rows if int(r['mode'])==mode and float(r['tone_hz'])==1000];plt.plot([int(r['harmonic']) for r in selected],[20*np.log10(max(1e-12,float(r['amplitude']))) for r in selected],marker='o',label=['Hard','Classic','Modern','Modern Gold','Classic Gold'][mode])
plt.xlabel('Harmonic number');plt.ylabel('Amplitude (dB relative to 1)');plt.title('1 kHz sine, amplitude 1.2 / ceiling 1, transfer functions');plt.legend();plt.ylim(-140,5);plt.grid(alpha=.2);save('harmonics')
