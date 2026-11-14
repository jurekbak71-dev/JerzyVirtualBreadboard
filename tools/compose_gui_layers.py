"""Crisp vector-derived typography/scales, kept separate from rendered geometry."""
from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
import math,json
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'assets/gui/rendered-v1';OUT.mkdir(parents=True,exist_ok=True)
S=3;im=Image.new('RGBA',(1672*S,600*S));d=ImageDraw.Draw(im)
FONT='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf';BOLD='/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'
def text(x,y,t,size=13,fill='#e9e6df',bold=False,anchor='mm'):
 d.text((x*S,y*S),t,font=ImageFont.truetype(BOLD if bold else FONT,round(size*S)),fill=fill,anchor=anchor,stroke_width=0)
def line(points,fill='#ddd8cc',width=1):d.line([(x*S,y*S) for x,y in points],fill=fill,width=round(width*S))
logoMask=Image.new('L',im.size);ImageDraw.Draw(logoMask).text((287*S,39*S),'MICROPHONIC',font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-BoldOblique.ttf',34*S),fill=255,anchor='mm')
metal=Image.new('RGBA',im.size);md=ImageDraw.Draw(metal)
for yy in range(17*S,61*S):
 value=int(185+50*math.cos((yy/S-25)*.12)+(yy%5-2)*3);value=max(90,min(245,value));md.line((140*S,yy,435*S,yy),fill=(value,value,max(0,value-4),255))
im.paste(metal,(0,0),logoMask)
text(528,29,'VIRTUAL MICROPHONE',12);text(528,45,'FEEDBACK PROCESSOR',12)

for x,y,t in [(322,106,'FUZZ'),(629,106,'VIRTUAL MIC'),(1006,106,'ROOM IR'),(1293,106,'FEEDBACK'),(1516,109,'FILTER 1'),(1516,277,'FILTER 2'),(183,470,'MODULATION')]:text(x,y,t,20 if y!=470 else 17,bold=True)
knobs=[('gain',239,202,44,'GAIN','0','10'),('bias',364,202,36,'BIAS','0','10'),('tone',239,323,32,'TONE','0','10'),('gate',364,323,32,'GATE','0','10'),('distance',493,204,28,'DISTANCE','0.1 m','4.0 m'),('position',493,320,28,'POSITION','L','R'),('movement',777,204,30,'MOVEMENT','0','100'),('sensitivity',777,320,30,'SENSITIVITY','0','10'),('irMix',899,356,26,'IR MIX','0','100%'),('early',995,356,26,'EARLY','0','100%'),('decay',1094,356,26,'DECAY','0.1 s','10 s'),('feedback',1285,225,91,'','0','100'),('resonance',1227,375,35,'RESONANCE','0','10'),('instability',1354,375,35,'INSTABILITY','0','10'),('filter1Freq',1486,177,32,'FREQ','20 Hz','20 kHz'),('filter1Res',1579,177,26,'RES','0','10'),('filter2Freq',1486,354,32,'FREQ','20 Hz','20 kHz'),('filter2Res',1579,354,26,'RES','0','10'),('tremoloRate',185,536,22,'','', ''),('tremoloDepth',243,536,22,'','',''),('flangerRate',384,536,22,'','',''),('flangerDepth',435,536,22,'','',''),('flangerFeed',485,536,22,'','',''),('mix',1129,516,36,'MIX','0','100%'),('output',1256,516,36,'OUTPUT','−∞','+6')]
layout={}
for name,x,y,r,label,lo,hi in knobs:
 if label:text(x,y-r-17,label,12)
 for i in range(27):
  a=math.radians(135+270*i/26);r1=r+3;r2=r+(9 if i%2==0 else 6)
  line([(x+math.cos(a)*r1,y+math.sin(a)*r1),(x+math.cos(a)*r2,y+math.sin(a)*r2)],width=1.3)
 if lo:text(x-r*.73,y+r+12,lo,10)
 if hi:text(x+r*.85,y+r+12,hi,10)
 layout[name]={'center':[x,y],'radius':r,'bounds':[x-r,y-r,2*r,2*r]}
text(203,392,'TYPE',12);
text(499,398,'MODE',13);

text(178,499,'TREMOLO',12);text(391,499,'FLANGER',12)
for x,t in [(185,'RATE'),(243,'DEPTH'),(384,'RATE'),(435,'DEPTH'),(485,'FEED')]:text(x,564,t,10)
text(1390,483,'LIMITER',12);text(1256,561,'0 dB',10)
for y,t in [(467,'0'),(483,'−12'),(501,'−24'),(518,'−36'),(536,'−48'),(554,'−60')]:text(562,y,t,10,anchor='rm')
for x,t in [(575,'20'),(638,'50'),(684,'100'),(730,'200'),(789,'500'),(836,'1k'),(885,'2k'),(942,'5k'),(988,'10k'),(1033,'20k')]:text(x,565,t,10)
for y,t in [(467,'0'),(481,'−6'),(495,'−12'),(509,'−18'),(523,'−24'),(537,'−36'),(550,'−48'),(563,'−60')]:text(1501,y,t,9)
text(1450,571,'L',9);text(1470,571,'R',9)
im.resize((1672,600),Image.Resampling.LANCZOS).save(OUT/'labels_overlay.png')
# Actual scale and lettering for the independent VU face, with no baked needle.
vu=Image.open(OUT/'vu_face_geometry.png').convert('RGBA') if (OUT/'vu_face_geometry.png').exists() else Image.new('RGBA',(440,220))
v=ImageDraw.Draw(vu);f=ImageFont.truetype(FONT,13)
for i,t in enumerate(['−20','−10','−7','−5','−3','−1','0','+1','+3']):
 a=math.radians(205+i*15);x=220+165*math.cos(a);y=188+135*math.sin(a);v.line((x,y,x+8*math.cos(a),y+8*math.sin(a)),fill='#302015',width=2);v.text((x-10,y-17),t,font=f,fill='#b51d0c' if i>6 else '#302015')
v.text((205,167),'VU',font=ImageFont.truetype(BOLD,22),fill='#302015');vu.save(OUT/'vu_face.png')
manifest={'referenceSize':[1672,600],'reference':'assets/gui/production/reference/concept_original.png','knobs':layout,'layers':{'labels':'labels_overlay.png','micBackground':{'file':'mic_chamber_background.png','bounds':[542,129,185,237]},'micGeometry':{'file':'mic_capsule_geometry.png','bounds':[542,129,185,237]},'tube':{'file':'tube_assembly.png','bounds':[50,12,101,379]}},'controls':{'fuzzTypes':{'bounds':[231,378,173,24],'states':['Si','Ge','LED','BJT']},'mode':{'bounds':[537,383,251,31],'states':['SUSTAIN','SCREAM','CHAOS']},'roomAB':{'bounds':[922,128,138,40],'states':['A','B']},'vuNeedle':{'file':'vu_needle.png','pivotNormalized':[.5,.845],'motion':'rotate needle only; fixed face and lighting'}},'status':'Reference-based reconstruction; visual approval pending','separation':'Geometry and labels separate; chamber and capsule separate; meter face and needle separate. Beauty renders include lighting; separate 16-bit Cycles material_albedo, direct_lighting, reflection_direct, reflection_indirect, emission, ambient_occlusion passes are in lighting/<asset>/. A separate genuine Cycles shadow_catcher pass is exported using an invisible receiver, alongside ambient occlusion.'}
(OUT/'controls_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('OVERLAY_COMPLETE')
