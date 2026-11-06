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
assert(posted&&posted.startsWith("VBAL_SIM\\n"),"WebView2 bridge command must include a newline header");
assert(posted.includes("R1 in out"),"WebView2 must receive the actual graph");
assert(posted.includes(".model DGEN"),"WebView2 must receive the diode model");
console.log("PASS: WebView2 simulation button sends validated SPICE elements");
