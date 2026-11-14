"""Blender 4.3 CPU/Cycles reconstruction from concept_original.png.
Run: blender -b --factory-startup -t 3 --python tools/blender/render_assets.py
No source reference pixels are rotated. Lighting is fixed in world space.
"""
import bpy, math, json, sys
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'assets/gui/rendered-v1';OUT.mkdir(parents=True,exist_ok=True)
scene=bpy.context.scene
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=64;scene.cycles.use_denoising=False
scene.render.image_settings.file_format='PNG';scene.render.image_settings.color_mode='RGBA';scene.render.film_transparent=True
scene.render.resolution_percentage=100
scene.view_settings.view_transform='AgX'
scene.world.color=(0.25,0.25,0.25)
scene.render.threads_mode='FIXED';scene.render.threads=3

def material(name,color,metal=0.0,rough=.3,brush=False):
 m=bpy.data.materials.new(name);m.use_nodes=True
 n=m.node_tree.nodes;p=n.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough
 if metal>.5:
  p.inputs['Anisotropic'].default_value=.75
  tangent=n.new('ShaderNodeTangent');tangent.direction_type='RADIAL';tangent.axis='Z';m.node_tree.links.new(tangent.outputs[0],p.inputs['Tangent'])
 if brush:
  tex=n.new('ShaderNodeTexNoise');tex.inputs['Scale'].default_value=160;tex.inputs['Detail'].default_value=2
  coord=n.new('ShaderNodeTexCoord');mapping=n.new('ShaderNodeVectorMath');mapping.operation='MULTIPLY';mapping.inputs[1].default_value=(1,65,1)
  bump=n.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.15;bump.inputs['Distance'].default_value=.018
  m.node_tree.links.new(coord.outputs['Generated'],mapping.inputs[0]);m.node_tree.links.new(mapping.outputs[0],tex.inputs['Vector']);m.node_tree.links.new(tex.outputs['Fac'],bump.inputs['Height']);m.node_tree.links.new(bump.outputs['Normal'],p.inputs['Normal'])
 return m
steel=material('brushed nickel',(0.42,.43,.44),.92,.25,True)
warm=material('warm steel',(.43,.36,.25),.9,.28,True)
black=material('black anodised',(.018,.022,.024),.85,.25,True)
brass=material('brass',(.5,.32,.12),.85,.24,True)
dark=material('brushed charcoal',(.035,.04,.045),.8,.32,True)
wood=material('walnut rails',(.15,.045,.013),.15,.38,True)
rubber=material('inset rubber',(.009,.009,.009),.05,.65)
white=material('engraved ivory',(.78,.75,.65),.35,.4)

def cube(name,loc,scale,mat,bevel=.04):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.name=name;o.dimensions=scale;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if bevel:
  mod=o.modifiers.new('machined edge','BEVEL');mod.width=bevel;mod.segments=3;o.modifiers.new('weighted normals','WEIGHTED_NORMAL')
 o.data.materials.append(mat);return o

def cyl(name,loc,radius,depth,mat,vertices=64):
 bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=radius,depth=depth,location=loc);o=bpy.context.object;o.name=name;o.data.materials.append(mat)
 bevel=o.modifiers.new('edge rounding','BEVEL');bevel.width=.018;bevel.segments=3;o.modifiers.new('weighted normals','WEIGHTED_NORMAL')
 for p in o.data.polygons:p.use_smooth=True
 return o

def camera(w,h):
 bpy.ops.object.camera_add(location=(0,0,40));c=bpy.context.object;c.data.type='ORTHO';c.data.ortho_scale=w;scene.camera=c
 scene.render.resolution_x=int(w*100);scene.render.resolution_y=int(h*100)

def lamp(name,rot,power):
 bpy.ops.object.light_add(type='SUN',rotation=rot);o=bpy.context.object;o.name=name;o.data.energy=power;o.data.angle=.25
lamp('warm studio key',(.45,-.55,-.4),1.5);lamp('cool fill',(-.7,.2,.6),.5)
bpy.ops.object.light_add(type='AREA',location=(-3,4,7));soft=bpy.context.object;soft.rotation_euler=(Vector((0,0,0))-soft.location).to_track_quat('-Z','Y').to_euler();soft.data.energy=1500;soft.data.shape='RECTANGLE';soft.data.size=2;soft.data.size_y=8
# Optional pass export does not rerender moving-index atlases.
LAYER_EXPORT='--layers' in sys.argv
pass_nodes=[]
if LAYER_EXPORT:
 vl=scene.view_layers[0]
 vl.use_pass_diffuse_color=True;vl.use_pass_diffuse_direct=True;vl.use_pass_diffuse_indirect=True
 vl.use_pass_glossy_color=True;vl.use_pass_glossy_direct=True;vl.use_pass_glossy_indirect=True
 vl.use_pass_emit=True;vl.cycles.use_pass_shadow_catcher=True
 scene.use_nodes=True;nodes=scene.node_tree.nodes;nodes.clear()
 rl=nodes.new('CompositorNodeRLayers');comp=nodes.new('CompositorNodeComposite');scene.node_tree.links.new(rl.outputs['Image'],comp.inputs['Image'])
 for socket,label in [('DiffCol','material_albedo'),('DiffDir','direct_lighting'),('DiffInd','indirect_lighting'),('GlossCol','reflection_color'),('GlossDir','reflection_direct'),('GlossInd','reflection_indirect'),('Emit','emission'),('Shadow Catcher','shadow_catcher')]:
  node=nodes.new('CompositorNodeOutputFile');node.format.file_format='PNG';node.format.color_mode='RGBA';node.format.color_depth='16'
  scene.node_tree.links.new(rl.outputs[socket],node.inputs[0]);pass_nodes.append((node,label))
def render_current():
 receiver=None
 if LAYER_EXPORT:
  stem=Path(scene.render.filepath).stem
  if '--reuse-existing' in sys.argv and all((OUT/'lighting'/stem/(label+'_0001.png')).exists() for _,label in pass_nodes):return
  for node,label in pass_nodes:
   node.base_path=str(OUT/'lighting'/stem);node.file_slots[0].path=label+'_'
  bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.5));receiver=bpy.context.object;receiver.is_shadow_catcher=True
 bpy.ops.render.render(write_still=True)
 if receiver:bpy.data.objects.remove(receiver,do_unlink=True)
manifest={'version':'rendered-v1','renderer':f'Blender 4.3.2 / Cycles CPU / {scene.cycles.samples} samples / AgX','reference':'../production/reference/concept_original.png','status':'3D reconstruction; visual acceptance pending','knobs':{}}

# Orthographic knob body, light and pointer are independent reusable layers.
# Each body is rotationally symmetric; only its index moves. Thus no highlight rotation.
for name,mat,radius,ribs in [('steel_large',steel,.38,0),('steel_warm',warm,.38,0),('black_anodized',black,.38,32),('steel_bright',steel,.36,0),('chrome_small',steel,.32,40),('black_small',black,.32,36),('brass_small',brass,.32,32)]:
 for o in list(scene.objects):
  if o.type not in {'LIGHT'}:bpy.data.objects.remove(o,do_unlink=True)
 camera(1.0,1.0)
 cyl('fixed rear collar',(0,0,0),radius+.045,.07,rubber)
 cyl('fixed metal bezel',(0,0,.05),radius+.02,.07,brass if name=='steel_warm' else black)
 cyl('machined knob body',(0,0,.12),radius,.17,mat)
 if ribs:
  for i in range(ribs):
   a=i*2*math.pi/ribs
   o=cube('knurl',((radius-.008)*math.cos(a),(radius-.008)*math.sin(a),.13),(.025,.025,.14),mat,.006);o.rotation_euler.z=a
 bpy.ops.mesh.primitive_cone_add(vertices=128,radius1=radius*.91,radius2=.001,depth=.045,location=(0,0,.245));bpy.context.object.name='radially machined shallow cone';bpy.context.object.data.materials.append(mat)
 scene.render.resolution_x=192;scene.render.resolution_y=192
 scene.render.filepath=str(OUT/f'{name}_body.png');render_current()
 if LAYER_EXPORT:continue
 # Retain body as shadow receiver while exporting actual moving index geometry separately.
 for o in scene.objects:
  if o.type=='MESH':o.hide_render=True
 pointer=cube('rotating engraved index',(0,radius*.65,.28),(.018,radius*.38,.008),white,.003)
 # Render pointer atlas in a shared scene: directional light is identical at every cell.
 bpy.data.objects.remove(scene.camera,do_unlink=True)
 camera(16,8);scene.render.resolution_x=1536;scene.render.resolution_y=768
 mesh=pointer.data
 bpy.data.objects.remove(pointer,do_unlink=True)
 for frame in range(128):
  angle=math.radians(135-270*frame/127)
  x=(frame%16)-7.5;y=3.5-(frame//16)
  o=bpy.data.objects.new(f'index_{frame:03}',mesh);scene.collection.objects.link(o)
  o.location=(x-math.sin(angle)*radius*.65,y+math.cos(angle)*radius*.65,.28);o.rotation_euler.z=angle

 scene.render.filepath=str(OUT/f'{name}_index_128.png');render_current()
 manifest['knobs'][name]={'body':f'{name}_body.png','index':f'{name}_index_128.png','frameWidth':96,'frameHeight':96,'columns':16,'rows':8,'frames':128,'lighting':'world-fixed; moving index only'}

# Rack and separately reusable panel geometry, sized to the top rack in the reference.
for o in list(scene.objects):
 if o.type!='LIGHT':bpy.data.objects.remove(o,do_unlink=True)
camera(16.72,6.0)
def pos(x,y,z):return ((x-836)/100,(300-y)/100,z)
def panel(name,x,y,w,h):
 objects=[]
 objects.append(cube(name+' frame',pos(x+w/2,y+h/2,.05),(w/100,h/100,.10),steel,.055))
 objects.append(cube(name+' inset',pos(x+w/2,y+h/2,.12),((w-8)/100,(h-8)/100,.07),dark,.04))
 for sx in [x+13,x+w-13]:
  for sy in [y+13,y+h-13]:
   objects.append(cyl(name+' screw well',pos(sx,sy,.18),.075,.025,rubber,32))
   objects.append(cyl(name+' screw',pos(sx,sy,.2),.05,.035,steel,32))
   objects.append(cube(name+' screw slot',pos(sx,sy,.222),(.055,.012,.005),rubber,.002))
 return objects
base=[cube('rack chassis',(0,0,-.15),(16.68,5.95,.2),black,.05)]
for x in [-8.15,8.15]:base.append(cube('wood side cheek',(x,0,.0),(.29,5.9,.35),wood,.035))
for x in [-8.16,8.16]:
 for y in [-2.77,2.77]:base.append(cyl('mount bolt',(x,y,.25),.09,.07,steel))
scene.render.filepath=str(OUT/'rack_base.png');render_current()
for o in base:o.hide_render=True
panels=[('header',43,3,1584,72),('tube',43,81,113,353),('fuzz',162,81,263,353),('virtual_mic',436,81,391,353),('room_ir',839,81,300,353),('feedback',1149,81,265,353),('filter1',1425,81,200,158),('filter2',1425,249,200,185),('modulation',47,446,474,139),('analyzer',529,446,525,139),('output',1065,446,463,139),('scope',1536,446,90,139)]
manifest['panels']=[]
for name,x,y,w,h in panels:
 objs=panel(name,x,y,w,h)
 if LAYER_EXPORT:
  scene.render.use_border=True;scene.render.use_crop_to_border=False
  scene.render.border_min_x=max(0,(x-20)/1672);scene.render.border_max_x=min(1,(x+w+20)/1672)
  scene.render.border_min_y=max(0,1-(y+h+20)/600);scene.render.border_max_y=min(1,1-(y-20)/600)
 scene.render.filepath=str(OUT/f'panel_{name}.png');render_current()
 for o in objs:o.hide_render=True
 manifest['panels'].append({'file':f'panel_{name}.png','bounds':[x,y,w,h]})
# Source geometry saved alongside outputs; rerun script to reproduce every layer.
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'rack_source.blend'))
if not LAYER_EXPORT:(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('MICROPHONIC_RENDER_COMPLETE')
