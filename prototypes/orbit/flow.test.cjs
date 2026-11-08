const assert=require('node:assert/strict');
const {createPhrase}=require('./flow.js');
const initial=[0,4,2,6,4,7,2,3];
let seed=7123;
const random=()=>{seed=(seed*1664525+1013904223)>>>0;return seed/4294967296;};
const locks=[true,false,false,false,true,false,false,false];
const lengths=new Set();let notes=[...initial],rests=0,long=0;
for(let n=0;n<200;n++){
 const next=createPhrase(notes,locks,.8,random);
 assert.equal(next.notes[0],initial[0]);assert.equal(next.notes[4],initial[4]);
 assert(next.notes.some((v,i)=>v!==notes[i]));
 assert(next.notes.every(v=>Number.isInteger(v)&&v>=0&&v<=7));
 lengths.add(next.events.length);
 for(const e of next.events){
  assert(e.index>=0&&e.index<8);assert([1,2].includes(e.beats));
  if(locks[e.index]){assert.equal(e.rest,false);assert.equal(e.beats,1);}
  rests+=e.rest?1:0;long+=e.beats===2?1:0;
 }
 notes=next.notes;
}
assert.equal(lengths.size,3);assert(rests>0);assert(long>0);
for(const [amount,fixed] of [[0,locks],[1,Array(8).fill(true)]]){
 const result=createPhrase(initial,fixed,amount,random);
 assert.deepEqual(result.notes,initial);
 assert.deepEqual(result.events,initial.map((_,index)=>({index,rest:false,beats:1})));
}
console.log('PASS: 200 evolving phrases, locked notes, bounds, rests, lengths, zero variation and all locked.');
