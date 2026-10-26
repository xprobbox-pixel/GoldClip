"""Record checksums and native header inspection, without executing release binaries."""
import hashlib,json,pathlib,subprocess,sys,struct
root=pathlib.Path(sys.argv[1]).resolve()
identity=(pathlib.Path(__file__).resolve().parents[1]/'cmake/Identity.cmake').read_text()
version=identity.split('set(AURUM_VERSION ')[1].split(')')[0]
def run(args):
    p=subprocess.run(args,text=True,capture_output=True)
    return {'exit':p.returncode,'stdout':p.stdout.strip(),'stderr':p.stderr.strip()}
files=[]
for p in sorted(root.rglob('*')):
    if not p.is_file() or p.name=='manifest.json':continue
    data=p.read_bytes();record={'path':p.relative_to(root).as_posix(),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
    if data[:2]==b'MZ':
        offset=struct.unpack_from('<I',data,60)[0]
        record['machine']=hex(struct.unpack_from('<H',data,offset+4)[0])
    if sys.platform=='darwin' and '/Contents/MacOS/' in str(p):
        record['architectures']=run(['lipo','-archs',str(p)])
        record['signature']=run(['codesign','-dv',str(p)])
    files.append(record)
root.mkdir(parents=True,exist_ok=True)
(root/'manifest.json').write_text(json.dumps({'product':'AURUM CLIP','phase':'2A','version':version,'source_commit':run(['git','rev-parse','HEAD'])['stdout'],'release_signing':'not performed','notarisation':'not performed','host':sys.platform,'files':files},indent=2)+'\n')
