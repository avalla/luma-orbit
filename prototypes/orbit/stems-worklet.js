/* One render callback captures all three dry buses on the same sample boundary. */
class OrbitStems extends AudioWorkletProcessor {
 constructor(){super();this.active=false;this.frames=0;this.used=0;this.chunks=Array.from({length:3},()=>new ArrayBuffer(16384));this.views=this.chunks.map(b=>new DataView(b));this.port.onmessage=({data})=>{if(data==='start'){this.frames=0;this.used=0;this.active=true;}if(data==='stop')this.finish();};}
 flush(){if(!this.used)return;for(let track=0;track<3;track++){const chunk=this.chunks[track].slice(0,this.used*2);this.port.postMessage({track,chunk},[chunk]);}this.used=0;}
 finish(){if(!this.active)return;this.active=false;this.flush();this.port.postMessage({done:true,frames:this.frames});}
 process(inputs,outputs){
  if(!this.active)return true;const count=outputs[0]?.[0]?.length||128;
  for(let i=0;i<count;i++){
   for(let t=0;t<3;t++)for(let c=0;c<2;c++){const v=inputs[t]?.[c]?.[i]??inputs[t]?.[0]?.[i]??0,x=Number.isFinite(v)?Math.max(-1,Math.min(1,v)):0;this.views[t].setInt16((this.used+c)*2,Math.round(x*(x<0?32768:32767)),true);}
   this.used+=2;this.frames++;if(this.used===8192)this.flush();if(this.frames>=sampleRate*300){this.finish();break;}
  }return true;
 }
}
registerProcessor('orbit-stems',OrbitStems);
