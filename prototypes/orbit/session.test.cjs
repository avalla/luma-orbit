'use strict';
const assert=require('node:assert/strict'),vm=require('node:vm'),fs=require('node:fs');
const {validate,ranges,wavHeader}=require('./session.js'),drums=require('./drums.js');
const preset={format:'luma-orbit-web',version:1,base:Array(8).fill(0),notes:Array(8).fill(1),locks:Array(8).fill(false),sound:'bell',scale:'minor',flow:false,drumsEnabled:true,drumEnabled:{kick:true,clap:false,hat:true},controls:Object.fromEntries(Object.entries(ranges).map(([k,[min]])=>[k,min])),phrase:Array.from({length:8},(_,index)=>({index,beats:1,rest:false})),drumPattern:drums.pattern(.5)};
assert.deepEqual(validate(preset),preset);
for(const mutate of [p=>p.version=2,p=>p.notes[0]=8,p=>p.controls.bpm=Infinity,p=>p.controls['delay-feedback']=99,p=>p.locks=[],p=>p.phrase[0].beats=0,p=>p.drumPattern[0].velocity=2]){const p=structuredClone(preset);mutate(p);assert.throws(()=>validate(p));}
const header=new DataView(wavHeader(96000,48000));assert.equal(header.getUint32(40,true),192000);assert.equal(header.getUint16(22,true),2);assert.equal(header.getUint32(24,true),48000);
let Processor;const messages=[];
vm.runInNewContext(fs.readFileSync(__dirname+'/recorder-worklet.js','utf8'),{AudioWorkletProcessor:class{constructor(){this.port={postMessage:m=>messages.push(m)};}},sampleRate:100,registerProcessor:(_,p)=>Processor=p});
const recorder=new Processor();recorder.port.onmessage({data:'start'});recorder.process([[new Float32Array([.5,-1,2])]]);recorder.port.onmessage({data:'stop'});
assert.equal(messages.at(-1).frames,3);const pcm=new DataView(messages[0].chunk);assert.equal(pcm.getInt16(0,true),16384);assert.equal(pcm.getInt16(2,true),16384);assert.equal(pcm.getInt16(4,true),-32768);assert.equal(pcm.getInt16(8,true),32767);
messages.length=0;recorder.port.onmessage({data:'start'});for(let i=0;i<300;i++)recorder.process([[new Float32Array(128)]]);assert.equal(messages.at(-1).frames,30000);assert.equal(messages.filter(m=>m.done).length,1);
console.log('PASS: presets validation, WAV header, stereo PCM, clipping, stop and five-minute limit');
