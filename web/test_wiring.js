// Behavioral regression: explicit physical-terminal connections must not merge whole nets.
const fs=require("fs"),vm=require("vm"),assert=require("assert"),path=require("path");
const html=fs.readFileSync(path.join(__dirname,"editor.html"),"utf8");
const js=html.split("<script>")[1].split("</script>")[0];
const controls=new Map();
function element(tag="div"){
 return {
  tag,attrs:{},children:[],value:"",textContent:"",innerHTML:"",dataset:{},style:{},onclick:null,
  setAttribute(k,v){this.attrs[k]=v},
  appendChild(x){this.children.push(x);return x},
  append(...xs){this.children.push(...xs)},
  addEventListener(event,handler){(this.events??={})[event]=handler},
  setPointerCapture(){},
  getBoundingClientRect(){return {width:900,height:520}}
 };
}
const tools=["R","POT","SW","C","L","VDC","NPN","PNP","NMOS","NJFET","OPAMP","D","SCH","GE","Z","LED"].map(kind=>Object.assign(element("button"),{dataset:{add:kind}}));
const document={
 body:{dataset:{}},
 querySelector(key){if(!controls.has(key))controls.set(key,element());return controls.get(key)},
 querySelectorAll(key){if(key==="[data-add]")return tools;return []},
 createElementNS(ns,tag){return element(tag)},
 createElement(tag){return element(tag)}
};
const context=vm.createContext({window:{},document,console,Math,Map,Set,Number,Object,String,Error,URL:{createObjectURL(){return "blob:"},revokeObjectURL(){}},Blob:function(){},setTimeout(){},setInterval(){}});
vm.runInContext(js,context,{filename:"editor.html"});
const run=expr=>vm.runInContext(expr,context);
assert.equal(run("parts.length"),4);
assert.equal(run("wires.length"),8);
assert.equal(run("validate()"),true);
assert.match(run("netlist()"),/D1 out 0 DGEN/);
const original=run("wires.length");
run('removeWire(wires.findIndex(w=>w.includes("IN")&&w.includes("R1.a")))');
assert.equal(run("wires.length"),original-1);
assert.throws(()=>run("validate()"),/Brak połączenia|Występują niepołączone/);
run('connect("IN");connect("R1.a")');
assert.equal(run("wires.length"),original);
assert.equal(run("validate()"),true);
const before=run("wires.length");
tools[0].onclick();
assert.equal(run("parts.length"),5);
assert.equal(run("wires.length"),before,"New resistor must remain disconnected");
assert.throws(()=>run("validate()"),/Występują niepołączone/);
run('connect("IN");connect("R2.a")');
assert.equal(run("wires.length"),before+1);
assert.notEqual(run('parts.find(p=>p.id==="R2").b'),"0","New resistor terminal must NOT auto-connect to ground");
assert.notEqual(run('parts.find(p=>p.id==="R2").b'),"out","New resistor terminal must NOT auto-connect to output");
run('connect("R2.b");connect("OUT")');
assert.equal(run("validate()"),true);
assert.match(run("netlist()"),/R2 in out/);
const wiresBefore=run("wires.length");
run('removeWire(wires.findIndex(w=>w.includes("R2.b")))');
assert.equal(run("wires.length"),wiresBefore-1);
assert.throws(()=>run("validate()"),/Występują niepołączone/);
console.log("PASS: independent wires, unconnected components, add/remove, connectivity, SPICE export");

// Browser-like regression: pointerup fires before click and must not destroy the pin target.
function clickPin(kind,ref){
 const board=controls.get("#board");
 const node=board.children.find(e=>e.tag==="g" && e.attrs?.transform?.includes("translate(") && e.children.some(t=>t.textContent===ref));
 assert(node,"component rendered: "+ref);
 const terminal=kind==="a"?node.children[0]:node.children[1];
 assert.equal(typeof terminal.events?.pointerup,"function","terminal needs pointerup event, not fragile click-after-rerender");
 let stopped=false;
 terminal.events.pointerup({stopPropagation(){stopped=true},preventDefault(){}});
 assert(stopped,"terminal pointerup must not bubble into component drag");
}
run('seed(false);wires=[];syncNodes();render()');
const startCount=run("wires.length");
const board=controls.get("#board");
const input=board.children.find(e=>e.tag==="g"&&e.children.some(t=>t.textContent==="IN"));
assert(input&&input.events.pointerup,"IN anchor should handle pointerup");
input.events.pointerup({stopPropagation(){},preventDefault(){}});
clickPin("a","R1");
assert.equal(run("wires.length"),startCount+1,"IN to resistor should create a visible wire");
assert.equal(run('wires[0].join("|")'),"IN|R1.a");
console.log("PASS: mouse pointerup on IN and resistor connects wire");

run('seed(true)');
let posted;
context.window={chrome:{webview:{postMessage(text){posted=text;},addEventListener(){}}}};
controls.get("#simulate-spice").onclick();
assert(posted&&posted.startsWith("VBAL_SIM\n"),"WebView2 bridge command must include a newline header");
assert(posted.includes("R1 in out"),"WebView2 must receive the actual graph");
assert(posted.includes(".model DGEN"),"WebView2 must receive the diode model");
console.log("PASS: WebView2 simulation button sends validated SPICE elements");

run('importSpiceText("* RC imported from EDA\\nV1 in 0 AC 1\\nR1 in mid 10k\\nC1 mid 0 22n\\nR2 mid out 4.7k\\n")');
assert.equal(run("parts.length"),3);
assert.equal(run("validate()"),true);
assert.equal(run('parts.find(p=>p.id==="R1").value'),10000);
assert(Math.abs(run('parts.find(p=>p.id==="C1").value')-22e-9)<1e-15);
assert.match(run("netlist()"),/R2 .* out 4700/);
assert(run("wires.length")>=4);
const imported=run('importSpiceText("* unnamed IO\\nVsig n001 0 AC 1\\nR5 n001 n002 1k\\nC9 n002 0 100n\\n")');
assert.equal(imported.inferred,true);
assert.equal(run("validate()"),true);
assert.throws(()=>run('importSpiceText("Q1 in out 0 NPN\\n")'),/Nieobsługiwany element/);
assert.throws(()=>run('importSpiceText("R1 in out 10xyz\\nC1 out 0 10n\\n")'),/Nieobsługiwany sufiks/);
console.log("PASS: SPICE netlist import rebuilds editable parts/wires and parses engineering suffixes");

const kicadLike=run('importSpiceText("KiCad schematic\\n.title demo\\nV1 in 0 AC 1\\nR1 in out 4.7k\\nC1 out 0 100n\\n.save all\\n.tran 1u 1m\\n")');
assert.equal(kicadLike.parts,2);
assert.equal(run("validate()"),true);
assert.equal(run('parts.find(p=>p.id==="R1").value'),4700);
console.log("PASS: KiCad/LTspice-style title and directives import");

run('importSpiceText("Physical plan\\nV1 in 0 AC 1\\nR1 in out 10k\\nC1 out 0 100n\\n")');
const plan=run("autoPhysicalLayout()");
assert.equal(run("physicalMode"),true);
assert.equal(["A","B","C","D","E","F","G","H","I","J"].includes(plan.assignments["R1.a"].name[0]),true);
assert.equal(["A","B","C","D","E","F","G","H","I","J"].includes(plan.assignments["C1.b"].name[0]),true);
assert(plan.netGroups.in&&plan.netGroups.in.length>=1);
assert(plan.lines.some(line=>line.includes("R1:")),"Physical plan should list concrete holes");
assert.equal(controls.get("#physical-plan").textContent.includes("PLAN PRAWDZIWEGO BREADBOARDU"),true);
run('physicalMode=false;physicalPlan=null;render()');
console.log("PASS: imported circuit maps to concrete breadboard hole groups and ground rail");

assert.equal(run('sameBreadboardNode(breadboardHole("a",5),breadboardHole("e",5))'),true);
assert.equal(run('sameBreadboardNode(breadboardHole("a",5),breadboardHole("f",5))'),false);
assert.equal(run('sameBreadboardNode(breadboardHole("T+",5),breadboardHole("T-",5))'),false);
run("boardColumns=63;splitRails=false");
assert.equal(run('sameBreadboardNode(breadboardHole("T+",10),breadboardHole("T+",50))'),true);
assert.equal(run('sameBreadboardNode(breadboardHole("B-",10),breadboardHole("B-",50))'),true);
run("splitRails=true");
assert.equal(run('sameBreadboardNode(breadboardHole("T+",10),breadboardHole("T+",50))'),false);
assert.equal(run('sameBreadboardNode(breadboardHole("B-",10),breadboardHole("B-",50))'),false);
run("boardColumns=30;splitRails=false");
console.log("PASS: real breadboard topology has A-E/F-J split, independent rails and split long rails");

run('importSpiceText('+JSON.stringify("Package plan\nV1 in 0 AC 1\nR1 in n1 10k\nC1 n1 out 100n\nR2 out 0 10k\n")+')');
const packagePlan=run("autoPhysicalLayout()");
assert.notEqual(run('stripId(physicalPlan.assignments["R1.a"])'),run('stripId(physicalPlan.assignments["R1.b"])'));
assert.notEqual(run('stripId(physicalPlan.assignments["C1.a"])'),run('stripId(physicalPlan.assignments["C1.b"])'));
assert(packagePlan.jumpers.length>0,"Package-first layout must add jumpers between separate strips");
console.log("PASS: package-first planner places component leads in different strips and derives jumpers");

run("seedOpampX2()");
const dipPlan=run("autoPhysicalLayout()");
assert.equal(dipPlan.packages["BOP1"].type,"dip8");
assert.equal(dipPlan.assignments["BOP1.minus"].row,"e");
assert.equal(dipPlan.assignments["BOP1.plus"].row,"e");
assert.equal(dipPlan.assignments["BOP1.vp"].row,"f");
assert.equal(dipPlan.assignments["BOP1.out"].row,"f");
console.log("PASS: op-amp physical footprint straddles center trench as DIP-8");



const kicadXml='<export version="D"><components>'+
'<comp ref="V1"><value>AC 1</value></comp>'+
'<comp ref="R1"><value>10k</value></comp>'+
'<comp ref="C1"><value>47n</value></comp>'+
'</components><nets>'+
'<net code="1" name="/IN"><node ref="V1" pin="1"/><node ref="R1" pin="1"/></net>'+
'<net code="2" name="/OUT"><node ref="R1" pin="2"/><node ref="C1" pin="1"/></net>'+
'<net code="3" name="GND"><node ref="V1" pin="2"/><node ref="C1" pin="2"/></net>'+
'</nets></export>';
const xmlResult=run('importKiCadXmlText('+JSON.stringify(kicadXml)+')');
assert.equal(xmlResult.parts,2);
assert.equal(run("validate()"),true);
assert.equal(run('parts.find(p=>p.id==="R1").a'),"in");
assert.equal(run('parts.find(p=>p.id==="R1").b'),"out");
assert.equal(run('parts.find(p=>p.id==="C1").b'),"0");
const badKicadXml='<export><components><comp ref="Q1"><value>NPN</value></comp></components><nets></nets></export>';
assert.throws(()=>run('importKiCadXmlText('+JSON.stringify(badKicadXml)+')'),/nieobsługiwany element/);
console.log("PASS: KiCad XML netlist imports R/C with /IN, /OUT and GND mapping");

run('importSpiceText("Anchor plan\\nV1 in 0 AC 1\\nR1 in out 10k\\nC1 out 0 100n\\n")');
const anchorPlan=run("autoPhysicalLayout()");
assert.equal(anchorPlan.anchorsPhysical.in.name,"E1");
assert.equal(anchorPlan.anchorsPhysical["0"].name.startsWith("B-"),true);
assert.notEqual(anchorPlan.assignments["R1.a"].name,anchorPlan.anchorsPhysical.in.name,"IN jumper must not occupy resistor pin hole");
assert.notEqual(anchorPlan.assignments["C1.b"].name,anchorPlan.anchorsPhysical["0"].name,"GND jumper must not occupy component ground hole");
console.log("PASS: physical planner reserves distinct IN/OUT/GND jumper holes");

run('importSpiceText("Physical validation\\nV1 in 0 AC 1\\nR1 in out 10k\\nC1 out 0 100n\\n")');
const safePlan=run("autoPhysicalLayout()");
assert.equal(run("validatePhysicalPlan(physicalPlan)"),true);
assert.throws(()=>run('(()=>{const p=JSON.parse(JSON.stringify(physicalPlan));p.assignments["C1.a"]={...p.assignments["R1.b"]};return validatePhysicalPlan(p)})()'),/kolizja otworu/);
console.log("PASS: physical plan rejects duplicate hole assignments");

run('seed(false)');
posted=undefined;
controls.get("#live-start").onclick();
assert(posted&&posted.startsWith("VBAL_LIVE|"),"Live start must send devices and current circuit to native host");
assert(posted.includes("R1 in out"),"Live start must send current topology");
posted=undefined;
controls.get("#live-stop").onclick();
assert.equal(posted,"VBAL_LIVE_STOP");
console.log("PASS: live audio Start/Stop controls send current breadboard circuit");

posted=undefined;
context.document.querySelector("#live-start").onclick();
assert(posted&&posted.startsWith("VBAL_LIVE|"));
context.document.querySelector("#live-stop").onclick();
assert.equal(posted,"VBAL_LIVE_STOP");
console.log("PASS: live audio controls survive status polling setup");

context.document.body.dataset.live="on";
posted=undefined;
run('connect("IN")');
assert.equal(posted,"VBAL_LIVE_STOP","Circuit edit must stop live audio before changing topology");
assert.equal(context.document.body.dataset.live,"off");
console.log("PASS: editing circuit automatically stops live audio");

run('seed(false)');
posted=undefined;
controls.get("#tape-open").onclick();
assert.equal(posted,"VBAL_TAPE_PICK","File picker must open independently from circuit validation");
posted=undefined;controls.get("#tape-play").onclick();
assert(posted&&posted.startsWith("VBAL_TAPE_PLAY|"),"Tape PLAY must send output device and current circuit");
assert(posted.includes("R1 in out"),"Tape PLAY must use current breadboard topology");
posted=undefined;controls.get("#tape-rewind").onclick();assert.equal(posted,"VBAL_TAPE_REWIND");
posted=undefined;controls.get("#tape-loop").onclick();assert.equal(posted,"VBAL_TAPE_LOOP_ON");
controls.get("#tape-loop").onclick();assert.equal(posted,"VBAL_TAPE_LOOP_OFF");
posted=undefined;controls.get("#tape-stop").onclick();assert.equal(posted,"VBAL_LIVE_STOP");
console.log("PASS: tape transport sends file/play, rewind, loop and stop commands");

run("newProject()");
assert.equal(run("parts.length"),0);assert.equal(run("wires.length"),0);
posted=undefined;controls.get("#tape-open").onclick();
assert.equal(posted,"VBAL_TAPE_PICK","Tape picker must work on an empty NEW project");
controls.get("#board-size").value="60";controls.get("#board-size").onchange({target:controls.get("#board-size")});
assert.equal(run("boardColumns"),60);assert(run("boardWidth()")>1500);
tools.find(x=>x.dataset.add==="L").onclick();
tools.find(x=>x.dataset.add==="LED").onclick();
assert.equal(run('parts.some(p=>p.kind==="L")'),true);
assert.equal(run('parts.some(p=>p.kind==="LED")'),true);
assert.equal(run('parts.find(p=>p.kind==="L").value'),0.01);
run('newProject();parts=[{id:"L1",kind:"L",a:"in",b:"out",value:0.01,x:200,y:200},{id:"DLED1",kind:"LED",a:"out",b:"0",value:0,x:400,y:200}];wires=[["IN","L1.a"],["L1.b","OUT"],["L1.b","DLED1.a"],["DLED1.b","GND"]];syncNodes()');
assert.equal(run("validate()"),true);
assert.match(run("elements()"),/L1 in out 0\.0100000000000/);
assert.match(run("elements()"),/DLED1 out 0 LEDGEN/);
assert.match(run("elements()"),/\.model LEDGEN/);
console.log("PASS: NEW, scalable board, tape picker, inductor and LED catalog");

run('seed(false)');
controls.get("#audio-input-device").value="2";
controls.get("#audio-output-device").value="3";
controls.get("#input-gain").value="6";
controls.get("#drive-gain").value="18";
controls.get("#output-gain").value="-12";
controls.get("#audio-bypass").checked=false;
posted=undefined;controls.get("#live-start").onclick();
assert(posted.startsWith("VBAL_LIVE|2|3\n"));
posted=undefined;run("sendAudioSettings()");
assert.equal(posted,"VBAL_AUDIO_SETTINGS|6|18|-12|0|0");
posted=undefined;controls.get("#audio-bypass").checked=true;run("sendAudioSettings()");
assert.equal(posted,"VBAL_AUDIO_SETTINGS|6|18|-12|1|0");
controls.get("#audio-input-mode").value="2";posted=undefined;run("sendAudioSettings()");
assert.equal(posted,"VBAL_AUDIO_SETTINGS|6|18|-12|1|2");
console.log("PASS: live input/output device, channel mode and gain/drive/bypass settings bridge");

run('newProject();parts=[{id:"DSCH1",kind:"SCH",a:"in",b:"out",value:0,x:200,y:120},{id:"DGE1",kind:"GE",a:"out",b:"n1",value:0,x:350,y:180},{id:"DZ1",kind:"Z",a:"n1",b:"0",value:0,x:500,y:240}];wires=[["IN","DSCH1.a"],["DSCH1.b","OUT"],["DSCH1.b","DGE1.a"],["DGE1.b","DZ1.a"],["DZ1.b","GND"]];syncNodes()');
assert.equal(run("validate()"),true);
const diodeDeck=run("elements()");
assert.match(diodeDeck,/DSCH1 in out SCHGEN/);
assert.match(diodeDeck,/DGE1 out n\d+ GEGEN/);
assert.match(diodeDeck,/DZ1 n\d+ 0 Z5V1/);
assert.match(diodeDeck,/\.model SCHGEN/);assert.match(diodeDeck,/\.model GEGEN/);assert.match(diodeDeck,/\.model Z5V1/);
console.log("PASS: Schottky, germanium and Zener UI models serialize");

run('loadEffectFromLibrary("lpf")');
assert.equal(run('parts.find(p=>p.id==="C1").value'),1e-7);
assert.equal(run("validate()"),true);
run('seedDiodeClip("SCH")');
assert.equal(run('parts.filter(p=>p.kind==="SCH").length'),2);
assert.match(run("elements()"),/SCHGEN/);
run('loadEffectFromLibrary("fuzz")');
assert.equal(run('parts.filter(p=>p.kind==="PNP"&&p.material==="Ge").length'),2);
assert.match(run("elements()"),/GPNP_Q1 PNP\(Is=5e-6/);
assert.match(run("elements()"),/GPNP_Q2 PNP\(Is=5e-6/);
run('loadEffectFromLibrary("gefuzz")');
assert.equal(run('parts.filter(p=>p.kind==="GE").length'),2);
assert.match(run("elements()"),/GEGEN/);
run('loadEffectFromLibrary("distortion")');
assert.equal(run('parts.filter(p=>p.kind==="LED").length'),2);
assert.match(run("elements()"),/LEDGEN/);
console.log("PASS: strong RC and nonlinear clipping presets are electrically valid");

run('seed(false)');
assert.equal(run('plainDecimal(parts.find(p=>p.id==="C1").value)'),"0.00000001");
assert.match(run("elements()"),/C1 out 0 0\.00000001/);
run('seedStrongRc()');
assert.equal(run('plainDecimal(parts.find(p=>p.id==="C1").value)'),"0.0000001");
run('seedDiodeClip("D")');
assert.equal(run('plainDecimal(parts.find(p=>p.id==="C1").value)'),"0.000000047");
console.log("PASS: capacitor values use plain decimal farads, not exponent notation");

run("newProject()");
tools.find(x=>x.dataset.add==="POT").onclick();
assert.equal(run("parts.length"),1);
assert.equal(run('parts[0].kind'),"POT");
assert.equal(run('parts[0].value'),100000);
assert.equal(run('parts[0].position'),0.5);
run('connect("IN");connect("P1.a");connect("P1.w");connect("OUT");connect("P1.b");connect("GND")');
assert.equal(run("validate()"),true);
let potDeck=run("elements()");
assert.match(potDeck,/RP1A in out 50000/);
assert.match(potDeck,/RP1B out 0 50000/);
run('parts[0].position=0.25');
potDeck=run("elements()");
assert.match(potDeck,/RP1A in out 25000/);
assert.match(potDeck,/RP1B out 0 75000/);
const potPlan=run("autoPhysicalLayout()");
assert(potPlan.assignments["P1.w"],"Pot wiper must receive a physical breadboard hole");
console.log("PASS: 3-pin potentiometer validates, expands to two resistors and receives physical holes");

run("newProject()");
tools.find(x=>x.dataset.add==="VDC").onclick();
assert.equal(run('parts[0].kind'),"VDC");
assert.equal(run('parts[0].value'),9);
run('parts.push({id:"R1",kind:"R",a:"in",b:"out",value:1000,x:300,y:150});parts.push({id:"R2",kind:"R",a:"out",b:"vcc",value:1000,x:450,y:200});parts[0].a="vcc";parts[0].b="0";wires=[["IN","R1.a"],["R1.b","OUT"],["R1.b","R2.a"],["R2.b","V1.a"],["V1.b","GND"]];syncNodes()');
assert.equal(run("validate()"),true);
assert.match(run("elements()"),/V1 .* 0 9\.00000000000/);
console.log("PASS: DC supply element serializes into live/SPICE circuit");

run('newProject();parts='+
JSON.stringify([
{id:"V1",kind:"VDC",a:"vcc",b:"0",value:9,x:180,y:100},
{id:"RIN",kind:"R",a:"in",b:"base",value:10000,x:300,y:140},
{id:"RB",kind:"R",a:"vcc",b:"base",value:100000,x:420,y:180},
{id:"RC",kind:"R",a:"vcc",b:"out",value:4700,x:540,y:220},
{id:"Q1",kind:"NPN",c:"out",b:"base",e:"0",value:100,x:660,y:260}
])+';wires=[["IN","RIN.a"],["RIN.b","Q1.b"],["V1.a","RB.a"],["RB.b","Q1.b"],["V1.a","RC.a"],["RC.b","OUT"],["OUT","Q1.c"],["Q1.e","GND"],["V1.b","GND"]];syncNodes()');
assert.equal(run("validate()"),true);
const npnDeck=run("elements()");
assert.match(npnDeck,/V1 .* 0 9\.00000000000/);
assert.match(npnDeck,/Q1 out n\d+ 0 NPNGEN/);
assert.match(npnDeck,/\.model NPNGEN NPN/);
const npnPlan=run("autoPhysicalLayout()");
assert(npnPlan.assignments["Q1.c"]&&npnPlan.assignments["Q1.b"]&&npnPlan.assignments["Q1.e"]);
console.log("PASS: powered NPN C/B/E circuit serializes and maps to breadboard holes");

run("newProject()");
run('$("#supply-plus").value="12"');
controls.get("#power-plus").onclick();
assert.equal(run("parts.length"),1);
assert.equal(run('parts[0].kind'),"VDC");
assert.equal(run('parts[0].value'),12);
assert(run('wires.some(w=>w.includes("V1.b")&&w.includes("GND"))'));
run('$("#supply-plus").value="9";$("#supply-minus").value="9"');
controls.get("#power-bipolar").onclick();
assert.equal(run('parts.filter(p=>p.kind==="VDC").length'),3);
assert(run('parts.some(p=>p.value===9)'));
assert(run('parts.some(p=>p.value===-9)'));
console.log("PASS: dedicated +V/-V/bipolar power panel creates grounded supply sources");

run("newProject()");
tools.find(x=>x.dataset.add==="PNP").onclick();
assert.equal(run('parts[0].kind'),"PNP");
assert.deepEqual(Array.from(run('terminalSides(parts[0])')),["c","b","e"]);
run("newProject()");
tools.find(x=>x.dataset.add==="NMOS").onclick();
assert.equal(run('parts[0].kind'),"NMOS");
assert.deepEqual(Array.from(run('terminalSides(parts[0])')),["d","g","s"]);
run('newProject();parts=[{id:"V1",kind:"VDC",a:"vcc",b:"0",value:9,x:100,y:100},{id:"R1",kind:"R",a:"in",b:"out",value:1000000,x:220,y:100},{id:"QP1",kind:"PNP",c:"out",b:"in",e:"vcc",value:100,x:360,y:140},{id:"M1",kind:"NMOS",d:"out",g:"in",s:"0",value:2,x:500,y:180},{id:"R2",kind:"R",a:"out",b:"0",value:1000,x:650,y:200}];wires=[["IN","R1.a"],["R1.b","OUT"],["QP1.c","OUT"],["QP1.b","IN"],["QP1.e","V1.a"],["M1.d","OUT"],["M1.g","IN"],["M1.s","GND"],["R2.a","OUT"],["R2.b","GND"],["V1.b","GND"]];syncNodes()');
assert.equal(run("validate()"),true);
const activeDeck=run("elements()");
assert.match(activeDeck,/QP1 out in .* PNPGEN/);
assert.match(activeDeck,/M1 out in 0 NMOSGEN/);
assert.match(activeDeck,/\.model PNPGEN PNP/);
assert.match(activeDeck,/\.model NMOSGEN NMOS/);
console.log("PASS: PNP and NMOS UI serialize with real 3-pin models");

run("newProject()");
tools.find(x=>x.dataset.add==="NJFET").onclick();
assert.equal(run('parts[0].kind'),"NJFET");
assert.deepEqual(Array.from(run('terminalSides(parts[0])')),["d","g","s"]);
run('newProject();parts=[{id:"V1",kind:"VDC",a:"vcc",b:"0",value:9,x:100,y:100},{id:"RD",kind:"R",a:"vcc",b:"out",value:2200,x:260,y:120},{id:"J1",kind:"NJFET",d:"out",g:"in",s:"0",value:2,x:430,y:180}];wires=[["IN","J1.g"],["J1.d","OUT"],["RD.b","OUT"],["RD.a","V1.a"],["J1.s","GND"],["V1.b","GND"]];syncNodes()');
assert.equal(run("validate()"),true);
const jfetDeck=run("elements()");
assert.match(jfetDeck,/J1 out in 0 NJFETGEN/);
assert.match(jfetDeck,/\.model NJFETGEN NJF/);
console.log("PASS: NJFET UI serializes D/G/S and model");

run("newProject()");
tools.find(x=>x.dataset.add==="OPAMP").onclick();
assert.equal(run('parts[0].kind'),"OPAMP");
assert.deepEqual(Array.from(run('terminalSides(parts[0])')),["out","plus","minus","vp","vm"]);
run("seedOpampX2()");
assert.equal(run("validate()"),true);
const opDeck=run("elements()");
assert.match(opDeck,/BOP1 .* 0 V=max\(/);
assert.match(opDeck,/100000\*\(V\(/);
const opPlan=run("autoPhysicalLayout()");
for(const pin of ["out","plus","minus","vp","vm"])assert(opPlan.assignments["BOP1."+pin],"op-amp pin "+pin+" needs breadboard hole");
console.log("PASS: 5-pin op-amp preset validates, serializes and maps all rail/input/output pins");

run('loadEffectFromLibrary("delay")');
assert.equal(run("selectedMacroFx"),2);
assert.equal(posted,"VBAL_FX_MODE|2");
assert.equal(run("validate()"),true);
run('loadEffectFromLibrary("chorus")');
assert.equal(run("selectedMacroFx"),3);
assert.equal(posted,"VBAL_FX_MODE|3");
run('loadEffectFromLibrary("compressor")');
assert.equal(run("selectedMacroFx"),1);
run('loadEffectFromLibrary("eq")');
assert.equal(run("selectedMacroFx"),5);
run('loadEffectFromLibrary("reverb")');
assert.equal(run("selectedMacroFx"),6);
assert.equal(posted,"VBAL_FX_MODE|6");
run('loadEffectFromLibrary("flanger")');
assert.equal(run("selectedMacroFx"),7);
assert.equal(posted,"VBAL_FX_MODE|7");
run('loadEffectFromLibrary("hpf")');
assert.equal(run("selectedMacroFx"),0);
assert.equal(run("validate()"),true);
assert.equal(run('parts.find(p=>p.id==="C1").value'),0.0000001);
console.log("PASS: effects list loads analog circuits and live macro FX modes without preset buttons");


// Physical holes, not pre-existing logical wires, must determine the simulated circuit.
run('seed(false)');
const physical=run('autoPhysicalLayout()');
const savedPhysical=run('physicalSnapshot()');
assert(savedPhysical.assignments["R1.a"] && savedPhysical.jumpers.length > 0);
assert.equal(run('physicalResolved()["R1.a"]'), "in");
assert.equal(run('validate()'), true);
assert.match(run('elements()'), /R1 in out/);
assert.equal(run('movePhysicalPin("R1.a",breadboardHole("a",20))'),true);
assert.notEqual(run('physicalResolved()["R1.a"]'),"in","Moving a pin to an unconnected strip must disconnect that node");
assert.throws(()=>run('validate()'),/niepołączone|Brak połączenia/);
assert.equal(run('restorePhysicalSnapshot('+JSON.stringify(savedPhysical)+')'),true);
assert.equal(run('physicalResolved()["R1.a"]'),"in");
assert.equal(run('validate()'),true);
console.log("PASS: physical pin movement updates SPICE nets and saved physical layout restores them");

const beforeJumpers=run('physicalPlan.jumpers.length');
run('physicalPlan.jumpers=[]');
assert.throws(()=>run('validate()'),/niepołączone|Brak połączenia/);
run('restorePhysicalSnapshot('+JSON.stringify(savedPhysical)+')');
assert.equal(run('physicalPlan.jumpers.length'),beforeJumpers);
console.log("PASS: removing real jumpers disconnects circuit even when schematic wires remain");

assert.throws(()=>run('movePhysicalPin("R1.a",physicalPlan.assignments["R1.b"])'),/zajęty/);
run('boardColumns=63;splitRails=true');
assert.equal(run('holeFromName("T+L10").segment'),0);
assert.equal(run('holeFromName("T+R50").segment'),1);
assert.throws(()=>run('holeFromName("T+L50")'),/segment/);
console.log("PASS: physical hole validation rejects occupied sockets and split-rail mismatch");


// New components are physically mounted without destroying existing pin positions.
run('seed(false);autoPhysicalLayout()');
for(const tool of tools){
 const before=run('Object.keys(physicalPlan.assignments).length');
 tool.onclick();
 assert.equal(run("physicalMode"),true,"Adding a component must stay in breadboard mode");
 assert(run('Object.keys(physicalPlan.assignments).length')>before,"Physical part placement must add assigned holes: "+tool.dataset.add);
}
console.log("PASS: all 16 component categories mount physically without resetting the breadboard");

for(const effect of ["clean","overdrive","distortion","fuzz","boost","npnboost","compressor","delay","chorus","tremolo","eq","reverb","flanger","lpf","lpf-light","hpf"]){
 run('loadEffectFromLibrary('+JSON.stringify(effect)+')');
 assert.equal(run("physicalMode"),true,"Effect must open on physical breadboard: "+effect);
 assert.equal(run("validate()"),true,"Effect must validate after physical placement: "+effect);
 assert(run('physicalPlan.jumpers.length')>0,"Effect needs physical jumpers: "+effect);
}
console.log("PASS: all 16 effect examples load on the physical board and validate");


// Real topology examples: changing components changes exported equations, not just a label.
for(const name of ["threeband","gefuzz","fuzz"]){
 run('loadEffectFromLibrary('+JSON.stringify(name)+')');
 assert.equal(run("physicalMode"),true);
 assert.equal(run("validate()"),true);
 assert(run('physicalPlan.jumpers.length')>0);
}
run('loadEffectFromLibrary("threeband")');
const eqBefore=run('elements()');
run('parts.find(p=>p.id==="C1").value=0.00000001');
const eqAfter=run('elements()');
assert.notEqual(eqBefore,eqAfter);
assert.match(eqAfter,/C1 .* 0\.00000001/);
console.log("PASS: three-band EQ loads physical R/C network and capacitor edits change solver input");

run('loadEffectFromLibrary("fuzz")');
const geFuzz9=run('elements()');
assert.match(geFuzz9,/GPNP_Q1 PNP\(Is=5e-6 Bf=85 Vaf=40 Eg=0\.72\)/);
assert.match(geFuzz9,/GPNP_Q2 PNP\(Is=5e-6 Bf=120 Vaf=40 Eg=0\.72\)/);
run('parts.find(p=>p.id==="V1").value=-6');
const geFuzz6=run('elements()');
assert.notEqual(geFuzz9,geFuzz6);
assert.match(geFuzz6,/V1 .* -6\.000/);
run('parts.find(p=>p.id==="Q1").value=110');
assert.match(run('elements()'),/GPNP_Q1 PNP\(Is=5e-6 Bf=110/);
console.log("PASS: germanium Fuzz Face negative supply and both BJT beta models affect the SPICE deck");

run('loadEffectFromLibrary("gefuzz")');
const geDiode=run('elements()');
run('parts.find(p=>p.id==="DGE1").kind="D"');
const siDiode=run('elements()');
assert.notEqual(geDiode,siDiode);
assert.match(siDiode,/DGE1 out 0 DGEN/);
assert.match(siDiode,/\.model DGEN/);
console.log("PASS: replacement of germanium with silicon diode changes the actual SPICE model");

run('loadEffectFromLibrary("fuzz")');
const beforeAssembly=run('physicalSnapshot()');
const movedPackage=run('relocatePhysicalPackage("R1",1)');
assert.equal(movedPackage,true);
const afterAssembly=run('physicalSnapshot()');
assert.notEqual(beforeAssembly.assignments["R1.a"],afterAssembly.assignments["R1.a"]);
assert.notEqual(beforeAssembly.assignments["R1.b"],afterAssembly.assignments["R1.b"]);
assert.throws(()=>run('relocatePhysicalPackage("R1",999)'),/Nie ma takiej kolumny/);
run('restorePhysicalSnapshot('+JSON.stringify(beforeAssembly)+')');
assert.equal(run('physicalSnapshot().assignments["R1.a"]'),beforeAssembly.assignments["R1.a"]);
console.log("PASS: moving entire component snaps all its pins and can be reversed");


run('loadEffectFromLibrary("threeband")');
const originalCap=run('parts.find(p=>p.id==="C1").value');
context.document.body.dataset.live="on";
run('activeAudioSource="live"');
posted=undefined;
run('editAndRestartAudio(()=>{parts.find(p=>p.id==="C1").value=0.00000001})');
assert(posted?.startsWith("VBAL_LIVE|"),"Changing a component must restart the real circuit live audio");
assert(posted.includes("C1 ")&&posted.includes("0.00000001"),"Updated electrical value must reach Windows DSP graph");
assert.notEqual(run('parts.find(p=>p.id==="C1").value'),originalCap);
console.log("PASS: capacitor edit restarts WASAPI live audio with recalculated physical SPICE circuit");


run('newProject();parts=[{id:"R1",kind:"R",a:"in",b:"out",value:10000,x:200,y:150},{id:"R2",kind:"R",a:"out",b:"0",value:1000,x:400,y:150},{id:"M1",kind:"NMOS",d:"out",g:"in",s:"0",value:3.25,x:450,y:250},{id:"V1",kind:"VDC",a:"vcc",b:"0",value:9,x:250,y:100},{id:"R3",kind:"R",a:"vcc",b:"out",value:4700,x:350,y:140}];wires=[["IN","R1.a"],["R1.b","OUT"],["R2.a","OUT"],["R2.b","GND"],["M1.d","OUT"],["M1.g","IN"],["M1.s","GND"],["V1.a","R3.a"],["V1.b","GND"],["R3.b","OUT"]];syncNodes()');
assert.match(run("elements()"),/M1 out in 0 NMOS_M1/);
assert.match(run("elements()"),/NMOS_M1 NMOS\(VTO=3\.25/);
run('parts.find(p=>p.id==="M1").value=1.5');
assert.match(run("elements()"),/NMOS_M1 NMOS\(VTO=1\.5/);
run('parts.push({id:"J1",kind:"NJFET",d:"out",g:"in",s:"0",value:1.4,x:300,y:270});wires.push(["J1.d","OUT"],["J1.g","IN"],["J1.s","GND"]);syncNodes()');
assert.match(run("elements()"),/J1 out in 0 NJFET_J1/);
assert.match(run("elements()"),/NJFET_J1 NJF\(BETA=0\.002 VTO=-1\.4/);
console.log("PASS: NMOS threshold and JFET pinch-off changes are serialized as real electrical models");


run('newProject();parts=[{id:"R1",kind:"R",a:"in",b:"out",value:1000},{id:"R2",kind:"R",a:"out",b:"0",value:100000},{id:"Q1",kind:"NPN",c:"out",b:"in",e:"0",value:100}];wires=[["IN","R1.a"],["R1.b","OUT"],["R2.a","OUT"],["R2.b","GND"],["Q1.c","OUT"],["Q1.b","IN"],["Q1.e","GND"]];syncNodes()');
assert.throws(()=>run('elements()'),/wymaga rzeczywistego zasilania DC/);
run('parts.pop();wires=wires.filter(w=>!w.some(k=>k.startsWith("Q1.")));syncNodes()');
assert.equal(run('validate()'),true,"Passive signal-powered networks must not require batteries");
console.log("PASS: unpowered active devices are rejected while passive circuits remain valid");


// CAD user workflow: physical board first, mouse/keyboard placement and schematic.
run('freshPhysicalBoard()');
assert.equal(run('physicalMode'),true);
assert.equal(run('parts.length'),0);
const snappedId=run('placeCatalogPartAt("R",11,"j")');
assert.equal(run('physicalPlan.assignments["'+snappedId+'.a"].row'),"j");
assert.equal(run('physicalPlan.assignments["'+snappedId+'.a"].col'),11);
assert.equal(run('physicalPlan.assignments["'+snappedId+'.b"].row'),"j");
run('showView("schematic")');
assert.equal(controls.get("#view-schematic").attrs["aria-pressed"],"true");
assert(controls.get("#schematic").children.length>0,"Schematic must contain actual SVG symbol elements");
run('showView("compare")');
assert.equal(controls.get("#view-compare").attrs["aria-pressed"],"true");
console.log("PASS: empty physical board, drop component snapped to row J and autogenerated schematic");

run('loadEffectFromLibrary("gefuzz")');
const beforePolarity=run('physicalSnapshot()');
run('rotatePhysicalPart("DGE1")');
const afterPolarity=run('physicalSnapshot()');
assert.equal(afterPolarity.assignments["DGE1.a"],beforePolarity.assignments["DGE1.b"]);
assert.equal(afterPolarity.assignments["DGE1.b"],beforePolarity.assignments["DGE1.a"]);
assert.notEqual(run('elements()').includes("DGE1 out 0"),true,"Reversing diode must reverse its actual SPICE anode/cathode");
console.log("PASS: diode direction reversal changes physical pins and electrical diode direction");

run('loadEffectFromLibrary("gefuzz")');
run('showContextMenu({preventDefault(){},stopPropagation(){},clientX:100,clientY:100},{type:"part",id:"DGE1"})');
const menu=controls.get("#context-menu");
assert(menu.children.some(btn=>String(btn.textContent).includes("Usuń pojedynczy element")));
assert(menu.children.some(btn=>String(btn.textContent).includes("Odwróć orientację")));
run('deletePhysicalPart("DGE1")');
assert.equal(run('parts.some(p=>p.id==="DGE1")'),false);
assert.equal(run('physicalPlan.assignments["DGE1.a"]'),undefined);
console.log("PASS: right-click menu offers deletion and polarity reversal without deleting other components");

run('loadEffectFromLibrary("threeband")');
posted=undefined;
run('selectVoltageProbe(physicalPlan.assignments["R1.b"])');
assert(posted&&posted.startsWith("VBAL_PROBE|"),"Voltage probe must request an existing physical MNA node");
run('selectCurrentProbe("R1")');
assert(posted.endsWith("|R1|0"),"Ammeter must request physically solved resistor branch referenced to GND");
run('drawScopeTrace([0,0.1,0.4,-0.2,0.1],0.15,0.0015,true)');
assert(controls.get("#probe-meter").textContent.includes("0.150 V"));
assert(controls.get("#probe-current").textContent.includes("1.500 mA"));
assert(controls.get("#probe-trace").children.length>=2);
console.log("PASS: on-board voltage/current probes and oscilloscope trace bind to native telemetry");

// DCACLab-inspired workbench: visual palette remains actionable and electrically modelled.
for(const kind of ["R","C","GE","PNP","OPAMP","VDC"]){
 const button=tools.find(btn=>btn.dataset.add===kind);
 assert(button.children.some(ch=>ch.className==="component-thumb"),"Missing original vector component thumbnail for "+kind);
}
assert(html.includes("grid-template-columns:minmax(0,1fr) 260px"),"Breadboard work area must dominate layout");
assert(html.includes("grid-column:1/-1;grid-row:1"),"Palette must form horizontal strip above board");
console.log("PASS: light full-width component palette has interactive vector thumbnails");


// A schematic must show REAL copper nets with orthogonal wires, not detached part cards.
run('loadEffectFromLibrary("threeband")');
run('showView("schematic")');
const schematic=controls.get("#schematic");
const schematicLines=schematic.children.filter(x=>x.tag==="line");
const schematicNodes=schematic.children.filter(x=>x.tag==="circle");
assert(schematicLines.length>25,"Circuit schematic needs actual drawn electrical wires");
assert(schematicNodes.length>8,"Conductive junctions must have visible connection dots");
assert(!schematic.children.some(x=>x.tag==="rect"&&x.attrs?.rx==7),"No detached per-component netlist cards");
assert.equal(run('buildSchematicTopology().terminals.get("out").length')>=3,true);
console.log("PASS: electrical schematic connects standard component symbols by conductive nets, including junction dots");


// Manufacturer-independent jumper kit: retain exact electrical graph while editing physical leads.
run('freshPhysicalBoard()');
const a=run('breadboardHole("c",10)'),b=run('breadboardHole("c",14)'),newEnd=run('breadboardHole("d",17)');
assert.equal(run('addPhysicalJumper(breadboardHole("c",10),breadboardHole("c",14))'),true);
assert.equal(run('physicalPlan.jumpers.length'),1);
assert(run('jumperInformation(physicalPlan.jumpers[0]).length')>3.9);
assert.match(run('jumperInformation(physicalPlan.jumpers[0]).color'),/^#[0-9a-f]{6}$/i);
const wireSnap=run('physicalSnapshot()');
run('physicalPlan.jumpers[0].color="#d14c43"');
const colorSnap=run('physicalSnapshot()');
assert.equal(colorSnap.jumpers[0][2],"#d14c43");
run('restorePhysicalSnapshot('+JSON.stringify(colorSnap)+')');
assert.equal(run('physicalPlan.jumpers[0].color'),"#d14c43");
assert.equal(run('physicallyRerouteJumper(0,"to",breadboardHole("d",17))'),true);
assert.equal(run('physicalPlan.jumpers[0].to.name'),"D17");
assert.equal(run('physicalPlan.jumpers[0].from.name'),"C10");
assert.throws(()=>run('physicallyRerouteJumper(0,"to",breadboardHole("c",10))'),/zajęty|kolizja/);
assert.equal(run('physicalPlan.jumpers[0].to.name'),"D17","Illegal reroute must rollback physical endpoint");
run('selectJumper(0)');
assert(run('selectedJumperIndex')===0);
assert.equal(run('deleteSelectedJumper()'),true);
assert.equal(run('physicalPlan.jumpers.length'),0);
console.log("PASS: jumper lengths and colors, saved kit properties, endpoint rerouting and single-delete");


// Full-state undo/redo must restore both breadboard copper connectivity and UI-independent components.
run('freshPhysicalBoard()');
const blankState=run('currentHistoryFrame()');
run('addPhysicalJumper(breadboardHole("c",10),breadboardHole("c",14))');
const wiredState=run('currentHistoryFrame()');
assert.notEqual(blankState,wiredState);
assert.equal(run('undoProjectChange()'),true);
assert.equal(run('currentHistoryFrame()'),blankState);
assert.equal(run('physicalPlan.jumpers.length'),0);
assert.equal(run('redoProjectChange()'),true);
assert.equal(run('currentHistoryFrame()'),wiredState);
assert.equal(run('physicalPlan.jumpers.length'),1);
console.log("PASS: undo/redo restores exact physical wiring and project state");

// Diagnostics are a connectivity test and must not misrepresent an operating-point calculation.
run('loadEffectFromLibrary("threeband")');
assert.equal(run('diagnoseDcPaths().active'),0);
assert.equal(run('diagnoseDcPaths().warnings.length'),0);
run('loadEffectFromLibrary("fuzz")');
assert.equal(run('diagnoseDcPaths().active'),2);
run('newProject();parts=[{id:"R1",kind:"R",a:"in",b:"out",value:1000},{id:"Q1",kind:"NPN",c:"collector",b:"in",e:"0",value:100},{id:"V1",kind:"VDC",a:"vcc",b:"0",value:9}];wires=[];syncNodes();render()');
const diagnostic=run('diagnoseDcPaths()');
assert.equal(diagnostic.active,1);
assert(diagnostic.warnings.some(x=>x.includes("Q1")&&x.includes("brak ścieżki DC")));
console.log("PASS: DC diagnostics distinguish passive circuits from floating active transistors");


// Differential probes must map two independently selected physical nodes.
run('loadEffectFromLibrary("threeband")');
posted=undefined;
run('selectVoltageProbe(physicalPlan.assignments["R1.a"],false)');
run('selectVoltageProbe(physicalPlan.assignments["R1.b"],true)');
assert(posted&&posted.startsWith("VBAL_PROBE|"),"Physical probes must issue a solver measurement");
const differential=posted.split("|");
assert.equal(differential.length,4,"Differential probe message must carry two nodes and resistor reference");
assert.notEqual(differential[1],differential[3],"Two probe tips must resolve to different electrical nets");
assert.equal(controls.get("#probe-reference").value,differential[3]);
run('drawScopeTrace([0.1,-0.2,0.3],0.25,0,false)');
assert(controls.get("#probe-meter").textContent.includes(" − "),"Meter display must show the reference probe");
console.log("PASS: differential oscilloscope and voltmeter probes use separate physical nodes");


// Board zoom is a viewport-only operation: it must never mutate solved nets.
run('loadEffectFromLibrary("threeband")');
const viewBefore=run('elements()'), historyBeforeZoom=run('currentHistoryFrame()');
run('setBoardZoom(1.2)');
assert.equal(run('boardZoom'),1.2);
assert.equal(run('elements()'),viewBefore);
assert.equal(run('currentHistoryFrame()'),historyBeforeZoom);
assert.equal(controls.get("#zoom-value").textContent,"120%");
run('setBoardZoom(0.2)');
assert.equal(run('boardZoom'),0.4);
console.log("PASS: fit-ready zoom preserves every physical electrical connection");


// The Wampler-inspired calculator is an ideal RC comparison, never a claim that
// this separately computed curve represents the complete electrical circuit.
assert.equal(run('nearestE12(10000)'),10000);
assert.equal(run('nearestE12(11300)'),12000);
assert.equal(run('nearestE12(10600)'),10000);
assert.equal(run('nearestE12(0.00000001)'),0.00000001);
assert.throws(()=>run('nearestE12(-1)'),/dodatnia/);
const cutoff=run('rcCutoff(10000,0.00000001)');
assert(Math.abs(cutoff-1591.5494)<0.01);
assert(Math.abs(run('rcDb('+cutoff+','+cutoff+',"lp")')+3.0103)<0.0001);
assert(Math.abs(run('rcDb('+cutoff+','+cutoff+',"hp")')+3.0103)<0.0001);
run('loadEffectFromLibrary("threeband")');
run('renderRcAnalysis()');
assert(controls.get("#rc-curve")?.children?.length>=0);
const beforeE12=run('elements()');
run('parts.find(p=>p.id==="R1").value=11300');
run('parts.find(p=>p.id==="C1").value=0.0000000113');
const exactE12=run('elements()');
run('parts.find(p=>p.id==="R1").value=nearestE12(parts.find(p=>p.id==="R1").value)');
run('parts.find(p=>p.id==="C1").value=nearestE12(parts.find(p=>p.id==="C1").value)');
const roundedE12=run('elements()');
assert.notEqual(exactE12,roundedE12);
assert.notEqual(beforeE12,roundedE12);
console.log("PASS: ideal RC cutoff/E12 calculations and real circuit resistor/capacitor edits");


// PT2399: real 16-pin DIP body, every leg in an individual breadboard hole,
// mixed-signal X-device exported with resistor-controlled oscillator and real +5V.
run('loadEffectFromLibrary("pt2399")');
assert.equal(run('parts.find(p=>p.id==="X1").kind'),"PT2399");
assert.equal(run('terminalSides(parts.find(p=>p.id==="X1")).length'),16);
assert.equal(run('physicalMode'),true,'PT2399 physical placement error: '+run('status.textContent'));
assert.equal(run('physicalPlan.packages.X1.type'),"dip16");
const x1=run('physicalPlan.packages.X1.col');
assert.equal(run('physicalPlan.assignments["X1.p1"].name'),"E"+x1);
assert.equal(run('physicalPlan.assignments["X1.p8"].name'),"E"+(x1+7));
assert.equal(run('physicalPlan.assignments["X1.p9"].name'),"F"+(x1+7));
assert.equal(run('physicalPlan.assignments["X1.p16"].name'),"F"+x1);
assert.equal(run('validate()'),true);
assert.equal(run('resolved()["X1.p3"]'),"0");
assert.equal(run('resolved()["X1.p4"]'),"0");
const ptDeck=run('elements()');
const ptRow=ptDeck.split("\n").find(line=>line.startsWith("X1 "));
assert.equal(ptRow.split(/\s+/).length,18);
assert.match(ptRow,/ PT2399_VBAL$/);
assert.match(ptDeck,/^V1 \S+ 0 5(?:\.0+)?$/m);
assert.equal(run('parts.find(p=>p.id==="R2").value'),2000);
run('showView("schematic")');
assert(controls.get("#schematic").children.length>10);
console.log("PASS: real 16-pin PT2399 physical placement, power, pin-6 resistor, 16-node netlist and schematic");


// MN3101 oscillator and MN3007 BBD use two genuinely distinct DIP-8 packages.
run('loadEffectFromLibrary("bbdchorus")');
assert.equal(run('physicalMode'),true,'Physical BBD placement: '+run('status.textContent'));
assert.equal(run('parts.find(p=>p.id==="X1").kind'),"MN3101");
assert.equal(run('parts.find(p=>p.id==="X2").kind'),"MN3007");
assert.equal(run('physicalPlan.packages.X1.type'),"dip8-mn");
assert.equal(run('physicalPlan.packages.X2.type'),"dip8-mn");
assert.equal(run('terminalSides(parts.find(p=>p.id==="X1")).length'),8);
assert.equal(run('terminalSides(parts.find(p=>p.id==="X2")).length'),8);
assert.equal(run('resolved()["X1.p2"]'),run('resolved()["X2.p2"]'));
assert.equal(run('resolved()["X1.p4"]'),run('resolved()["X2.p6"]'));
assert.equal(run('resolved()["X1.p8"]'),run('resolved()["X2.p4"]'));
const bbdNet=run('elements()');
assert.match(bbdNet,/X1 \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ MN3101_VBAL/);
assert.match(bbdNet,/X2 \S+ \S+ \S+ \S+ \S+ \S+ \S+ \S+ MN3007_VBAL/);
assert.match(bbdNet,/^V1 \S+ 0 -15(?:\.0+)?$/m);
run('showView("schematic")');
assert(controls.get("#schematic").children.length>15);
console.log("PASS: two separate MN3101/MN3007 DIP8 footprints, clock wiring, power and real BBD netlist");


// Importer consumes structured KiCad netlist data (not pixel-guessing).
for(const effect of ["pt2399","bbdchorus"]){
 run('loadEffectFromLibrary('+JSON.stringify(effect)+')');
 const exported=run('parts.map(p=>({...p}))');
 const expectedIds=exported.map(p=>p.id);
 const kindValue=p=>p.kind==="VDC"?"DC "+p.value:
  ["PT2399","MN3007","MN3101"].includes(p.kind)?p.kind:String(p.value);
 const escape=s=>String(s).replace(/&/g,"&amp;").replace(/"/g,"&quot;");
 const components=exported.map(p=>'<comp ref="'+escape(p.id)+'"><value>'+escape(kindValue(p))+'</value></comp>').join("");
 const groups=new Map();
 for(const part of exported){
  const sides=["PT2399","MN3007","MN3101"].includes(part.kind)?
   Array.from({length:part.kind==="PT2399"?16:8},(_,i)=>"p"+(i+1)):["a","b"];
  for(let i=0;i<sides.length;i++){
   const n=part[sides[i]];
   if(!groups.has(n))groups.set(n,[]);
   groups.get(n).push('<node ref="'+part.id+'" pin="'+(i+1)+'"/>');
  }
 }
 const nets=[...groups].map(([net,pins],i)=>'<net code="'+(i+1)+'" name="'+escape(net)+'">'+pins.join("")+'</net>').join("");
 const documentText='<export version="D"><components>'+components+'</components><nets>'+nets+'</nets></export>';
 const imported=run('importKiCadXmlText('+JSON.stringify(documentText)+')');
 assert.equal(imported.extended,true);
 assert.equal(imported.parts,exported.length);
 assert.deepEqual(run('parts.map(p=>p.id)'),expectedIds);
 assert.equal(run('validate()'),true);
 const deck=run('elements()');
 assert.match(deck,/X1 .+_(?:VBAL|MN3007|MN3101)|X1 .+PT2399_VBAL/);
 console.log("PASS: KiCad XML structured import reconstructs "+effect+" including exact numeric IC pin mapping");
}


// KiCad XML interoperability: optional NC pins must remain floating and
// <node> attribute order cannot change the electrical result.
run('loadEffectFromLibrary("pt2399")');
const ptImportParts=run('parts.map(p=>({...p}))');
const ciValue=p=>p.kind==="VDC"?"DC "+p.value: p.kind==="PT2399"?"PT2399":String(p.value);
const xmlComponents=ptImportParts.map(p=>'<comp ref="'+p.id+'"><value>'+ciValue(p)+'</value></comp>').join("")+
 '<comp ref="#PWR01"><value>GND</value></comp>';
const ciNets=new Map();
for(const part of ptImportParts){
 const pins=part.kind==="PT2399"?Array.from({length:16},(_,i)=>"p"+(i+1)):["a","b"];
 for(let k=0;k<pins.length;k++){
  if(part.kind==="PT2399"&&[5,9].includes(k+1))continue; // intentionally unconnected pins, with no other components on their nets
  const net=part[pins[k]],node='<node pin="'+(k+1)+'" pintype="passive" ref="'+part.id+'"/>';
  if(!ciNets.has(net))ciNets.set(net,[]);
  ciNets.get(net).push(node);
 }
}
const ciNetsXml=[...ciNets].map(([name,nodes],i)=>'<net code="'+(i+1)+'" name="'+name+'">'+nodes.join("")+'</net>').join("");
const ciXml='<export><components>'+xmlComponents+'</components><nets>'+ciNetsXml+'</nets></export>';
const ciResult=run('importKiCadXmlText('+JSON.stringify(ciXml)+')');
assert.equal(ciResult.extended,true);
assert.equal(ciResult.parts,ptImportParts.length);
const ptNc5=run('parts.find(p=>p.id==="X1").p5'),ptNc7=run('parts.find(p=>p.id==="X1").p9');
assert.notEqual(ptNc5,ptNc7,"Optional NC pins must never be silently shorted");
assert.notEqual(ptNc5,run('parts.find(p=>p.id==="X1").p6'));
assert.notEqual(ptNc7,"0");
assert.equal(run('validate()'),true);
const missingP6=ciXml.replace(/<node pin="6" pintype="passive" ref="X1"\/>/,"");
assert.throws(()=>run('importKiCadXmlText('+JSON.stringify(missingP6)+')'),/wymaganych połączeń pinów.*6/);
console.log("PASS: KiCad XML skips power symbols, accepts reordered pin attributes and floats optional IC pins");


// Analog laboratory expansion: each new option must be a real, complete
// editable electrical network, not a macro DSP preset.
const labNames=["mufftone","muffdark","rangetreble","lpbboost","asymclip",
 "tsfeedback","rathard","distplus","bridgedt","tilteq","cockedwah",
 "allpass","zenerdrive","fuzztwo"];
for(const name of labNames){
 run("loadEffectFromLibrary("+JSON.stringify(name)+")");
 assert.equal(run("validate()"),true,"Electrical validation of "+name);
 assert.equal(run("physicalMode"),true,"Physical placement of "+name+": "+run("status.textContent"));
 const deck=run("elements()");
 assert(deck.includes("R")||deck.includes("Q"),"Electrical elements must be serialized for "+name);
 assert.equal(run("selectedMacroFx"),0,"No DSP macro substitution for "+name);
}
console.log("PASS: 14 additional source-linked electrical circuit templates, SPICE serialization and breadboard placement");

// Rotary potentiometer is a controllable physical three-pin voltage divider.
run('loadEffectFromLibrary("mufftone")');
run('selected="P1";inspector()');
const rotaryPanels=controls.get("#inspector").children.filter(e=>e.className==="rotary-control");
assert(rotaryPanels.length>0,"Inspector must show a rotary shaft, not just numeric input");
const face=rotaryPanels.at(-1).children.find(e=>e.tag==="svg");
const knob=face?.children.find(e=>e.attrs?.role==="slider");
assert(knob&&knob.events?.keydown&&knob.events?.pointerdown&&knob.events?.pointerup,"Rotary control must be interactive");
const deckBeforeRotation=run("elements()");
knob.events.keydown({key:"ArrowRight",preventDefault(){}});
const deckAfterRotation=run("elements()");
assert.notEqual(deckBeforeRotation,deckAfterRotation,"Rotating knob must change MNA/SPICE physical resistor split");
assert(controls.get("#scope-a").children.length>5,"Oscilloscope A should be displayed in the main workbench");
assert(controls.get("#scope-b").children.length>5,"Oscilloscope B should be displayed in the main workbench");
run('drawBenchTrace("#scope-b",[0,0.25,-0.3,0.1],"Wejście B")');
assert(controls.get("#scope-b").children.some(e=>e.tag==="path"),"Channel B draws actual supplied samples");
console.log("PASS: mechanical rotary knob changes two resistors and both visible oscilloscopes render independent input/measurement traces");


if(process.argv[2]==="--export-analog"&&process.argv[3]){
 const directory=path.resolve(process.argv[3]);fs.mkdirSync(directory,{recursive:true});
 for(const name of labNames){
  run("loadEffectFromLibrary("+JSON.stringify(name)+")");
  assert.equal(run("validate()"),true);
  const deck=run("elements()");
  fs.writeFileSync(path.join(directory,name+".txt"),deck,"utf8");
 }
 console.log("PASS: exported "+labNames.length+" actual editor-generated analogue fixture decks");
}
