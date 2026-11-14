"""Independent accessory geometry; Blender 4.3 CPU Cycles. Reference reconstruction."""
import bpy, math, json
from pathlib import Path
from mathutils import Vector
OUT=Path(__file__).resolve().parents[2]/'assets/gui/rendered-v1';OUT.mkdir(parents=True,exist_ok=True)
s=bpy.context.scene
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
s.render.engine='CYCLES';s.cycles.device='CPU';s.cycles.samples=48;s.cycles.use_denoising=False
s.render.threads_mode='FIXED';s.render.threads=1
s.render.film_transparent=True;s.render.image_settings.file_format='PNG';s.render.image_settings.color_mode='RGBA';s.world.color=(.18,.18,.18)
s.view_settings.view_transform='AgX'
def mat(n,c,metal=.0,rough=.25,emit=0):
 m=bpy.data.materials.new(n);m.diffuse_color=(*c,1);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*c,1);p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough
 if emit:p.inputs['Emission Color'].default_value=(*c,1);p.inputs['Emission Strength'].default_value=emit
 return m
steel=mat('brushed nickel',(.43,.43,.41),.95);black=mat('black anodised',(.015,.018,.019),.7);brass=mat('brass',(.4,.21,.06),.9);ivory=mat('warm meter parchment',(.72,.5,.22),0,.5);amber=mat('amber illumination',(1,.25,.003),0,.3,4)
def cube(n,p,d,m,b=.04):
 bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.name=n;o.dimensions=d;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(m)
 if b:q=o.modifiers.new('machined bevel','BEVEL');q.width=b;q.segments=3;o.modifiers.new('smooth normals','WEIGHTED_NORMAL')
 return o
def cyl(n,p,r,d,m):
 bpy.ops.mesh.primitive_cylinder_add(vertices=48,radius=r,depth=d,location=p);o=bpy.context.object;o.name=n;o.data.materials.append(m);q=o.modifiers.new('edge bevel','BEVEL');q.width=.012;q.segments=2
 for f in o.data.polygons:f.use_smooth=True
 return o
def rod(n,a,b,r,m):
 v=Vector(b)-Vector(a);o=cyl(n,(Vector(a)+Vector(b))/2,r,v.length,m);o.rotation_euler=v.to_track_quat('Z','Y').to_euler();return o
def reset(w,h,px,py):
 for o in list(s.objects):
  if o.type!='LIGHT':bpy.data.objects.remove(o,do_unlink=True)
 bpy.ops.object.camera_add(location=(0,0,30));c=bpy.context.object;c.data.type='ORTHO';c.data.ortho_scale=max(w,h);s.camera=c;s.render.resolution_x=px;s.render.resolution_y=py;s.render.resolution_percentage=100
for rot,e in [((.4,-.5,-.3),3),((-.6,.4,.5),1.1)]:
 bpy.ops.object.light_add(type='SUN',rotation=rot);bpy.context.object.data.energy=e;bpy.context.object.data.angle=.2
# Physically separate Cycles passes, exported independently of beauty PNGs.
vl=s.view_layers[0];vl.use_pass_diffuse_color=True;vl.use_pass_diffuse_direct=True;vl.use_pass_glossy_direct=True;vl.use_pass_glossy_indirect=True;vl.use_pass_emit=True;vl.use_pass_ambient_occlusion=True;vl.cycles.use_pass_shadow_catcher=True
s.use_nodes=True;nodes=s.node_tree.nodes;nodes.clear();rl=nodes.new('CompositorNodeRLayers');comp=nodes.new('CompositorNodeComposite');s.node_tree.links.new(rl.outputs['Image'],comp.inputs['Image'])
pass_nodes=[]
for socket,label in [('DiffCol','material_albedo'),('DiffDir','direct_lighting'),('GlossDir','reflection_direct'),('GlossInd','reflection_indirect'),('Emit','emission'),('AO','ambient_occlusion'),('Shadow Catcher','shadow_catcher')]:
 if socket in rl.outputs:
  node=nodes.new('CompositorNodeOutputFile');node.format.file_format='PNG';node.format.color_mode='RGBA';node.format.color_depth='16';s.node_tree.links.new(rl.outputs[socket],node.inputs[0]);pass_nodes.append((node,label))
files=[]
def render(n):
 # Genuine invisible Cycles receiver captures geometry-cast shadows independently.
 bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.48));receiver=bpy.context.object;receiver.name='independent shadow receiver';receiver.is_shadow_catcher=True
 for node,label in pass_nodes:
  node.base_path=str(OUT/'lighting'/n);node.file_slots[0].path=label+'_'
 s.render.filepath=str(OUT/(n+'.png'));bpy.ops.render.render(write_still=True);files.append(n+'.png');bpy.data.objects.remove(receiver,do_unlink=True)
for color,c in [('amber',(1,.3,.005)),('red',(1,.015,.005)),('green',(.1,1,.01)),('blue',(.01,.2,1)),('white',(1,.9,.7))]:
 reset(1,1,96,96);cyl('LED steel socket',(0,0,0),.31,.1,steel);cyl('black gasket',(0,0,.07),.25,.08,black);cyl('glass lens',(0,0,.13),.19,.1,mat(color,c,0,.12,3));render('control_led_'+color)
for state,x in [('left',-.23),('centre',0),('right',.23)]:
 reset(1.5,1.1,144,106);cyl('switch retaining nut',(0,0,0),.32,.13,steel);cyl('switch socket',(0,0,.09),.22,.08,black);rod('lever',(0,0,.13),(x,.15,.6),.075,steel);cyl('lever tip',(x,.15,.6),.09,.12,black);render('control_toggle_'+state)
for state in ['off','on']:
 reset(1.8,.8,180,80);cube('button chrome rim',(0,0,0),(1.65,.65,.13),steel);cube('button black inset',(0,0,.09),(1.53,.53,.12),black);cube('button cap',(0,0,.17),(1.4,.42,.1),amber if state=='on' else black);render('control_button_'+state)
for state in ['a','b']:
 reset(2.3,.8,230,80)
 for i in range(2):
  x=(i-.5)*1.05;cube('selector metal rim',(x,0,0),(1,.65,.1),steel);cube('selector cap',(x,0,.09),(.9,.53,.1),amber if i==(0 if state=='a' else 1) else black)
 render('control_selector_'+state)
# Microphone is physically modelled in the image plane, with a separate rear chamber.
reset(2.1,2.8,252,336)
cube('chamber bright rim',(0,0,-.23),(2.02,2.7,.2),steel,.17);cube('deep black chamber',(0,0,-.1),(1.9,2.58,.18),black,.14)
for x in [-.72,.72]:cube('warm side reflection',(x,0,.02),(.035,1.95,.01),brass,.01)
render('mic_chamber_background')
for o in list(s.objects):
 if o.type=='MESH':bpy.data.objects.remove(o,do_unlink=True)
def lathe(n,profile,m,cx=0,cz=0):
 verts=[];faces=[];N=96
 for y,r in profile:
  for k in range(N):
   a=k*math.tau/N;verts.append((cx+r*math.sin(a),y,cz+r*math.cos(a)))
 for j in range(len(profile)-1):
  for k in range(N):a=j*N+k;b=j*N+(k+1)%N;faces.append((a,b,b+N,a+N))
 mesh=bpy.data.meshes.new(n);mesh.from_pydata(verts,[],faces);mesh.update();o=bpy.data.objects.new(n,mesh);s.collection.objects.link(o);o.data.materials.append(m)
 for f in o.data.polygons:f.use_smooth=True
 return o
lathe('rounded brass capsule',[(.08,0),(.09,.22),(.12,.275),(.87,.275),(.97,.25),(1.02,.16),(1.04,0)],brass,cz=.1)
# Grille follows a rounded cylinder, wrapping across its visible front hemisphere.
for i in range(23):
 a=-math.pi/2+i*math.pi/22;x=.28*math.sin(a);z=.1+.28*math.cos(a)
 rod('woven vertical mesh',(x,.14,z),(x,.91,z),.0045,steel)
for i in range(30):
 y=.15+i*.0265
 for k in range(24):
  a=-math.pi/2+k*math.pi/24;b=a+math.pi/24
  rod('woven curved horizontal mesh',(.282*math.sin(a),y,.1+.282*math.cos(a)),(.282*math.sin(b),y,.1+.282*math.cos(b)),.004,brass)
lathe('cylindrical microphone barrel',[(-.91,0),(-.88,.24),(-.82,.275),(.05,.275),(.09,.23),(.09,0)],black,cz=.1)
lathe('polished capsule collar',[(.01,.28),(.09,.28)],steel,cz=.1)
for y in [-.3,-.73]:cube('shockmount crossbar',(0,y,.42),(1.08,.06,.06),black,.025)
for side in [-1,1]:
 rod('shock mount',(.48*side,.02,.35),(.36*side,-.9,.35),.027,steel);rod('elastic suspension',(.48*side,.02,.39),(-.36*side,-.9,.39),.012,black)
rod('stand',(0,-.85,.1),(0,-1.35,.1),.07,steel);render('mic_capsule_geometry')
# Tall vacuum tube and cage, independent from rack.
reset(1.2,4.0,144,480)
glass=mat('slightly smoky glass',(.15,.07,.018),0,.12)
p=glass.node_tree.nodes.get('Principled BSDF');p.inputs['Transmission Weight'].default_value=.93;p.inputs['IOR'].default_value=1.46
# Thin hollow lathed glass envelope: visible circular shoulders and bulb.
profile=[(-1.72,.12),(-1.67,.27),(-1.53,.35),(-1.3,.37),(.93,.37),(1.18,.33),(1.35,.23),(1.41,.07),(1.43,.025)]
o=lathe('vacuum glass envelope',profile,glass,cz=0);solid=o.modifiers.new('real glass wall','SOLIDIFY');solid.thickness=.012
lathe('lower ceramic socket',[(-1.83,0),(-1.82,.38),(-1.61,.38),(-1.59,0)],black)
# Black anode plates reveal a central hot cathode through the slit.
for x in [-.14,.14]:cube('anode plate',(x,-.38,-.03),(.13,1.4,.25),black,.03)
for x in [-.22,.22]:rod('electrode support',(x,-1.4,0),(x,1.05,0),.012,steel)
for i in range(100):
 a=i*math.tau/12;y=-1.13+i*.014
 b=a+math.tau/12
 rod('hot coiled cathode',(.075*math.cos(a),y,.075*math.sin(a)+.15),(.075*math.cos(b),y+.014,.075*math.sin(b)+.15),.008,amber)
for y in [-1.48,.78,1.16]:lathe('electrode mica disc',[(y-.015,.26),(y+.015,.26)],steel)
# Curved black metal cage wraps rounded tube, warm interior visible between ribs.
for k in range(9):
 a=-math.pi/2+k*math.pi/8;x=.46*math.sin(a);z=.46*math.cos(a)
 pts=[(x*.65,-1.74,z*.65),(x,-1.5,z),(x,1.14,z),(x*.72,1.48,z*.72)]
 for a1,b1 in zip(pts,pts[1:]):rod('curved cage upright',a1,b1,.018,black)
for y in [-1.56,-.12,1.19]:lathe('cage retaining hoop',[(y-.035,.47),(y+.035,.47)],black)
for y in [-1.78,1.52]:lathe('chrome end cap',[(y-.055,.0),(y-.05,.39),(y+.05,.39),(y+.055,0)],steel)
render('tube_assembly')
reset(2.2,1.1,440,220);cube('VU metal frame',(0,0,0),(2.15,1.05,.12),steel);cube('VU bezel',(0,0,.1),(2.03,.93,.1),black);cube('VU illuminated face',(0,0,.17),(1.86,.76,.03),ivory,.06);render('vu_face_geometry')
reset(2.2,1.1,440,220);rod('VU independent needle',(0,-.38,.25),(.18,.32,.25),.009,black);cyl('VU needle pivot',(0,-.38,.26),.035,.02,brass);render('vu_needle')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'controls_source.blend'))
(OUT/'controls_geometry_manifest.json').write_text(json.dumps({'renderer':'Blender 4.3 Cycles CPU 48 samples','files':files,'reference':'concept_original.png','status':'reconstruction; requires visual acceptance','needlePivotNormalized':[.5,.845]},indent=2))
print('CONTROLS_COMPLETE')
