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
