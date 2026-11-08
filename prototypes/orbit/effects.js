'use strict';
class OrbitEffects {
 constructor(context,destination){this.context=context;this.input=context.createGain();this.output=destination;this.nodes=[];this.build();}
 build(){
  const c=this.context;this.delay=c.createDelay(4);this.feedback=c.createGain();this.delayWet=c.createGain();this.room=c.createConvolver();this.roomWet=c.createGain();
  this.delay.delayTime.value=.25;this.feedback.gain.value=0;this.delayWet.gain.value=0;this.roomWet.gain.value=0;
  this.nodes=[this.delay,this.feedback,this.delayWet,this.room,this.roomWet];
  this.input.connect(this.output);this.input.connect(this.delay);this.delay.connect(this.feedback).connect(this.delay);this.delay.connect(this.delayWet).connect(this.output);this.input.connect(this.room);this.room.connect(this.roomWet).connect(this.output);this.size=-1;
 }
 update(v){
  const t=this.context.currentTime,set=(p,x)=>p.setTargetAtTime(x,t,.03);
  set(this.delay.delayTime,Math.min(4,60/v.bpm*[.5,.75,1][v['delay-division']]));set(this.feedback.gain,v['delay-feedback']/100);set(this.delayWet.gain,v['delay-mix']/100*.7);set(this.roomWet.gain,v['room-mix']/100*.6);
  if(this.size!==v['room-size']){
   this.size=v['room-size'];const length=Math.ceil(this.context.sampleRate*(.25+this.size/100*2.75)),buffer=this.context.createBuffer(2,length,this.context.sampleRate);
   let seed=73;for(let ch=0;ch<2;ch++){const data=buffer.getChannelData(ch);for(let i=0;i<length;i++){seed=(1664525*seed+1013904223)>>>0;data[i]=(seed/4294967296*2-1)*Math.pow(1-i/length,3);}}
   this.room.buffer=buffer;
  }
 }
 reset(){this.input.disconnect();for(const node of this.nodes)node.disconnect();this.build();}
}
