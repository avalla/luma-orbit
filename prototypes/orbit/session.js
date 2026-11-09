/* Versioned web presets and PCM WAV encoding, shared with Node tests. */
(function(root){
'use strict';
const ranges={bpm:[40,200],amount:[0,100],tone:[0,100],bass:[0,100],volume:[0,100],density:[0,100],depth:[0,100],'drum-volume':[0,100],'synth-volume':[0,100],'delay-mix':[0,100],'delay-feedback':[0,75],'room-mix':[0,100],'room-size':[0,100],'delay-division':[0,2],'accent-length':[5,8],swing:[0,60],probability:[0,100],energy:[0,100],body:[0,100]};
function validate(input,nested=false){
 const p=JSON.parse(JSON.stringify(input));
 const fail=()=>{throw new Error('Preset Orbit web non valido');};
 if(!p)fail();
 if(p.format==='luma-orbit-web'&&p.version===1){p.format='luma-orbit';p.version=2;p.controls={...p.controls,swing:0,probability:100,energy:50};}
 if(p.format!=='luma-orbit'||p.version!==2)fail();
 if(p.controls&&p.controls.body===undefined)p.controls.body=0;
 for(const key of ['base','notes'])if(!Array.isArray(p[key])||p[key].length!==8||p[key].some(n=>!Number.isInteger(n)||n<0||n>7))fail();
 if(!Array.isArray(p.locks)||p.locks.length!==8||p.locks.some(n=>typeof n!=='boolean'))fail();
 if(!['glass','rubber','mist','keys','bell'].includes(p.sound)||!['minor','major','pentatonic'].includes(p.scale)||typeof p.flow!=='boolean'||typeof p.drumsEnabled!=='boolean')fail();
 if(!p.drumEnabled||['kick','clap','hat'].some(k=>typeof p.drumEnabled[k]!=='boolean'))fail();
 if(!p.controls)fail();for(const [id,[min,max]] of Object.entries(ranges)){const v=p.controls[id];if(!Number.isFinite(v)||v<min||v>max)fail();}
 if(![5,7,8].includes(p.controls['accent-length']))fail();
 if(!Array.isArray(p.phrase)||p.phrase.length<8||p.phrase.length>16||p.phrase.some(e=>!e||!Number.isInteger(e.index)||e.index<0||e.index>7||![1,2].includes(e.beats)||typeof e.rest!=='boolean'))fail();
 if(!Array.isArray(p.drumPattern)||p.drumPattern.length!==16||p.drumPattern.some(h=>!h||['kick','clap','hat'].some(k=>typeof h[k]!=='boolean')||!Number.isFinite(h.velocity)||h.velocity<0||h.velocity>1))fail();
 if(p.scenes!==undefined){if(nested||!Array.isArray(p.scenes)||p.scenes.length!==4)fail();p.scenes=p.scenes.map(s=>s===null?null:validate(s,true));}
 return p;
}
function wavHeader(samples,rate){
 const b=new ArrayBuffer(44),v=new DataView(b),str=(at,s)=>{for(let i=0;i<s.length;i++)v.setUint8(at+i,s.charCodeAt(i));};
 str(0,'RIFF');v.setUint32(4,36+samples*2,true);str(8,'WAVE');str(12,'fmt ');v.setUint32(16,16,true);v.setUint16(20,1,true);v.setUint16(22,2,true);v.setUint32(24,rate,true);v.setUint32(28,rate*4,true);v.setUint16(32,4,true);v.setUint16(34,16,true);str(36,'data');v.setUint32(40,samples*2,true);return b;
}
// Neutral midpoint leaves existing sound untouched; no master gain boost.
function performance(c){const e=(c.energy-50)/100,clamp=x=>Math.max(0,Math.min(100,x));return {...c,tone:clamp(c.tone+e*50),amount:clamp(c.amount+e*60),density:clamp(c.density+e*80)};}
function pulseBeats(index,swing){return .5*(1+(index%2?-1:1)*swing/100);}
function midiFile(input){
 const p=validate(input),ppq=480,tracks=[[],[],[]],notes={minor:[57,59,60,62,64,65,67,69],major:[60,62,64,65,67,69,71,72],pentatonic:[57,60,62,64,67,69,72,74]}[p.scale];
 let pulse=0,accent=0,tick=0;const times=[0];
 const total=p.phrase.reduce((n,e)=>n+e.beats,0);
 for(let i=0;i<total;i++)times.push(times.at(-1)+Math.round(ppq*pulseBeats(i,p.controls.swing)));
 const add=(track,start,end,note,velocity,channel)=>{tracks[track].push([start,[0x90+channel,note,velocity]],[end,[0x80+channel,note,0]]);};
 for(const e of p.phrase){tick=times[pulse];if(!e.rest){const v=accent%p.controls['accent-length']%2===0?100:65;add(0,tick,Math.max(tick+1,times[pulse+e.beats]-12),notes[p.notes[e.index]]-(p.sound==='rubber'?12:0),v,0);if(p.controls.bass>0)add(1,tick,Math.max(tick+1,times[pulse+e.beats]-12),notes[p.notes[e.index]]-24,85,1);}pulse+=e.beats;accent+=e.beats;}
 if(p.drumsEnabled)for(let i=0;i<total;i++){const h=p.drumPattern[i%16];for(const [k,n] of [['kick',36],['clap',39],['hat',42]])if(h[k]&&p.drumEnabled[k])add(2,times[i],times[i]+30,n,Math.max(1,Math.round(h.velocity*100)),9);}
 const be=(v,n)=>Array.from({length:n},(_,i)=>(v>>>((n-1-i)*8))&255),str=s=>Array.from(s,c=>c.charCodeAt(0));
 const vlq=n=>{let b=[n&127];while(n>>=7)b.unshift((n&127)|128);return b;};
 const tempo=Math.round(60000000/p.controls.bpm),end=times.at(-1);
 const chunks=tracks.map((events,index)=>{let last=0;const bytes=index===0?[0,255,81,3,...be(tempo,3),0,255,88,4,4,2,24,8]:[];events.sort((a,b)=>a[0]-b[0]||a[1][0]-b[1][0]);for(const [t,msg] of events){bytes.push(...vlq(t-last),...msg);last=t;}bytes.push(...vlq(Math.max(0,end-last)),255,47,0);return [...str('MTrk'),...be(bytes.length,4),...bytes];});
 return new Uint8Array([...str('MThd'),0,0,0,6,0,1,0,3,...be(ppq,2),...chunks.flat()]);
}
const api={ranges,validate,wavHeader,performance,pulseBeats,midiFile};if(typeof module!=='undefined')module.exports=api;else root.OrbitSession=api;
})(globalThis);
