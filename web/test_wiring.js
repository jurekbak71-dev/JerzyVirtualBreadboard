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
const tools=["R","C","D"].map(kind=>Object.assign(element("button"),{dataset:{add:kind}}));
const document={
 querySelector(key){if(!controls.has(key))controls.set(key,element());return controls.get(key)},
 querySelectorAll(key){if(key==="[data-add]")return tools;return []},
 createElementNS(ns,tag){return element(tag)},
 createElement(tag){return element(tag)}
};
const context=vm.createContext({document,console,Math,Map,Set,Number,Object,String,Error,URL:{createObjectURL(){return "blob:"},revokeObjectURL(){}},Blob:function(){},setTimeout(){}});
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
assert.equal(plan.assignments["R1.a"].name.startsWith("A"),true);
assert.equal(plan.assignments["C1.b"].name.startsWith("-"),true);
assert.equal(plan.netColumns.in,3);
assert(plan.lines.some(line=>line.includes("R1:")),"Physical plan should list concrete holes");
assert.equal(controls.get("#physical-plan").textContent.includes("PLAN BREADBOARD"),true);
run('physicalMode=false;physicalPlan=null;render()');
console.log("PASS: imported circuit maps to concrete breadboard hole groups and ground rail");

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
assert.equal(anchorPlan.anchorsPhysical.in.name,"E3");
assert.equal(anchorPlan.anchorsPhysical["0"].name,"-1");
assert.notEqual(anchorPlan.assignments["R1.a"].name,anchorPlan.anchorsPhysical.in.name,"IN jumper must not occupy resistor pin hole");
assert.notEqual(anchorPlan.assignments["C1.b"].name,anchorPlan.anchorsPhysical["0"].name,"GND jumper must not occupy component ground hole");
console.log("PASS: physical planner reserves distinct IN/OUT/GND jumper holes");

run('importSpiceText("Physical validation\\nV1 in 0 AC 1\\nR1 in out 10k\\nC1 out 0 100n\\n")');
const safePlan=run("autoPhysicalLayout()");
assert.equal(run("validatePhysicalPlan(physicalPlan)"),true);
assert.throws(()=>run('(()=>{const p=JSON.parse(JSON.stringify(physicalPlan));p.assignments["C1.a"]={...p.assignments["R1.b"]};return validatePhysicalPlan(p)})()'),/kolizja otworu/);
console.log("PASS: physical plan rejects duplicate hole assignments");
