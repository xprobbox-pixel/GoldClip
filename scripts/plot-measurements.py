"""Render exported, actual DSP measurements; no simulated meter or response data."""
import csv,pathlib,sys,os,tempfile
os.environ.setdefault("MPLCONFIGDIR",str(pathlib.Path(tempfile.gettempdir())/"aurum-plot-cache"))
os.environ.setdefault("XDG_CACHE_HOME",str(pathlib.Path(tempfile.gettempdir())/"aurum-font-cache"))
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
rows=read('clip-filter.csv');plt.figure(figsize=(8,5))
for rate in [44100,48000,96000]:
 for cutoff in [8000,20000]:
  selected=[r for r in rows if int(r['rate'])==rate and int(r['cutoff'])==cutoff];plt.semilogx([float(r['hz']) for r in selected],[float(r['db']) for r in selected],marker='.',label=f'{rate} Hz, cutoff {cutoff}')
plt.xlabel('Frequency (Hz)');plt.ylabel('Measured gain (dB)');plt.title('Actual oversampled Clip Filter response');plt.legend(fontsize=8);plt.grid(alpha=.2);save('clip-filter-response')
def float_wav(path):
 import struct
 data=path.read_bytes();position=12;channels=2;rate=48000
 while position+8<=len(data):
  tag=data[position:position+4];length=struct.unpack_from('<I',data,position+4)[0];position+=8
  if tag==b'fmt ':format,channels,rate=struct.unpack_from('<HHI',data,position);assert format==3
  if tag==b'data':return np.frombuffer(data[position:position+length],dtype='<f4').reshape(-1,channels),rate
  position+=length+(length%2)
 raise ValueError('WAV data chunk missing')
rows=read('signals.csv');metrics=[]
for row in rows:
 name=row['signal'];source,rate=float_wav(root/(name+'-input.wav'));processed,_=float_wav(root/(name+'-output.wav'));latency=int(row['latency_samples']);processed=processed[latency:];source=source[:len(processed)]
 rms=lambda x:float(np.sqrt(np.mean(x.astype(float)**2)))
 metrics.append({'signal':name,'latency_samples':latency,'input_crest_db':20*np.log10(max(1e-12,np.max(np.abs(source)))/max(1e-12,rms(source))),'output_crest_db':20*np.log10(max(1e-12,np.max(np.abs(processed)))/max(1e-12,rms(processed)))})
 if name=='drums':
  n=int(rate*.18);plt.figure(figsize=(9,4));t=np.arange(n)/rate*1000;plt.plot(t,source[:n,0],label='Original',color='#aaa');plt.plot(t,processed[:n,0],label='Processed, compensated alignment',color='#f0ca72');plt.xlabel('Time (ms)');plt.ylabel('Amplitude');plt.title('Deterministic drum transient through actual Master chain');plt.legend();plt.grid(alpha=.2);save('drum-transient')
import json
(root/'transient-metrics.json').write_text(json.dumps(metrics,indent=2,default=float)+'\n')
