/* Capture the final stereo mix, bounded to five minutes. */
class OrbitRecorder extends AudioWorkletProcessor {
 constructor(){super();this.active=false;this.frames=0;this.used=0;this.chunk=new ArrayBuffer(16384);this.view=new DataView(this.chunk);this.port.onmessage=({data})=>{if(data==='start'){this.frames=0;this.used=0;this.active=true;}if(data==='stop')this.finish();};}
 flush(){if(!this.used)return;const data=this.chunk.slice(0,this.used*2);this.port.postMessage({chunk:data},[data]);this.used=0;}
 finish(){if(!this.active)return;this.active=false;this.flush();this.port.postMessage({done:true,frames:this.frames});}
 process(inputs){
  if(!this.active)return true;const channels=inputs[0];if(!channels?.length)return true;
  const left=channels[0],right=channels[1]||left;
  for(let i=0;i<left.length;i++){
   for(let ch=0;ch<2;ch++){const value=ch?right[i]:left[i],x=Math.max(-1,Math.min(1,Number.isFinite(value)?value:0));this.view.setInt16(this.used++*2,Math.round(x*(x<0?32768:32767)),true);}
   this.frames++;if(this.used===8192)this.flush();if(this.frames>=sampleRate*300){this.finish();break;}
  }return true;
 }
}
registerProcessor('orbit-recorder',OrbitRecorder);
