"""Compare corner-weighted mesh fits using a synthetic test card."""
from pathlib import Path
import json
import numpy as np
from PIL import Image, ImageDraw, ImageFont

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
BASE=np.array(json.loads((HERE/'reference-mesh.json').read_text())['vertices'])
N=len(BASE); H=2/(N-1); AREA=np.pi/4
SMOOTH=.2
BEND=30.
U,V=np.meshgrid(np.linspace(-1,1,N),np.linspace(-1,1,N))
FREE=np.hypot(U,V)>.4
FREE[[0,-1],:]=False; FREE[:,[0,-1]]=False
SLICES=[(slice(None,-1),slice(None,-1)),(slice(None,-1),slice(1,None)),(slice(1,None),slice(None,-1)),(slice(1,None),slice(1,None))]
G=[]
for a in [.5-.5/np.sqrt(3),.5+.5/np.sqrt(3)]:
 for b in [.5-.5/np.sqrt(3),.5+.5/np.sqrt(3)]:
  dx=np.array([-(1-b),1-b,-b,b])/H
  dy=np.array([-(1-a),-a,1-a,a])/H
  x=U[:-1,:-1]+a*H;y=V[:-1,:-1]+b*H
  corner=np.exp(-((1-abs(x))**2+(1-abs(y))**2)/.32**2)
  G.append((dx,dy,corner))

def energy(flat,iso,area):
 mesh=flat.reshape(N,N,2)
 points=np.stack([mesh[q] for q in SLICES]);grad=np.zeros_like(mesh);cost=0.
 for dx,dy,corner in G:
  jx=np.einsum('k,kijc->ijc',dx,points);jy=np.einsum('k,kijc->ijc',dy,points)
  a,c=jx[...,0],jx[...,1];b,d=jy[...,0],jy[...,1]
  det=a*d-b*c
  if np.any(det<=1e-8):return np.inf,np.zeros_like(flat)
  norm=a*a+b*b+c*c+d*d;log=np.log(det/AREA)
  wi=1+iso*corner;wa=1.5+area*corner
  cost+=np.mean(wa*log*log+wi*(norm/(2*det)-1))/4
  coeff=2*wa*log/det-wi*norm/(2*det*det)
  gx=wi[...,None]*jx/det[...,None]+coeff[...,None]*np.stack([d,-b],-1)
  gy=wi[...,None]*jy/det[...,None]+coeff[...,None]*np.stack([-c,a],-1)
  for k,q in enumerate(SLICES):grad[q]+=(gx*dx[k]+gy*dy[k])/(4*(N-1)**2)
 lap=(mesh[:-2,1:-1]+mesh[2:,1:-1]+mesh[1:-1,:-2]+mesh[1:-1,2:]-4*mesh[1:-1,1:-1])/(H*H)
 smooth=SMOOTH;cost+=smooth*np.mean(np.sum(lap*lap,axis=-1))
 lg=2*smooth*lap/((N-2)**2*H*H)
 grad[:-2,1:-1]+=lg;grad[2:,1:-1]+=lg;grad[1:-1,:-2]+=lg;grad[1:-1,2:]+=lg;grad[1:-1,1:-1]-=4*lg
 # Penalize reversals of grid-line curvature.
 if BEND:
  signy=np.sign(V[:,1:-1]);signx=np.sign(U[1:-1,:])
  yxx=(mesh[:,2:,1]-2*mesh[:,1:-1,1]+mesh[:,:-2,1])/(H*H)
  xyy=(mesh[2:,:,0]-2*mesh[1:-1,:,0]+mesh[:-2,:,0])/(H*H)
  by=np.maximum(signy*yxx,0);bx=np.maximum(signx*xyy,0)
  cost+=BEND*(np.mean(by*by)+np.mean(bx*bx))
  gy=2*BEND*by*signy/(by.size*H*H)
  gx=2*BEND*bx*signx/(bx.size*H*H)
  grad[:,2:,1]+=gy;grad[:,1:-1,1]-=2*gy;grad[:,:-2,1]+=gy
  grad[2:,:,0]+=gx;grad[1:-1,:,0]-=2*gx;grad[:-2,:,0]+=gx
 grad[~FREE]=0
 return cost,grad.ravel()

def optimize(iso,area):
 x=BASE.ravel().copy();f,g=energy(x,iso,area);history=[]
 for it in range(1600):
  q=g.copy();alphas=[]
  for s,y,rho in reversed(history):
   alpha=rho*np.dot(s,q);alphas.append(alpha);q-=alpha*y
  if history:
   s,y,_=history[-1];q*=np.dot(s,y)/np.dot(y,y)
  else:q*=.005
  for (s,y,rho),alpha in zip(history,reversed(alphas)):q+=s*(alpha-rho*np.dot(y,q))
  direction=-q;gd=np.dot(g,direction)
  if gd>=0:direction=-g*.001;gd=np.dot(g,direction);history=[]
  step=1.
  for _ in range(28):
   xn=x+step*direction;fn,gn=energy(xn,iso,area)
   if fn<=f+1e-4*step*gd:break
   step*=.5
  else:break
  s=xn-x;y=gn-g;sy=np.dot(s,y)
  if sy>1e-14:history.append((s,y,1/sy));history=history[-16:]
  x,f,g=xn,fn,gn
  if it%200==0:print('fit',iso,area,it,round(f,6),flush=True)
  if np.max(abs(g))<1e-6:break
 mesh=x.reshape(N,N,2)
 assert np.allclose(mesh[~FREE],BASE[~FREE])
 return mesh

def sample_mesh(mesh,uv):
 grid=np.clip((uv+1)/H,0,N-1-1e-10);ix=np.floor(grid[...,0]).astype(int);iy=np.floor(grid[...,1]).astype(int)
 a=(grid[...,0]-ix)[...,None];b=(grid[...,1]-iy)[...,None]
 return mesh[iy,ix]*(1-a)*(1-b)+mesh[iy,ix+1]*a*(1-b)+mesh[iy+1,ix]*(1-a)*b+mesh[iy+1,ix+1]*a*b

def metrics(mesh):
 result={}
 for name,px in [('top_left',(30,60)),('top_right',(690,60)),('bottom_left',(30,675)),('bottom_right',(690,675)),('center',(360,360))]:
  uv=np.array(px)/360-1;eps=1e-5
  j=np.column_stack([(sample_mesh(mesh,uv+axis*eps)-sample_mesh(mesh,uv-axis*eps))/(2*eps) for axis in np.eye(2)])
  singular=np.linalg.svd(j,compute_uv=False)
  result[name]={'stretch_ratio':float(singular[0]/singular[1]),'area':float(np.linalg.det(j)),
                'min_scale':float(singular[1]),'max_scale':float(singular[0])}
 minimum=1.
 for a,b in [(0,0),(0,1),(1,0),(1,1)]:
  p,q,r,s=(mesh[z] for z in SLICES)
  dx=(1-b)*(q-p)+b*(s-r);dy=(1-a)*(r-p)+a*(s-q)
  minimum=min(minimum,float(np.min(dx[...,0]*dy[...,1]-dx[...,1]*dy[...,0])))
 assert minimum>0,'Folded mesh'
 result['min_face_determinant']=minimum
 return result

def font(size):
 for path in ['/usr/share/fonts/TTF/DejaVuSans.ttf','/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf']:
  if Path(path).exists():return ImageFont.truetype(path,size)
 return ImageFont.load_default(size=size)

def card():
 # Supersample once from a synthetic source, never from an already warped image.
 scale=2;im=Image.new('RGB',(720*scale,720*scale),'#172333');d=ImageDraw.Draw(im)
 for x in range(0,721,60):d.line((x*scale,0,x*scale,720*scale),fill='#3f586e',width=scale)
 for y in range(0,721,60):d.line((0,y*scale,720*scale,y*scale),fill='#3f586e',width=scale)
 for x,y in [(30,60),(690,60),(360,360),(30,675),(690,675)]:
  d.ellipse(((x-10)*scale,(y-10)*scale,(x+10)*scale,(y+10)*scale),outline='#ffcb77',width=3*scale)
 d.text((18*scale,18*scale),'Menu / Dialog / Close',font=font(16*scale),fill='#9de9d5')
 d.text((18*scale,695*scale),'720 x 720 — Corner legibility',font=font(15*scale),fill='#9de9d5')
 d.text((260*scale,342*scale),'Same source',font=font(22*scale),fill='#9de9d5')
 for y in range(150,625,40):
  if 340<=y<=380:continue
  d.text((90*scale,y*scale),f'Row {y//40:02d}  AaBbCc  0123456789',font=font(16*scale),fill='#e3e9f0')
 return im

def raster(mesh,n):
 # Match the two triangles used by the compositor for each quad.
 outx=np.full((n,n),np.nan);outy=outx.copy();dest=(mesh+1)*.5*(n-1)
 for j in range(N-1):
  for i in range(N-1):
   for offsets in [[(0,0),(1,0),(1,1)],[(0,0),(1,1),(0,1)]]:
    p=np.array([dest[j+dy,i+dx] for dx,dy in offsets]);uv=np.array([[-1+H*(i+dx),-1+H*(j+dy)] for dx,dy in offsets])
    lo=np.maximum(np.floor(p.min(0)).astype(int),0);hi=np.minimum(np.ceil(p.max(0)).astype(int)+1,n)
    x,y=np.meshgrid(np.arange(lo[0],hi[0]),np.arange(lo[1],hi[1]))
    e=p[1]-p[0];f=p[2]-p[0];det=e[0]*f[1]-e[1]*f[0]
    a=((x-p[0,0])*f[1]-(y-p[0,1])*f[0])/det;b=(e[0]*(y-p[0,1])-e[1]*(x-p[0,0]))/det
    valid=(a>=-1e-8)&(b>=-1e-8)&(a+b<=1+1e-8)
    xy=uv[0]+a[...,None]*(uv[1]-uv[0])+b[...,None]*(uv[2]-uv[0])
    outx[lo[1]:hi[1],lo[0]:hi[0]][valid]=xy[...,0][valid]
    outy[lo[1]:hi[1],lo[0]:hi[0]][valid]=xy[...,1][valid]
 return outx,outy

def render(mesh,source):
 n=960;x,y=raster(mesh,n);ok=np.isfinite(x)
 src=np.asarray(source,dtype=float);height,width=src.shape[:2]
 fx=np.clip((np.nan_to_num(x)+1)*.5*(width-1),0,width-1.001);fy=np.clip((np.nan_to_num(y)+1)*.5*(height-1),0,height-1.001)
 ix=np.floor(fx).astype(int);iy=np.floor(fy).astype(int);a=(fx-ix)[...,None];b=(fy-iy)[...,None]
 pixels=src[iy,ix]*(1-a)*(1-b)+src[iy,ix+1]*a*(1-b)+src[iy+1,ix]*(1-a)*b+src[iy+1,ix+1]*a*b
 out=np.zeros((n,n,3),dtype=np.uint8)
 xx,yy=np.meshgrid(np.linspace(-1,1,n),np.linspace(-1,1,n));out[xx*xx+yy*yy<=1]=(37,40,45)
 out[ok]=pixels[ok].clip(0,255).astype(np.uint8)
 return Image.fromarray(out)

if __name__=='__main__':
 # Check the analytic gradient at free vertices before fitting.
 flat=BASE.ravel().copy();_,g=energy(flat,12,4)
 for j,i,c in [(4,5,0),(8,8,1),(15,35,0)]:
  k=(j*N+i)*2+c;plus=flat.copy();minus=flat.copy();plus[k]+=1e-6;minus[k]-=1e-6
  estimate=(energy(plus,12,4)[0]-energy(minus,12,4)[0])/2e-6
  assert abs(estimate-g[k])<1e-4,(k,estimate,g[k])
 import sys
 if '--reuse' in sys.argv:
  corrected=np.array(json.loads((HERE/'A-mesh.json').read_text())['vertices'])
 else:corrected=optimize(100,6)
 reserved=sample_mesh(BASE,np.stack([U*.84,V*.84],-1))
 variants=[('M5 reference',BASE),('A · fuller corners',corrected),('B · 8% content margin',reserved)]
 source=card();source.save(HERE/'synthetic-source.png');stats={}
 thumb=720;head=85;sheet=Image.new('RGB',(3*thumb,head+thumb),(19,22,29));draw=ImageDraw.Draw(sheet)
 zoom=Image.new('RGB',(3*420,520),(19,22,29));zd=ImageDraw.Draw(zoom)
 for i,(label,mesh) in enumerate(variants):
  key=['reference','A','B'][i];stats[key]=metrics(mesh)
  im=render(mesh,source);im.save(HERE/f'{key}.png')
  (HERE/f'{key}-mesh.json').write_text(json.dumps({'size':N,'vertices':mesh.tolist()},separators=(',',':'))+'\n')
  draw.text((i*thumb+12,10),label,font=font(25),fill='#f2d486')
  corner=stats[key]['top_left'];draw.text((i*thumb+12,46),f'Corner stretch {corner["stretch_ratio"]:.2f}:1',font=font(22),fill='#c9d3e1')
  sheet.paste(im.resize((thumb,thumb),Image.Resampling.LANCZOS),(i*thumb,head))
  center=(sample_mesh(mesh,np.array([30,60])/360-1)+1)*.5*(im.width-1)
  cx,cy=map(int,center);crop=im.crop((cx-70,cy-70,cx+70,cy+70)).resize((420,420),Image.Resampling.LANCZOS)
  zd.text((i*420+10,12),label,font=font(20),fill='#f2d486')
  zd.text((i*420+10,46),f'Area: {corner["area"]:.3f} | min scale: {corner["min_scale"]:.3f}',font=font(16),fill='#c9d3e1')
  zoom.paste(crop,(i*420,90))
 (HERE/'metrics.json').write_text(json.dumps(stats,indent=2));sheet.save(HERE/'comparison.png');zoom.save(HERE/'corner-zoom.png')
 print(json.dumps(stats,indent=2),flush=True)
