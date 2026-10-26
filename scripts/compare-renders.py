"""Optional lawful-reference comparison. Requires FFmpeg on PATH and NumPy.
Both inputs must be rendered from the same original signal. No original audio
is distributed, no model is trained, and a residual is not proof of sonic identity.
"""
import argparse,json,pathlib,subprocess
import numpy as np
p=argparse.ArgumentParser();p.add_argument('reference');p.add_argument('candidate');p.add_argument('--rate',type=int,default=48000);p.add_argument('--output',default='comparison.json');a=p.parse_args()
def load(path):
 data=subprocess.check_output(['ffmpeg','-v','error','-i',str(path),'-f','f64le','-ar',str(a.rate),'-ac','2','-'])
 return np.frombuffer(data,dtype='<f8').reshape(-1,2)
x,y=load(a.reference),load(a.candidate)
size=min(len(x),len(y),a.rate*30);xm=x[:size].mean(axis=1);ym=y[:size].mean(axis=1)
n=1<<(2*size-1).bit_length();correlation=np.fft.irfft(np.fft.rfft(ym,n)*np.conj(np.fft.rfft(xm,n)),n)
limit=min(a.rate//2,size-1);lags=np.arange(-limit,limit+1);lag=int(lags[np.argmax(correlation[lags%n])])
if lag>=0:y=y[lag:]
else:x=x[-lag:]
size=min(len(x),len(y));x=x[:size];y=y[:size];gain=float(np.sum(x*y)/max(1e-30,np.sum(y*y)));residual=x-y*gain
rms=lambda z:float(np.sqrt(np.mean(z*z)))
result={'aligned_samples':size,'candidate_delay_samples':lag,'candidate_gain_for_match':gain,'reference_rms':rms(x),'candidate_rms':rms(y),'matched_residual_rms':rms(residual),'reference_peak':float(np.max(np.abs(x))),'candidate_peak':float(np.max(np.abs(y))),'note':'Level/time alignment and numerical residual only; not a claim of perceptual identity.'}
pathlib.Path(a.output).write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
