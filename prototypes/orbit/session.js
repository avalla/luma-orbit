/* Versioned web presets and PCM WAV encoding, shared with Node tests. */
(function(root){
'use strict';
const ranges={bpm:[40,200],amount:[0,100],tone:[0,100],bass:[0,100],volume:[0,50],density:[0,100],depth:[0,100],'drum-volume':[0,100],'synth-volume':[0,100],'delay-mix':[0,100],'delay-feedback':[0,75],'room-mix':[0,100],'room-size':[0,100],'delay-division':[0,2],'accent-length':[5,8]};
function validate(p){
 const fail=()=>{throw new Error('Preset Orbit web non valido');};
 if(!p||p.format!=='luma-orbit-web'||p.version!==1)fail();
 for(const key of ['base','notes'])if(!Array.isArray(p[key])||p[key].length!==8||p[key].some(n=>!Number.isInteger(n)||n<0||n>7))fail();
 if(!Array.isArray(p.locks)||p.locks.length!==8||p.locks.some(n=>typeof n!=='boolean'))fail();
 if(!['glass','rubber','mist','keys','bell'].includes(p.sound)||!['minor','major','pentatonic'].includes(p.scale)||typeof p.flow!=='boolean'||typeof p.drumsEnabled!=='boolean')fail();
 if(!p.drumEnabled||['kick','clap','hat'].some(k=>typeof p.drumEnabled[k]!=='boolean'))fail();
 if(!p.controls)fail();for(const [id,[min,max]] of Object.entries(ranges)){const v=p.controls[id];if(!Number.isInteger(v)||v<min||v>max)fail();}
 if(![5,7,8].includes(p.controls['accent-length']))fail();
 if(!Array.isArray(p.phrase)||p.phrase.length<8||p.phrase.length>16||p.phrase.some(e=>!e||!Number.isInteger(e.index)||e.index<0||e.index>7||![1,2].includes(e.beats)||typeof e.rest!=='boolean'))fail();
 if(!Array.isArray(p.drumPattern)||p.drumPattern.length!==16||p.drumPattern.some(h=>!h||['kick','clap','hat'].some(k=>typeof h[k]!=='boolean')||!Number.isFinite(h.velocity)||h.velocity<0||h.velocity>1))fail();
 return JSON.parse(JSON.stringify(p));
}
function wavHeader(samples,rate){
 const b=new ArrayBuffer(44),v=new DataView(b),str=(at,s)=>{for(let i=0;i<s.length;i++)v.setUint8(at+i,s.charCodeAt(i));};
 str(0,'RIFF');v.setUint32(4,36+samples*2,true);str(8,'WAVE');str(12,'fmt ');v.setUint32(16,16,true);v.setUint16(20,1,true);v.setUint16(22,2,true);v.setUint32(24,rate,true);v.setUint32(28,rate*4,true);v.setUint16(32,4,true);v.setUint16(34,16,true);str(36,'data');v.setUint32(40,samples*2,true);return b;
}
const api={ranges,validate,wavHeader};if(typeof module!=='undefined')module.exports=api;else root.OrbitSession=api;
})(globalThis);
