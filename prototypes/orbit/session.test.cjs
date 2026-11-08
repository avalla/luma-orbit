'use strict';
const assert=require('node:assert/strict'),vm=require('node:vm'),fs=require('node:fs');
const {validate,ranges,wavHeader}=require('./session.js'),drums=require('./drums.js');
const preset={format:'luma-orbit',version:2,base:Array(8).fill(0),notes:Array(8).fill(1),locks:Array(8).fill(false),sound:'bell',scale:'minor',flow:false,drumsEnabled:true,drumEnabled:{kick:true,clap:false,hat:true},controls:Object.fromEntries(Object.entries(ranges).map(([k,[min]])=>[k,min])),phrase:Array.from({length:8},(_,index)=>({index,beats:1,rest:false})),drumPattern:drums.pattern(.5)};
assert.deepEqual(validate(preset),preset);
const old={...structuredClone(preset),format:'luma-orbit-web',version:1};delete old.controls.energy;delete old.controls.swing;delete old.controls.probability;
assert.equal(validate(old).controls.energy,50);assert.equal(validate(old).controls.probability,100);
const bank=structuredClone(preset);bank.scenes=[structuredClone(preset),null,null,null];assert.deepEqual(validate(bank),bank);
bank.scenes[0].scenes=[];assert.throws(()=>validate(bank));
for(const mutate of [p=>p.version=3,p=>p.notes[0]=8,p=>p.controls.bpm=Infinity,p=>p.controls['delay-feedback']=99,p=>p.locks=[],p=>p.phrase[0].beats=0,p=>p.drumPattern[0].velocity=2]){const p=structuredClone(preset);mutate(p);assert.throws(()=>validate(p));}
const header=new DataView(wavHeader(96000,48000));assert.equal(header.getUint32(40,true),192000);assert.equal(header.getUint16(22,true),2);assert.equal(header.getUint32(24,true),48000);
let Processor;const messages=[];
vm.runInNewContext(fs.readFileSync(__dirname+'/recorder-worklet.js','utf8'),{AudioWorkletProcessor:class{constructor(){this.port={postMessage:m=>messages.push(m)};}},sampleRate:100,registerProcessor:(_,p)=>Processor=p});
const recorder=new Processor();recorder.port.onmessage({data:'start'});recorder.process([[new Float32Array([.5,-1,2])]]);recorder.port.onmessage({data:'stop'});
assert.equal(messages.at(-1).frames,3);const pcm=new DataView(messages[0].chunk);assert.equal(pcm.getInt16(0,true),16384);assert.equal(pcm.getInt16(2,true),16384);assert.equal(pcm.getInt16(4,true),-32768);assert.equal(pcm.getInt16(8,true),32767);
messages.length=0;recorder.port.onmessage({data:'start'});for(let i=0;i<300;i++)recorder.process([[new Float32Array(128)]]);assert.equal(messages.at(-1).frames,30000);assert.equal(messages.filter(m=>m.done).length,1);
console.log('PASS: presets validation, WAV header, stereo PCM, clipping, stop and five-minute limit');

const {pulseBeats,performance,midiFile}=require('./session.js');
for(const swing of [0,20,60])assert.equal(Array.from({length:8},(_,i)=>pulseBeats(i,swing)).reduce((a,b)=>a+b),4);
assert.equal(performance({...preset.controls,energy:50,tone:62}).tone,62);
assert.equal(performance({...preset.controls,energy:100,tone:90}).tone,100);
const score=structuredClone(preset);score.controls.bpm=120;score.controls.swing=50;score.controls.bass=30;score.phrase[1].rest=true;score.phrase[2].beats=2;
const bytes=Buffer.from(midiFile(score));assert.equal(bytes.toString('ascii',0,4),'MThd');assert.equal(bytes.readUInt16BE(10),3);assert.equal(bytes.readUInt16BE(12),480);
// Parse the file independently: all note-ons must have matching note-offs, three tracks end together.
let offset=14;const lengths=[];
for(let track=0;track<3;track++){
 assert.equal(bytes.toString('ascii',offset,offset+4),'MTrk');const end=offset+8+bytes.readUInt32BE(offset+4);offset+=8;let time=0;const active=new Set();
 const vlq=()=>{let n=0,b;do{b=bytes[offset++];n=(n<<7)|(b&127);}while(b&128);return n;};
 while(offset<end){time+=vlq();const status=bytes[offset++];if(status===255){const kind=bytes[offset++],length=vlq();if(kind===47)lengths.push(time);offset+=length;}else{const note=bytes[offset++],velocity=bytes[offset++];if((status&240)===144&&velocity>0){assert(!active.has(note));active.add(note);}else{assert(active.has(note));active.delete(note);}}}
 assert.equal(active.size,0);
}
assert.equal(new Set(lengths).size,1);assert.equal(lengths[0],2280); // Nine swung eighths: 4.75 quarters.
let StemProcessor;const stemMessages=[];
vm.runInNewContext(fs.readFileSync(__dirname+'/stems-worklet.js','utf8'),{AudioWorkletProcessor:class{constructor(){this.port={postMessage:m=>stemMessages.push(m)};}},sampleRate:100,registerProcessor:(_,p)=>StemProcessor=p});
const stems=new StemProcessor();stems.port.onmessage({data:'start'});stems.process([[new Float32Array([.25,.5])],[new Float32Array([-.5,-.25])],[]],[[new Float32Array(2)]]);stems.port.onmessage({data:'stop'});
assert.equal(stemMessages.at(-1).frames,2);assert.deepEqual(stemMessages.slice(0,3).map(m=>m.chunk.byteLength),[8,8,8]);assert(new DataView(stemMessages[0].chunk).getInt16(0,true)>0);assert(new DataView(stemMessages[1].chunk).getInt16(0,true)<0);assert.equal(new DataView(stemMessages[2].chunk).getInt16(0,true),0);
stemMessages.length=0;stems.port.onmessage({data:'start'});for(let i=0;i<300;i++)stems.process([],[[new Float32Array(128)]]);assert.equal(stemMessages.at(-1).frames,30000);
console.log('PASS: legacy migration, scene bank validation, swing bar length, Energy, parsed MIDI tracks and aligned stems');
