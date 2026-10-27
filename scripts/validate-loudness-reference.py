"""Compare actual exported 3 s LUFS readings to an installed FFmpeg ebur128.
Does not redistribute FFmpeg. Tests both taps of four deterministic signals.
"""
import argparse,csv,json,pathlib,re,subprocess
p=argparse.ArgumentParser();p.add_argument('directory');p.add_argument('--ffmpeg',default='ffmpeg');p.add_argument('--output',default='loudness-reference.json');a=p.parse_args();root=pathlib.Path(a.directory);results=[]
version=subprocess.check_output([a.ffmpeg,'-version'],text=True).splitlines()[0]
for row in csv.DictReader((root/'signals.csv').open()):
 for tap in ('input','output'):
  audio=root/(row['signal']+'-'+tap+'.wav')
  run=subprocess.run([a.ffmpeg,'-hide_banner','-i',str(audio),'-af','ebur128=metadata=1,ametadata=print:key=lavfi.r128.S','-f','null','-'],capture_output=True,text=True)
  values=re.findall(r'lavfi\.r128\.S=([-+0-9.]+)',run.stderr)
  if run.returncode or not values:raise RuntimeError(run.stderr[-1000:])
  reference=float(values[-1]);ours=float(row[tap+'_lufs']);difference=ours-reference
  results.append({'signal':row['signal'],'tap':tap,'ours':ours,'reference':reference,'difference_LU':difference})
  if abs(difference)>.05:raise AssertionError(results[-1])
pathlib.Path(a.output).write_text(json.dumps({'reference':version,'method':'last complete three-second ebur128 window','tolerance_LU':.05,'results':results},indent=2)+'\n')
print(len(results),'comparisons pass; maximum difference',max(abs(r['difference_LU']) for r in results),'LU')
